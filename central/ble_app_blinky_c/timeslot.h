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
#define TS_LEN_US               (20000UL)   /* Initial timeslot length */
#define TS_LEN_EXTENSION_US     (20000UL)   /* Extension timeslot length */
#define TS_SAFETY_MARGIN_US     (250UL)     /* The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US     (500UL)     /* The timeslot activity should request an extension this long before end of timeslot. */

#define TS_CALLBACK_EGU             NRF_EGU3
#define TS_CALLBACK_EGU_IRQn        SWI3_EGU3_IRQn
#define TS_CALLBACK_EGU_IRQHandler  SWI3_EGU3_IRQHandler
#define TS_EGU_CALLBACK_START_INDEX 0
#define TS_EGU_CALLBACK_END_INDEX   1

typedef void (*ts_started_callback)(void);
typedef void (*ts_stopped_callback)(void);

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
uint32_t timeslot_sd_init(ts_started_callback started_cb, ts_stopped_callback stopped_cb);


#endif
