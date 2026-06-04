#include "nrf_esb.h"
#include <stdbool.h>
#include <stdint.h>
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

#define TOTAL_NODES         30
#define SLOT_DURATION_US    3000
#define FRAME_DURATION_US   (TOTAL_NODES * SLOT_DURATION_US) // 90,000 us (90 ms)

#define MAGIC_SYNC_MARKER   0x55      
#define MAGIC_DATA_MARKER   0xAA      

nrf_esb_payload_t rx_payload;
nrf_esb_payload_t tx_ack_payload;

// Array to track if a node sent data during the current 90ms frame
static volatile bool g_slot_received_flags[TOTAL_NODES] = {false};
static volatile uint32_t g_last_frame_boundary_us = 0;

void timer_init(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER   = 4 << TIMER_PRESCALER_PRESCALER_Pos; // 1 MHz (1 us ticks)
    NRF_TIMER1->TASKS_START = 1; 
}

uint32_t get_master_time_us(void)
{
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER1->CC[1];
}

// Function to print the status of all 30 slots at the end of a frame
void print_tdma_frame_status(void)
{
    NRF_LOG_INFO("=================== TDMA FRAME REPORT ===================");
    for (uint8_t i = 0; i < TOTAL_NODES; i++)
    {
        if (g_slot_received_flags[i])
        {
            NRF_LOG_INFO("Slot %02d (Node %02d): [ DATA RECEIVED ]", i + 1, i);
        }
        else
        {
            NRF_LOG_INFO("Slot %02d (Node %02d): [ --- NO DATA --- ]", i + 1, i);
        }
        
        // Reset the flag for the next upcoming frame cycle
        g_slot_received_flags[i] = false;
    }
    NRF_LOG_INFO("=========================================================");
}

void nrf_esb_event_handler(nrf_esb_evt_t const * p_event) 
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_RX_RECEIVED:
            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) 
            {
                uint32_t current_master_time = get_master_time_us();
                uint8_t marker  = rx_payload.data[0];
                uint8_t node_id = rx_payload.data[1];

                // Check if a brand new 90ms network frame has started since our last print
                if (current_master_time - g_last_frame_boundary_us >= FRAME_DURATION_US)
                {
                    print_tdma_frame_status();
                    // Lock the new frame's starting time anchor
                    g_last_frame_boundary_us = current_master_time - (current_master_time % FRAME_DURATION_US);
                }

                // If it is a valid data packet from an assigned node, mark it as received
                if (marker == MAGIC_DATA_MARKER && node_id < TOTAL_NODES) 
                {
                    g_slot_received_flags[node_id] = true;
                }
                else if (marker == MAGIC_SYNC_MARKER)
                {
                    NRF_LOG_DEBUG("Sync Request from Node %d", node_id);
                }

                // PRE-LOAD ACK PAYLOAD WITH CURRENT MASTER CLOCK
                tx_ack_payload.pipe   = rx_payload.pipe;
                tx_ack_payload.length = 4; 
                tx_ack_payload.data[0] = (uint8_t)(current_master_time >> 24);
                tx_ack_payload.data[1] = (uint8_t)(current_master_time >> 16);
                tx_ack_payload.data[2] = (uint8_t)(current_master_time >> 8);
                tx_ack_payload.data[3] = (uint8_t)(current_master_time & 0xFF);

                // Use unified write_payload function for PRX ACK mode
                (void) nrf_esb_write_payload(&tx_ack_payload);
            }
            break;

        case NRF_ESB_EVENT_TX_FAILED:
            (void) nrf_esb_flush_tx();
            break;
            
        default:
            break;
    }
}

void clocks_start( void )
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

void gpio_init( void )
{
    bsp_board_init(BSP_INIT_LEDS);
}

uint32_t esb_init( void )
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };
    
    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
    nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PRX; 
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
    nrf_esb_config.selective_auto_ack       = false; 

    err_code = nrf_esb_init(&nrf_esb_config);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

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

    NRF_LOG_INFO("TDMA Central RX Master Hub Online with Diagnostics.");

    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);

    g_last_frame_boundary_us = get_master_time_us();

    while (true)
    {
        // Force evaluation of logs to prevent delayed outputs
        NRF_LOG_FLUSH();
        
        // If a node missed the frame interrupt boundary trigger during quiet times,
        // handle fallback printing here in the main thread thread safely.
        if (get_master_time_us() - g_last_frame_boundary_us >= FRAME_DURATION_US)
        {
            print_tdma_frame_status();
            g_last_frame_boundary_us = get_master_time_us();
        }

        if (NRF_LOG_PROCESS() == false)
        { 
            __WFE(); 
        }
    }
}