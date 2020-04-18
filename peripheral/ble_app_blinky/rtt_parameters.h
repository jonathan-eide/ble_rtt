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

#define DATAPIN_1 NRF_GPIO_PIN_MAP(1, 3)  /* Debug pin 1 */
#define DATAPIN_2 NRF_GPIO_PIN_MAP(1, 1)  /* Debug pin 2 */
#define DATAPIN_3 NRF_GPIO_PIN_MAP(1, 10) /* Debug pin 3 */
#define DATAPIN_4 NRF_GPIO_PIN_MAP(1, 12) /* Debug pin 4 */

/* BLE defines */
#define APP_ADV_INTERVAL                64                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

/* Timeslot API defines */
#define TS_TOT_EXT_LENGTH_US    (1000000UL) /* Desired total timeslot length */
#define TS_LEN_US               (38000UL)   /* Initial timeslot length */
#define TS_LEN_EXTENSION_US     TS_LEN_US   /* Extension timeslot length */
#define TS_SAFETY_MARGIN_US     (250UL)     /* The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US     (500UL)     /* The timeslot activity should request an extension this long before end of timeslot. */

/* RTT defines */
#define DO_RTT_END_MARGIN_US    1000UL + TS_EXTEND_MARGIN_US /* The RTT measurements should stop this long before the timeslot extend margin */
#define DO_RTT_LENGTH_US        TS_LEN_US - DO_RTT_END_MARGIN_US /* The duration of the RTT measurements */