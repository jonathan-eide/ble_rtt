/**
 * MIT License
 * 
 * Copyright (c) 2020 Martin Aalien
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "boards.h"
#include "timeslot.h"
#include "nrf_error.h"
#include "nrf_sdm.h"
#include "radio_001.h"
#include "rtt_parameters.h"

#define LED3 2
#define LED4 3

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
    m_timeslot_request.params.earliest.timeout_us  = NRF_RADIO_EARLIEST_TIMEOUT_MAX_US;
    return sd_radio_request(&m_timeslot_request);
}


/**@brief Configure next timeslot event in earliest configuration
 */
void configure_next_event_earliest(void)
{
    m_slot_length                                  = TS_LEN_EXTENSION_US;
    m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_HIGH;
    m_timeslot_request.params.earliest.length_us   = m_slot_length;
    m_timeslot_request.params.earliest.timeout_us  = NRF_RADIO_EARLIEST_TIMEOUT_MAX_US;
}

/**@brief Timeslot signal handler
 */
void nrf_evt_signal_handler(uint32_t evt_id)
{
    uint32_t err_code;
    
    switch (evt_id)
    {
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            /* No implementation needed */
            break;
        case NRF_EVT_RADIO_SESSION_IDLE:
            /* No implementation needed */
            break;
        case NRF_EVT_RADIO_SESSION_CLOSED:
            /* No implementation needed, session ended */
            break;
        case NRF_EVT_RADIO_BLOCKED:
            /* Fall through */
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
            nrf_gpio_pin_set(DATAPIN_1);

            /* TIMER0 is pre-configured for 1Mhz. */
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
            /* Don't do anything*/
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
            if (NRF_TIMER0->EVENTS_COMPARE[0] &&
               (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENCLR_COMPARE0_Pos)))
            {
                /* End margin reached. End current timeslot and request new one. */
                m_timeslot_request.params.earliest.length_us   = TS_LEN_US;
                m_slot_length                                  = TS_LEN_US;

                NRF_TIMER0->TASKS_STOP  = 1;
                NRF_TIMER0->EVENTS_COMPARE[0] = 0;
                (void)NRF_TIMER0->EVENTS_COMPARE[0];

                /* Schedule next timeslot */
                signal_callback_return_param.params.request.p_next = &m_timeslot_request;
                signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;

                TIMESLOT_END_EGU->TASKS_TRIGGER[0] = 1;

                nrf_gpio_pin_clear(DATAPIN_1);
                bsp_board_led_off(LED3);
            }
            else if (NRF_TIMER0->EVENTS_COMPARE[1] &&
               (NRF_TIMER0->INTENSET & (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENCLR_COMPARE1_Pos)))
            {
                /* Extend margin reached. Request extension. */
                m_timeslot_request.params.earliest.length_us   = TS_LEN_EXTENSION_US;
                m_slot_length                                  = TS_LEN_EXTENSION_US;

                configure_next_event_earliest();

                NRF_TIMER0->EVENTS_COMPARE[1] = 0;
                (void)NRF_TIMER0->EVENTS_COMPARE[1];
            
                /* This is the "try to extend timeslot" timeout */
                if (m_total_timeslot_length < (TS_TOT_EXT_LENGTH_US - 5000UL - TS_LEN_EXTENSION_US))
                {
                    /* Request timeslot extension if total length does not exceed TS_TOT_EXT_LENGTH_US */
                    signal_callback_return_param.params.extend.length_us = TS_LEN_EXTENSION_US;
                    signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_EXTEND;
                }

                nrf_gpio_pin_set(DATAPIN_2);
                nrf_gpio_pin_set(DATAPIN_3);
            }
            else
            {
                /* Don't do anything */
                signal_callback_return_param.params.request.p_next = NULL;
                signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            }
            
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
            

            /* Extension succeeded: update timer */
            NRF_TIMER0->TASKS_STOP          = 1;
            NRF_TIMER0->EVENTS_COMPARE[0]   = 0;
            NRF_TIMER0->EVENTS_COMPARE[1]   = 0;
            NRF_TIMER0->CC[0]               += (TS_LEN_EXTENSION_US - 25);
            NRF_TIMER0->CC[1]               += (TS_LEN_EXTENSION_US - 25);
            NRF_TIMER0->TASKS_START         = 1;
    
            /* Keep track of total length */
            m_total_timeslot_length += TS_LEN_EXTENSION_US;
            
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;

            nrf_gpio_pin_clear(DATAPIN_3);
            bsp_board_led_on(LED3);

            TIMESLOT_BEGIN_EGU->TASKS_TRIGGER[0] = 1;
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
            /* Don't do anything. The timer will expire before timeslot ends. */
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;

            nrf_gpio_pin_clear(DATAPIN_2);
            break;
        default:
            /* No implementation needed */
            break;
    }
    return (&signal_callback_return_param);
}


/**@brief Function for initializing the timeslot API.
 */
uint32_t timeslot_sd_init()
{
    uint32_t err_code;

    TIMESLOT_BEGIN_EGU->INTENSET = (1 << 0);
    TIMESLOT_END_EGU->INTENSET = (1 << 0);

    NVIC_SetPriority(TIMESLOT_BEGIN_IRQn, TIMESLOT_BEGIN_IRQPriority);
    NVIC_SetPriority(TIMESLOT_END_IRQn, TIMESLOT_END_IRQPriority);
    NVIC_EnableIRQ(TIMESLOT_BEGIN_IRQn);
    NVIC_EnableIRQ(TIMESLOT_END_IRQn);
    
    /* Open a session for radio timeslot requests */
    err_code = sd_radio_session_open(radio_callback);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    /* Request a timeslot */
    err_code = request_next_event_earliest();
    if (err_code != NRF_SUCCESS)
    {
        (void)sd_radio_session_close();
        return err_code;
    }

    return NRF_SUCCESS;
}

/**
 * TIMESLOT_BEGIN SWI handler.
 */
void TIMESLOT_BEGIN_IRQHandler(void)
{
    TIMESLOT_BEGIN_EGU->EVENTS_TRIGGERED[0] = 0;
    bsp_board_led_on(LED4);
    do_rtt_measurement();
}

/**
 * TIMESLOT_END SWI handler.
 */
void TIMESLOT_END_IRQHandler(void)
{
    TIMESLOT_END_EGU->EVENTS_TRIGGERED[0] = 0;
    bsp_board_led_off(LED4);
}