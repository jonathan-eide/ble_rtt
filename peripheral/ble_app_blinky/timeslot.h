#ifndef TIMESLOT_H__
#define TIMESLOT_H__

#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "boards.h"

#define TS_TOT_EXT_LENGTH_US    (1000000UL) /* Desired total timeslot length */
#define TS_LEN_US               (40000UL)   /* Initial timeslot length */
#define TS_LEN_EXTENSION_US     (40000UL)   /* Extension timeslot length */
#define TS_SAFETY_MARGIN_US     (250UL)     /* The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US     (500UL)     /* The timeslot activity should request an extension this long before end of timeslot. */

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
