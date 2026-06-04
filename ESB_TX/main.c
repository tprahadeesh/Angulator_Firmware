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

#define LED_PIN             NRF_GPIO_PIN_MAP(0,14)

// ==========================================
// TDMA NETWORK CONFIGURATION
// ==========================================
#define NODE_ID             3         // Unique ID for this device: MUST BE 0 to 29!
#define TOTAL_NODES         30        // Total number of TX devices in the network
#define SLOT_DURATION_US    3000      // 3 ms per slot
#define FRAME_DURATION_US   (TOTAL_NODES * SLOT_DURATION_US) // 90 ms total frame

#define MAGIC_SYNC_MARKER   0x55      // Packet ID telling Master "I need Sync"
#define MAGIC_DATA_MARKER   0xAA      // Packet ID telling Master "Normal Data"

typedef enum {
    STATE_UNSYNCED,
    STATE_SYNCHRONIZED
} tx_state_t;

static volatile tx_state_t  g_current_state = STATE_UNSYNCED;
static volatile uint32_t    g_master_time_offset_us = 0;
static volatile bool        g_ack_received = false;
static nrf_esb_payload_t    tx_payload;
static nrf_esb_payload_t    rx_ack_payload;

// 1. Initialize TIMER1 as a continuous free-running 32-bit microsecond counter
void timer_init(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER   = 4 << TIMER_PRESCALER_PRESCALER_Pos; // 1 MHz clock (1 us ticks)
    NRF_TIMER1->TASKS_START = 1; 
}

// Get current local microsecond timestamp safely
uint32_t get_local_time_us(void)
{
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER1->CC[1];
}

void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            // If the Master piggybacked a Sync ACK payload, read it immediately
            g_ack_received = true;
            break;
            
        case NRF_ESB_EVENT_TX_FAILED:
            (void) nrf_esb_flush_tx();
            break;
            
        case NRF_ESB_EVENT_RX_RECEIVED:
            // Master ACK contains data payload
            while (nrf_esb_read_rx_payload(&rx_ack_payload) == NRF_SUCCESS)
            {
                // Extract Master's time from the first 4 bytes of the ACK packet
                if (rx_ack_payload.length >= 4)
                {
                    uint32_t master_time = ((uint32_t)rx_ack_payload.data[0] << 24) |
                                           ((uint32_t)rx_ack_payload.data[1] << 16) |
                                           ((uint32_t)rx_ack_payload.data[2] << 8)  |
                                           ((uint32_t)rx_ack_payload.data[3]);
                    
                    // Synchronize timeline baseline
                    uint32_t local_now = get_local_time_us();
                    g_master_time_offset_us = master_time - local_now;
                    
                    if (g_current_state == STATE_UNSYNCED)
                    {
                        g_current_state = STATE_SYNCHRONIZED;
                        NRF_LOG_INFO("Clock LOCKED to Master! Offset: %u us", g_master_time_offset_us);
                    }
                }
            }
            break;
    }
}

void clocks_start(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

uint32_t esb_init(void)
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.retransmit_delay         = 250; 
    nrf_esb_config.retransmit_count         = 3;   // Up to 3 retries max (~1 ms max payload airtime)
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PTX;
    nrf_esb_config.selective_auto_ack       = false;

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
    
    err_code = nrf_esb_set_tx_power(NRF_ESB_TX_POWER_4DBM); 
    VERIFY_SUCCESS(err_code); 

    nrf_gpio_cfg_output(LED_PIN);
    
    tx_payload.pipe   = 0;
    tx_payload.noack  = false; // Must be false to receive master clock ticks in ACK
    tx_payload.length = 12;

    NRF_LOG_INFO("TDMA Node %d Initialized. Searching for Master clock...", NODE_ID);

    while (true)
    {
        NRF_LOG_FLUSH();

        // =================================================================
        // PHASE 1: UNSYNCHRONIZED - Aggressively request master sync
        // =================================================================
        if (g_current_state == STATE_UNSYNCED)
        {
            tx_payload.data[0] = MAGIC_SYNC_MARKER;
            tx_payload.data[1] = NODE_ID;
            // Fill remaining bytes with padding
            for(int i=2; i<12; i++) tx_payload.data[i] = 0;

            g_ack_received = false;
            (void) nrf_esb_flush_tx();
            
            if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS)
            {
                // Rapidly cycle attempts every 10ms until Master ACKs back
                nrf_delay_us(10000); 
            }
            continue; // Keep looping here until state becomes STATE_SYNCHRONIZED
        }

        // =================================================================
        // PHASE 2: SYNCHRONIZED TDMA STEADY-STATE
        // =================================================================
        
        // 1. Calculate running Global Master Time
        uint32_t current_master_time = get_local_time_us() + g_master_time_offset_us;
        
        // 2. Determine where we are inside the current 90ms frame window
        uint32_t frame_position = current_master_time % FRAME_DURATION_US;
        
        // 3. Calculate exactly when this specific node's 3ms window opens
        uint32_t target_start_window = NODE_ID * SLOT_DURATION_US;
        
        // 4. Time tracking variables to determine scheduling
        uint32_t time_to_wait_us = 0;

        if (frame_position < target_start_window)
        {
            // Our slot is coming up later in the current frame
            time_to_wait_us = target_start_window - frame_position;
        }
        else
        {
            // Our slot already passed in this frame, wait for the next frame's loop
            time_to_wait_us = (FRAME_DURATION_US - frame_position) + target_start_window;
        }

        // 5. Sleep/Delay until exactly 50 microseconds before our window opens 
        // (Leaving 50us overhead for API execution delays)
        if (time_to_wait_us > 50)
        {
            nrf_delay_us(time_to_wait_us - 50);
        }

        // =================================================================
        // TRANSMISSION SLOT OPEN - FIRE PACKET
        // =================================================================
        tx_payload.data[0] = MAGIC_DATA_MARKER;
        tx_payload.data[1] = NODE_ID;
        
        // Load actual sensor/dummy data into bytes 2-11
        for (uint8_t i = 2; i < 12; i++)
        {
            tx_payload.data[i] = 0xBB; 
        }

        g_ack_received = false;
        (void) nrf_esb_flush_tx();

        if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS)
        {
            nrf_gpio_pin_toggle(LED_PIN);
        }

        // Enforce the remaining 3ms slot cushion to prevent accidental double-bursting 
        // before looping back to calculate the next frame.
        nrf_delay_us(SLOT_DURATION_US);
    }
}