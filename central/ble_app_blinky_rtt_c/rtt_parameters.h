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

#define DATAPIN_1 NRF_GPIO_PIN_MAP(1, 1)  /* Debug pin 1 */
#define DATAPIN_2 NRF_GPIO_PIN_MAP(1, 3)  /* Debug pin 2 */
#define DATAPIN_3 NRF_GPIO_PIN_MAP(1, 10) /* Debug pin 3 */
#define DATAPIN_4 NRF_GPIO_PIN_MAP(1, 12) /* Debug pin 4 */

/* BLE defines */
#define SCAN_INTERVAL                   0x00A0                              /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                     0x0050                              /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_DURATION                   0x0000                              /**< Timout when scanning. 0x0000 disables timeout. */

#define MIN_CONNECTION_INTERVAL         MSEC_TO_UNITS(7.5, UNIT_1_25_MS)    /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL         MSEC_TO_UNITS(30, UNIT_1_25_MS)     /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                   /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 milliseconds. */

/* Timeslot API defines */
#define TS_TOT_EXT_LENGTH_US    (1000000UL) /* Desired total timeslot length */
#define TS_LEN_US               (10000UL)   /* Initial timeslot length */
#define TS_LEN_EXTENSION_US     TS_LEN_US   /* Extension timeslot length */
#define TS_SAFETY_MARGIN_US     (250UL)     /* The timeslot activity should be finished with this much to spare. */
#define TS_EXTEND_MARGIN_US     (500UL)     /* The timeslot activity should request an extension this long before end of timeslot. */

/* RTT defines */
#define DO_RTT_END_MARGIN_US    1000UL + TS_EXTEND_MARGIN_US /* The RTT measurements should stop this long before the timeslot extend margin */
#define DO_RTT_LENGTH_US        TS_LEN_US - DO_RTT_END_MARGIN_US /* The duration of the RTT measurements */
#define CATCH_UP_DELAY_US       100
#define TIMEOUT_IT              256