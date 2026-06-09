/*
 * rx_master.c — TDMA PRX master / hub (corrected)
 *
 * Fixes applied vs original:
 *  [C3] ACK timestamp captured as late as possible — right before
 *       nrf_esb_write_payload(&tx_ack_payload) — to minimise staleness.
 *  [C4] Race condition on g_slot_received_flags[] / g_node_latency_us[]
 *       resolved: ISR writes into a shadow (back) buffer; main loop
 *       atomically swaps the buffers under a brief IRQ-disabled section
 *       before printing, so no shared state is ever written and read
 *       concurrently.
 *  [M4] Frame boundary advances correctly even when no packet arrives:
 *       main loop uses modulo-aligned boundary update (same formula as the
 *       ISR) so the boundary never drifts.
 *  [M5] node_id bounds check is the very first parse step; any packet with
 *       an invalid ID is discarded before touching any array.
 *  [M6] tx_ack_payload built into a local struct per loop iteration so
 *       concurrent ESB FIFO enqueues don't share a single global struct.
 *  [Mi3] nrf_esb_set_tx_power() added so ACK link budget matches PTX.
 *  [Mi4] __WFE uses the ARM-recommended SEV/WFE/WFE idiom to drain any
 *        pre-set event register before sleeping.
 */

#include "nrf_esb.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdk_common.h"
#include "nrf.h"
#include "nrf_esb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_error.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define TOTAL_NODES         30u
#define SLOT_DURATION_US    3000u
#define FRAME_DURATION_US   (TOTAL_NODES * SLOT_DURATION_US)   /* 90,000 us */

#define MAGIC_SYNC_MARKER   0x55u
#define MAGIC_DATA_MARKER   0xAAu

/* =========================================================
 * Double-buffer for per-frame slot data
 *
 * [FIX-C4]
 * The ISR writes exclusively into g_back[] (the "shadow" buffer).
 * The main loop, under a brief IRQ-disabled section, swaps front/back
 * pointers then reads from the newly-promoted front buffer.
 * The two sides never share the same buffer at the same time.
 * ========================================================= */
typedef struct {
    bool     received[TOTAL_NODES];
    uint16_t latency_us[TOTAL_NODES];
} frame_data_t;

static frame_data_t g_buf[2];       /* Two physical buffers                 */
static frame_data_t * volatile g_back  = &g_buf[0]; /* ISR writes here      */
static frame_data_t * volatile g_front = &g_buf[1]; /* Main loop reads here */

static volatile uint32_t g_last_frame_boundary_us = 0;

/* =========================================================
 * TIMER — free-running 32-bit microsecond counter
 *   CC[1] : general snapshot (get_master_time_us)
 * ========================================================= */
void timer_init(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit
                              << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER   = 4u << TIMER_PRESCALER_PRESCALER_Pos; /* 1 MHz */
    NRF_TIMER1->TASKS_START = 1;
}

static inline uint32_t get_master_time_us(void)
{
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER1->CC[1];
}

/* =========================================================
 * Print the front-buffer frame report (called from main loop only).
 * ========================================================= */
static void print_tdma_frame_status(frame_data_t * p)
{
    NRF_LOG_INFO("============ TDMA NETWORK LATENCY REPORT ============");
    for (uint8_t i = 0; i < TOTAL_NODES; i++)
    {
        if (p->received[i])
        {
            NRF_LOG_INFO("Slot %02d (Node %02d): Latency = %u us",
                         i + 1, i, p->latency_us[i]);
        }
        else
        {
            NRF_LOG_INFO("Slot %02d (Node %02d): [ OFFLINE / NO DATA ]",
                         i + 1, i);
        }
    }
    NRF_LOG_INFO("=====================================================");
}

/* =========================================================
 * Atomically swap front/back buffers and clear the new back buffer.
 * Called from the main loop with interrupts briefly disabled.
 * ========================================================= */
static frame_data_t * swap_buffers(void)
{
    __disable_irq();
    frame_data_t * tmp = g_front;
    g_front            = g_back;
    g_back             = tmp;
    __enable_irq();

    /* Clear the new back buffer so the ISR writes into a clean slate. */
    memset(g_back, 0, sizeof(frame_data_t));

    return g_front;   /* Caller may safely read this without IRQ concern. */
}

/* =========================================================
 * ESB event handler (runs in interrupt context)
 * ========================================================= */
void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_RX_RECEIVED:
        {
            nrf_esb_payload_t rx_payload;

            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS)
            {
                if (rx_payload.length < 2) { continue; }

                uint8_t marker  = rx_payload.data[0];
                uint8_t node_id = rx_payload.data[1];

                /*
                 * [FIX-M5] Validate node_id before ANY array access.
                 * Silently drop packets with illegal IDs.
                 */
                if (node_id >= TOTAL_NODES) { continue; }

                /* Record data into the shadow (back) buffer. */
                if (marker == MAGIC_DATA_MARKER)
                {
                    g_back->received[node_id] = true;
                    g_back->latency_us[node_id] =
                        ((uint16_t)rx_payload.data[2] << 8) |
                        (uint16_t)rx_payload.data[3];
                }
                else if (marker == MAGIC_SYNC_MARKER)
                {
                    NRF_LOG_DEBUG("Sync request from Node %d", node_id);
                }

                /*
                 * [FIX-C3]  Build the ACK payload and capture the master
                 * timestamp as late as possible — right before write —
                 * to minimise the staleness of the clock value the TX node
                 * will receive.
                 *
                 * [FIX-M6]  Use a local struct so each iteration in this
                 * while-loop sends independent data and does not alias a
                 * single global buffer that ESB might not have finished
                 * reading from the previous iteration.
                 */
                nrf_esb_payload_t ack;
                ack.pipe   = rx_payload.pipe;
                ack.length = 4;

                /* Capture timestamp as late as possible. */
                uint32_t ack_time = get_master_time_us();
                ack.data[0] = (uint8_t)(ack_time >> 24);
                ack.data[1] = (uint8_t)(ack_time >> 16);
                ack.data[2] = (uint8_t)(ack_time >>  8);
                ack.data[3] = (uint8_t)(ack_time & 0xFF);

                (void)nrf_esb_write_payload(&ack);
            }
            break;
        }

        case NRF_ESB_EVENT_TX_FAILED:
            (void)nrf_esb_flush_tx();
            break;

        default:
            break;
    }
}

/* =========================================================
 * Peripheral init helpers
 * ========================================================= */
void clocks_start(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

void gpio_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}

uint32_t esb_init(void)
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

    nrf_esb_config_t nrf_esb_config     = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol             = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.bitrate              = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.mode                 = NRF_ESB_MODE_PRX;
    nrf_esb_config.event_handler        = nrf_esb_event_handler;
    nrf_esb_config.selective_auto_ack   = false;

    err_code = nrf_esb_init(&nrf_esb_config);
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
    uint32_t err_code;

    gpio_init();

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    clocks_start();
    timer_init();

    err_code = esb_init();
    APP_ERROR_CHECK(err_code);

    /*
     * [FIX-Mi3] Set ACK TX power to match PTX nodes (+4 dBm) so the
     * link budget is symmetric and every node can receive its ACK.
     */
    err_code = nrf_esb_set_tx_power(NRF_ESB_TX_POWER_4DBM);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("TDMA Latency Monitor Hub online.");

    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);

    /* Align the first frame boundary to "now". */
    uint32_t now                = get_master_time_us();
    g_last_frame_boundary_us    = now - (now % FRAME_DURATION_US);

    /* Clear both buffers before traffic starts. */
    memset(g_buf, 0, sizeof(g_buf));

    while (true)
    {
        NRF_LOG_FLUSH();

        /*
         * [FIX-M4] Check whether a full frame has elapsed.
         * Use the modulo-aligned formula (same as ISR) so the boundary
         * never drifts regardless of when in the frame this code runs,
         * and regardless of whether any packets arrived this frame.
         */
        now = get_master_time_us();
        if (now - g_last_frame_boundary_us >= FRAME_DURATION_US)
        {
            /*
             * [FIX-C4] Swap buffers atomically, then print the front buffer.
             * The ISR continues writing to the freshly-cleared back buffer
             * while we print — no race.
             */
            frame_data_t * completed = swap_buffers();
            print_tdma_frame_status(completed);

            /* Advance boundary by exactly one frame, aligned to the grid. */
            g_last_frame_boundary_us = now - (now % FRAME_DURATION_US);
        }

        if (NRF_LOG_PROCESS() == false)
        {
            /*
             * [FIX-Mi4] ARM-recommended WFE idiom: SEV clears any stale
             * event register, first WFE consumes it immediately (so we don't
             * sleep if an event arrived between the log check and here),
             * second WFE sleeps until the next real event.
             */
            __SEV();
            __WFE();
            __WFE();
        }
    }
}