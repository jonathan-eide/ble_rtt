#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "boards.h"

/* Constants for timeslot API */
static nrf_radio_request_t  m_timeslot_request;
static uint32_t             m_slot_length;
static uint32_t             m_total_timeslot_length = 0;

static nrf_radio_signal_callback_return_param_t signal_callback_return_param;

/**@brief Request next timeslot event in earliest configuration
 */
uint32_t request_next_event_earliest(void)
{
    m_slot_length                                  = TS_LEN_US;
    m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_HIGH;
    m_timeslot_request.params.earliest.length_us   = m_slot_length;
    m_timeslot_request.params.earliest.timeout_us  = 1000000;
    return sd_radio_request(&m_timeslot_request);
}


/**@brief Configure next timeslot event in earliest configuration
 */
void configure_next_event_earliest(void)
{
    m_slot_length                                  = TX_LEN_EXTENSION_US;
    m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_HIGH;
    m_timeslot_request.params.earliest.length_us   = m_slot_length;
    m_timeslot_request.params.earliest.timeout_us  = 1000000;
}

/**@brief Timeslot signal handler
 */
void nrf_evt_signal_handler(uint32_t evt_id)
{
    uint32_t err_code;
    
    switch (evt_id)
    {
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            //No implementation needed
            break;
        case NRF_EVT_RADIO_SESSION_IDLE:
            //No implementation needed
            break;
        case NRF_EVT_RADIO_SESSION_CLOSED:
            //No implementation needed, session ended
            break;
        case NRF_EVT_RADIO_BLOCKED:
            //Fall through
        case NRF_EVT_RADIO_CANCELED:
            err_code = request_next_event_earliest();
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}

/**@brief Timeslot event handler
 */
nrf_radio_signal_callback_return_param_t * radio_callback(uint8_t signal_type)
{
    switch(signal_type)
    {
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
            // TIMER0 is pre-configured for 1Mhz.
            NRF_TIMER0->TASKS_STOP          = 1;
            NRF_TIMER0->TASKS_CLEAR         = 1;
            NRF_TIMER0->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
            NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
            NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
    
            NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos) | 
                                   (TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos);

            NRF_TIMER0->CC[0]               = (TS_LEN_US - TS_SAFETY_MARGIN_US);
            NRF_TIMER0->CC[1]               = (TS_LEN_US - TS_EXTEND_MARGIN_US);
            NRF_TIMER0->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
            NRF_TIMER0->TASKS_START         = 1;
    
            NVIC_EnableIRQ(TIMER0_IRQn);
        
            m_total_timeslot_length = 0;
            
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
            // Don't do anything
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
            if (NRF_TIMER0->EVENTS_COMPARE[0] &&
               (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENCLR_COMPARE0_Pos)))
            {
                bsp_board_led_off(2);
                NRF_TIMER0->TASKS_STOP  = 1;
                NRF_TIMER0->EVENTS_COMPARE[0] = 0;
                (void)NRF_TIMER0->EVENTS_COMPARE[0];
            
                // Schedule next timeslot
                signal_callback_return_param.params.request.p_next = &m_timeslot_request;
                signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
            }
            else if (NRF_TIMER0->EVENTS_COMPARE[1] &&
               (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENCLR_COMPARE1_Pos)))
            {
                NRF_TIMER0->EVENTS_COMPARE[1] = 0;
                (void)NRF_TIMER0->EVENTS_COMPARE[1];
            
                // This is the "try to extend timeslot" timeout
                if (m_total_timeslot_length < (TS_TOT_EXT_LENGTH_US - 5000UL - TX_LEN_EXTENSION_US))
                {
                    // Request timeslot extension if total length does not exceed TS_TOT_EXT_LENGTH_US
                    signal_callback_return_param.params.extend.length_us = TX_LEN_EXTENSION_US;
                    signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
                }
            }
            else
            {
                signal_callback_return_param.params.request.p_next = NULL;
                signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            }
            
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
            bsp_board_led_on(2);

            // Extension succeeded: update timer
            NRF_TIMER0->TASKS_STOP          = 1;
            NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
            NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
            NRF_TIMER0->CC[0]               += (TX_LEN_EXTENSION_US - 25);
            NRF_TIMER0->CC[1]               += (TX_LEN_EXTENSION_US - 25);
            NRF_TIMER0->TASKS_START         = 1;
    
            // Keep track of total length
            m_total_timeslot_length += TX_LEN_EXTENSION_US;
            
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
            // Don't do anything. The timer will expire before timeslot ends.
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;
        default:
            //No implementation needed
            break;
    }
    return (&signal_callback_return_param);
}


/**@brief Function for initializing the timeslot API.
 */
uint32_t timeslot_sd_init(void)
{
    uint32_t err_code;
    
    err_code = sd_radio_session_open(&radio_callback);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = request_next_event_earliest();
    if (err_code != NRF_SUCCESS)
    {
        (void)sd_radio_session_close();
        return err_code;
    }
    return NRF_SUCCESS;
}