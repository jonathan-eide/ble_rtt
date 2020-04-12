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

#ifndef TIMESLOT_H__
#define TIMESLOT_H__

#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "boards.h"
#include "rtt_parameters.h"

#define TIMESLOT_BEGIN_EGU         NRF_EGU3
#define TIMESLOT_BEGIN_IRQn        SWI3_EGU3_IRQn
#define TIMESLOT_BEGIN_IRQHandler  SWI3_EGU3_IRQHandler
#define TIMESLOT_BEGIN_IRQPriority 7
#define TIMESLOT_END_EGU           NRF_EGU4
#define TIMESLOT_END_IRQn          SWI4_EGU4_IRQn
#define TIMESLOT_END_IRQHandler    SWI4_EGU4_IRQHandler
#define TIMESLOT_END_IRQPriority   7

/**@brief Radio event handler
*/
void RADIO_timeslot_IRQHandler(void);


/**@brief Request next timeslot event in earliest configuration
 */
uint32_t request_next_event_earliest(void);


/**@brief Configure next timeslot event in earliest configuration
 */
void configure_next_event_earliest(void);


/**@brief Configure next timeslot event in normal configuration
 */
void configure_next_event_normal(void);
 
 
/**@brief Timeslot signal handler
 */
void nrf_evt_signal_handler(uint32_t evt_id);


/**@brief Timeslot event handler
 */
nrf_radio_signal_callback_return_param_t * radio_callback(uint8_t signal_type);


/**@brief Function for initializing the timeslot API.
 */
uint32_t timeslot_sd_init();

#endif
