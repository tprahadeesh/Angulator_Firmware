/*
 * tx_node.c — TDMA PTX node
 *
 * History:
 *  v1  — original (vulnerabilities found in review)
 *  v2  — over-corrected; introduced LPF-during-sync corruption and
 *         fine-spin race.  This version reverts to the original simple
 *         structure and applies only the three fixes that matter:
 *
 *  [F1] TIMER1 is never reset after init.  RTT measured as CC[0]-CC[2]
 *       (elapsed), not CC[0] (absolute).
 *
 *  [F2] Clock offset is int32_t.  LPF applied ONLY after initial lock
 *       (not during the sync-request flood), so the offset converges
 *       cleanly on the first real data ACK.
 *
 *  [F3] The post-delay overshoot guard is removed.  It was correct in
 *       theory but caused the node to skip every slot because nrf_delay_us
 *       has ~±50 µs accuracy, and the guard window (3000 µs) is too
 *       narrow relative to jitter accumulated over the 87 ms wait.
 *       Instead: fire when we arrive — if we're a few hundred µs early or
 *       late within the 3 ms slot that is fine.  The master has a 3 ms
 *       window to receive us.
 *
 *  [F4] No nrf_delay_us(SLOT_DURATION_US) at the end of the loop.
 *       Timing is re-derived from the live master clock at the top of
 *       every iteration, so the loop self-corrects each frame.
 *
 *  [F5] APP_ERROR_CHECK used for nrf_esb_set_tx_power (not VERIFY_SUCCESS).
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdk_common.h"
#include "nrf.h"
#include "nrf_esb.h"
#include "nrf_error.h"
#include "nrf_esb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "app_util.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define LED_PIN             NRF_GPIO_PIN_MAP(0, 14)

/* =========================================================
 * TDMA NETWORK CONFIGURATION
 * ========================================================= */
#define NODE_ID             3
#define TOTAL_NODES         30
#define SLOT_DURATION_US    3000u
#define FRAME_DURATION_US   (TOTAL_NODES * SLOT_DURATION_US)   /* 90 ms */

#define MAGIC_SYNC_MARKER   0x55u
#define MAGIC_DATA_MARKER   0xAAu

/*
 * LPF: offset = (3/4)*old + (1/4)*new_measurement
 * Time-constant ≈ 4 frames = 360 ms.  Fast enough to track crystal
 * drift (<±20 ppm), slow enough to reject single-packet jitter.
 */
#define OFFSET_LPF_SHIFT    2u   /* alpha = 1 / (1 << 2) = 0.25 */

typedef enum {
    STATE_UNSYNCED,
    STATE_SYNCHRONIZED
} tx_state_t;

static volatile tx_state_t  g_current_state         = STATE_UNSYNCED;

/* [F2] int32_t so signed arithmetic works across uint32_t rollover. */
static volatile int32_t     g_master_time_offset_us = 0;

/* [F1] Elapsed RTT = CC[0] - CC[2]. */
static volatile uint16_t    g_measured_latency_us   = 0;

static volatile bool        g_ack_received          = false;
static nrf_esb_payload_t    tx_payload;
static nrf_esb_payload_t    rx_ack_payload;

/* =========================================================
 * TIMER1 — free-running 32-bit 1 MHz counter.  NEVER reset after init.
 *   CC[0] = RTT end    (TX_SUCCESS ISR)
 *   CC[1] = snapshot   (get_local_time_us)
 *   CC[2] = RTT start  (just before nrf_esb_write_payload)
 * ========================================================= */
static void timer_init(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit
                              << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER   = 4u << TIMER_PRESCALER_PRESCALER_Pos; /* 1 MHz */
    NRF_TIMER1->TASKS_START = 1;
}

static inline uint32_t get_local_time_us(void)
{
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER1->CC[1];
}

/* Apply signed offset so rollover is handled correctly. */
static inline uint32_t get_master_time_us(void)
{
    return (uint32_t)((int32_t)get_local_time_us() + g_master_time_offset_us);
}

/* =========================================================
 * ESB event handler
 * ========================================================= */
void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            /* [F1] Elapsed RTT = end - start. */
            NRF_TIMER1->TASKS_CAPTURE[0] = 1;
            uint16_t rtt = (uint16_t)(NRF_TIMER1->CC[0] - NRF_TIMER1->CC[2]);
              if (rtt >= 150u)   /* sanity check: below this is physically impossible */
            {
                 g_measured_latency_us = rtt;
            }
            g_ack_received = true;
            break;

        case NRF_ESB_EVENT_TX_FAILED:
            (void)nrf_esb_flush_tx();
            break;

        case NRF_ESB_EVENT_RX_RECEIVED:
            while (nrf_esb_read_rx_payload(&rx_ack_payload) == NRF_SUCCESS)
            {
                if (rx_ack_payload.length < 4) { continue; }

                uint32_t master_time =
                    ((uint32_t)rx_ack_payload.data[0] << 24) |
                    ((uint32_t)rx_ack_payload.data[1] << 16) |
                    ((uint32_t)rx_ack_payload.data[2] <<  8) |
                    ((uint32_t)rx_ack_payload.data[3]);

                uint32_t local_now  = get_local_time_us();

                /*
                 * Adjust for the one-way propagation delay: the master
                 * stamped the ACK before it flew back over the air, so
                 * add half the measured RTT to compensate.
                 */
                master_time += (uint32_t)(g_measured_latency_us / 2u);

                int32_t raw_offset = (int32_t)(master_time - local_now);

                if (g_current_state == STATE_UNSYNCED)
                {
                    /*
                     * [F2] First lock: accept the raw offset immediately.
                     * Do NOT apply LPF here — the node has been firing
                     * sync packets rapidly and each one returned a slightly
                     * different timestamp.  We want the single cleanest
                     * value, not a blend of all of them.
                     */
                    g_master_time_offset_us = raw_offset;
                    g_current_state         = STATE_SYNCHRONIZED;
                    NRF_LOG_INFO("Clock LOCKED. Offset=%d us  RTT=%u us",
                                 g_master_time_offset_us,
                                 g_measured_latency_us);
                }
                else
                {
                    /* Continuous drift correction — LPF after lock. */
                    g_master_time_offset_us =
                        g_master_time_offset_us
                        - (g_master_time_offset_us >> OFFSET_LPF_SHIFT)
                        + (raw_offset              >> OFFSET_LPF_SHIFT);
                }
            }
            break;

        default:
            break;
    }
}

/* =========================================================
 * Peripheral init
 * ========================================================= */
static void clocks_start(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

static uint32_t esb_init(void)
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

    nrf_esb_config_t cfg       = NRF_ESB_DEFAULT_CONFIG;
    cfg.protocol               = NRF_ESB_PROTOCOL_ESB_DPL;
    cfg.retransmit_delay       = 250;
    cfg.retransmit_count       = 3;
    cfg.bitrate                = NRF_ESB_BITRATE_2MBPS;
    cfg.event_handler          = nrf_esb_event_handler;
    cfg.mode                   = NRF_ESB_MODE_PTX;
    cfg.selective_auto_ack     = false;

    err_code = nrf_esb_init(&cfg);
    VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_prefixes(addr_prefix, NRF_ESB_PIPE_COUNT);
    VERIFY_SUCCESS(err_code);
    return err_code;
}

/* =========================================================
 * Main
 * ========================================================= */
int main(void)
{
    ret_code_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    clocks_start();
    timer_init();

    err_code = esb_init();
    APP_ERROR_CHECK(err_code);

    /* [F5] APP_ERROR_CHECK from main(), not VERIFY_SUCCESS. */
    err_code = nrf_esb_set_tx_power(NRF_ESB_TX_POWER_4DBM);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_output(LED_PIN);

    tx_payload.pipe   = 0;
    tx_payload.noack  = false;
    tx_payload.length = 12;

    NRF_LOG_INFO("TDMA Node %d — searching for master...", NODE_ID);

    while (true)
    {
        NRF_LOG_FLUSH();

        /* ==============================================================
         * PHASE 1: UNSYNCED
         * Fire sync requests at ~2 ms intervals and wait for the ISR to
         * set STATE_SYNCHRONIZED.
         * ============================================================== */
        if (g_current_state == STATE_UNSYNCED)
        {
            tx_payload.data[0] = MAGIC_SYNC_MARKER;
            tx_payload.data[1] = NODE_ID;
            for (int i = 2; i < 12; i++) { tx_payload.data[i] = 0; }

            (void)nrf_esb_flush_tx();
            (void)nrf_esb_write_payload(&tx_payload);
            nrf_delay_us(2000u);   /* give radio time; not a timing reference */
            continue;
        }

        /* ==============================================================
         * PHASE 2: SYNCHRONIZED TDMA STEADY-STATE
         *
         * Compute how long until this node's slot opens and sleep for
         * most of that time.  No guard band, no fine-spin, no overshoot
         * check — the 3 ms slot is wide enough to absorb nrf_delay_us
         * inaccuracy (≤ ±50 µs on nRF52 at 64 MHz).
         *
         * [F4] No blind nrf_delay_us(SLOT_DURATION_US) at the bottom.
         *      The wait is computed fresh from the master clock each frame.
         * ============================================================== */
        uint32_t master_now    = get_master_time_us();
        uint32_t frame_pos     = master_now % FRAME_DURATION_US;
        uint32_t slot_start    = (uint32_t)NODE_ID * SLOT_DURATION_US;
        uint32_t time_to_wait;

        if (frame_pos < slot_start)
        {
            time_to_wait = slot_start - frame_pos;
        }
        else if (frame_pos < slot_start + SLOT_DURATION_US)
        {
            /* Already inside the slot — fire immediately. */
            time_to_wait = 0u;
        }
        else
        {
            /* Slot has passed for this frame — wait for the next one. */
            time_to_wait = (FRAME_DURATION_US - frame_pos) + slot_start;
        }

        if (time_to_wait > 0u)
        {
            nrf_delay_us(time_to_wait);
        }

        /* ==============================================================
         * TRANSMISSION
         * ============================================================== */
        tx_payload.data[0] = MAGIC_DATA_MARKER;
        tx_payload.data[1] = NODE_ID;
        tx_payload.data[2] = (uint8_t)(g_measured_latency_us >> 8);
        tx_payload.data[3] = (uint8_t)(g_measured_latency_us & 0xFF);
        for (uint8_t i = 4; i < 12; i++) { tx_payload.data[i] = 0xBB; }

        (void)nrf_esb_flush_tx();

        /* Capture RTT start just before handing off to ESB. */
        NRF_TIMER1->TASKS_CAPTURE[2] = 1;

        if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS)
        {
            nrf_gpio_pin_toggle(LED_PIN);
        }

        /*
         * [F4] No nrf_delay_us(SLOT_DURATION_US) here.
         * Loop back to the top; get_master_time_us() will compute the
         * correct wait to the next frame's slot.
         */
    }
}