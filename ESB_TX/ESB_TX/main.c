/**
 * @file ptx_slave.c
 * @brief PTX Slave - PCA10040e (nRF52810)
 *
 * Simple two-pipe design:
 *   Pipe 0 - Discovery : sends 0xFF join request → receives 0xFE + id in ACK
 *   Pipe 1 - Data      : sends 9-byte dummy payload every 30 ms
 *                        if ACK payload = 0x99 → master kicked us, re-discover
 *
 * ID is held in RAM only — power cycle always starts fresh discovery.
 * If TX fails repeatedly (master gone) → forget ID, re-discover.
 *
 * Collision avoidance: random back-off delay on discovery using DEVICEID.
 * ESB retransmit handles transient packet loss on the data pipe.
 */

#include <stdbool.h>
#include <stdint.h>
#include "sdk_common.h"
#include "nrf.h"
#include "nrf_esb.h"
#include "nrf_error.h"
#include "nrf_esb_error_codes.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/* ── Config ──────────────────────────────────────────────────────── */
#define PIPE_ID_ALLOC        0
#define PIPE_DATA            1

#define MARKER_JOIN_REQ      0xFF
#define MARKER_ID_ASSIGN     0xFE
#define MARKER_DISASSOC      0x99

#define ESB_RETRANSMIT_COUNT 6      /* attempts per packet before TX_FAILED */
#define ESB_RETRANSMIT_DELAY 250    /* µs between retransmits */

#define DATA_INTERVAL_MS     30     /* how often to send data once registered */

/* After this many consecutive TX failures → forget ID and re-discover.
 * 10 × ~30 ms = ~300 ms of silence before giving up. */
#define TX_FAIL_LIMIT        10

/* Discovery back-off: random window so two slaves don't collide forever.
 * Each slave picks a different slot based on its unique DEVICEID. */
#define DISCOVERY_BASE_MS    200
#define DISCOVERY_RAND_MS    400

/* ── State ───────────────────────────────────────────────────────── */
typedef enum {
    STATE_DISCOVERY,
    STATE_DATA,
} state_t;

static volatile state_t  m_state     = STATE_DISCOVERY;
static volatile uint8_t  m_my_id     = 0;
static volatile uint8_t  m_fail_cnt  = 0;
static volatile bool     m_tx_done   = false;

static uint16_t          m_pkt_cnt   = 0;

static nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;

/* ── Timer ───────────────────────────────────────────────────────── */
static void timer_init(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->MODE        = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER1->PRESCALER   = 4;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->TASKS_START = 1;
}

/* ── ESB event handler ───────────────────────────────────────────── */
void nrf_esb_event_handler(nrf_esb_evt_t const *p_event)
{
    switch (p_event->evt_id) {

        case NRF_ESB_EVENT_TX_SUCCESS:
        {
            m_fail_cnt = 0;

            /* Check for an ACK payload from the master */
            bool got_ack = (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS);

            if (m_state == STATE_DISCOVERY) {
                /* Looking for ID assignment */
                if (got_ack &&
                    rx_payload.data[0] == MARKER_ID_ASSIGN &&
                    rx_payload.data[1] > 0) {
                    m_my_id = rx_payload.data[1];
                    m_state = STATE_DATA;
                    NRF_LOG_INFO("Got ID %d – starting data.", m_my_id);
                }
                /* No ID in ACK yet – main loop will retry after back-off */

            } else { /* STATE_DATA */
                /* Check if master is kicking us */
                if (got_ack && rx_payload.data[0] == MARKER_DISASSOC) {
                    NRF_LOG_WARNING("Master kicked us – re-discovering.");
                    m_my_id = 0;
                    m_state = STATE_DISCOVERY;
                }
                /* Otherwise data was ACKed fine, keep going */
            }

            m_tx_done = true;
            break;
        }

        case NRF_ESB_EVENT_TX_FAILED:
            nrf_esb_flush_tx();
            nrf_esb_flush_rx();
            m_fail_cnt++;

            if (m_fail_cnt >= TX_FAIL_LIMIT) {
                /* Lost the master – forget our ID and re-discover */
                NRF_LOG_WARNING("Lost master after %d failures – re-discovering.", m_fail_cnt);
                m_my_id    = 0;
                m_fail_cnt = 0;
                m_state    = STATE_DISCOVERY;
            }

            m_tx_done = true;
            break;

        default:
            break;
    }
}

/* ── Helpers ─────────────────────────────────────────────────────── */
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

    nrf_esb_config_t cfg   = NRF_ESB_DEFAULT_CONFIG;
    cfg.protocol           = NRF_ESB_PROTOCOL_ESB_DPL;
    cfg.bitrate            = NRF_ESB_BITRATE_2MBPS;
    cfg.mode               = NRF_ESB_MODE_PTX;
    cfg.tx_mode            = NRF_ESB_TXMODE_AUTO;
    cfg.retransmit_count   = ESB_RETRANSMIT_COUNT;
    cfg.retransmit_delay   = ESB_RETRANSMIT_DELAY;
    cfg.event_handler      = nrf_esb_event_handler;
    cfg.selective_auto_ack = false;

    err_code = nrf_esb_init(&cfg);                       VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_base_address_0(base_addr_0);  VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_base_address_1(base_addr_1);  VERIFY_SUCCESS(err_code);
    err_code = nrf_esb_set_prefixes(addr_prefix, 8);     VERIFY_SUCCESS(err_code);

    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
    return NRF_SUCCESS;
}

static void transmit_and_wait(void)
{
    m_tx_done = false;
    if (nrf_esb_write_payload(&tx_payload) != NRF_SUCCESS) {
        NRF_LOG_ERROR("write_payload failed.");
        m_tx_done = true;
        return;
    }
    while (!m_tx_done) { __WFE(); }
}

/* ── Main ────────────────────────────────────────────────────────── */
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

    NRF_LOG_INFO("Slave started – discovering...");

    /* Initial jitter: spread slaves so they don't hit pipe 0 at the same time.
     * Each device has a unique DEVICEID so this will differ per board. */
    nrf_delay_ms(DISCOVERY_BASE_MS + (NRF_FICR->DEVICEID[0] % DISCOVERY_RAND_MS));

    while (true) {

        if (m_state == STATE_DISCOVERY) {
            /* ── Send join request on Pipe 0 ── */
            tx_payload.pipe    = PIPE_ID_ALLOC;
            tx_payload.length  = 1;
            tx_payload.noack   = false;
            tx_payload.data[0] = MARKER_JOIN_REQ;

            transmit_and_wait();

            if (m_state == STATE_DISCOVERY) {
                /* Still no ID – back off with jitter before retrying.
                 * The random component keeps two slaves from locking in sync. */
                uint32_t backoff = DISCOVERY_BASE_MS +
                                   (NRF_FICR->DEVICEID[0] % DISCOVERY_RAND_MS);
                NRF_LOG_INFO("No ID yet – waiting %d ms.", backoff);
                nrf_delay_ms(backoff);
            }

        } else {
            /* ── Send dummy data on Pipe 1 ── */
            m_pkt_cnt++;

            tx_payload.pipe    = PIPE_DATA;
            tx_payload.length  = 9;
            tx_payload.noack   = false;
            tx_payload.data[0] = m_my_id;
            tx_payload.data[1] = (uint8_t)(m_pkt_cnt >> 8);
            tx_payload.data[2] = (uint8_t)(m_pkt_cnt & 0xFF);
            tx_payload.data[3] = 0xAB;
            tx_payload.data[4] = 0xAB;
            tx_payload.data[5] = 0xAB;
            tx_payload.data[6] = 0xAB;
            tx_payload.data[7] = 0xAB;
            tx_payload.data[8] = 0xAB;

            transmit_and_wait();

            /* Only delay if we're still in data mode (not kicked back to discovery) */
            if (m_state == STATE_DATA) {
                nrf_delay_ms(DATA_INTERVAL_MS);
            }
        }

        NRF_LOG_FLUSH();
    }
}