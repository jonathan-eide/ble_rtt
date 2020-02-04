#ifndef TIMESLOT_H__
#define TIMESLOT_H__

#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "boards.h"

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
uint32_t timeslot_sd_init(void);


#endif
