/**
 * MIT License
 * 
 * Copyright (c) 2019 Martin Aalien
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

#include <stdbool.h>
#include "radio_002.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <stdbool.h>
#include <stdio.h> 
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include <stdlib.h>

#define DATAPIN_4 NRF_GPIO_PIN_MAP(1, 12)
#define NRF_GPIO NRF_P0

#define NUMBER_OF_MEASUREMENTS 10

static uint32_t radio_freq = 78;
static uint32_t attempts = 0;
static uint8_t test_frame[256];
static uint32_t rx_pkt_counter = 0;
static uint32_t rx_pkt_counter_crcok = 0;
static uint32_t dbgcnt1=0;
static uint32_t tx_pkt_counter = 0;

static uint8_t response_test_frame[255] = 
    {0x00, 0x04, 0xFF, 0xC1, 0xFB, 0xE8};

/**
 * @brief Initializing the radio
 */
void nrf_radio_init(void)
{
    uint32_t aa_address = 0x71764129;
    NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);

    NRF_RADIO->MODE = 4 << RADIO_MODE_MODE_Pos; /* Radio in BLe 1M */
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                        (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos) |
                        (RADIO_SHORTS_DISABLED_TXEN_Enabled << RADIO_SHORTS_DISABLED_TXEN_Pos);	
    NRF_RADIO->PCNF0 = 0x01000108;
    NRF_RADIO->PCNF1 = 0x000300FF;
    NRF_RADIO->CRCPOLY = 0x65B;
    NRF_RADIO->CRCINIT = 0x555555;
    NRF_RADIO->CRCCNF = 0x103;
    NRF_RADIO->FREQUENCY = (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos)  +
                            ((radio_freq << RADIO_FREQUENCY_FREQUENCY_Pos) & RADIO_FREQUENCY_FREQUENCY_Msk);
    NRF_RADIO->PACKETPTR = (uint32_t)test_frame;
    NRF_RADIO->BASE0 = aa_address << 8;
    NRF_RADIO->PREFIX0 = (0xffffff00 | aa_address >> 24);
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->RXADDRESSES = 1;
    NRF_RADIO->MODECNF0= NRF_RADIO->MODECNF0 | 0x1F1F0000;
    NRF_RADIO->TIFS = 0x000000C0;
    NRF_RADIO->TXPOWER=0x0;
}

/**
 * @brief Initializing TIMER4 to keep track of when the timeslot is about to end.
 */
void timer4_compare_init()
{
    NRF_TIMER4->TASKS_STOP          = 1;
    NRF_TIMER4->TASKS_CLEAR         = 1;
    NRF_TIMER4->MODE                = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
    NRF_TIMER4->EVENTS_COMPARE[0]   = 0;
    NRF_TIMER4->CC[0]               = (35000UL);
    NRF_TIMER4->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
    NRF_TIMER4->PRESCALER           = 4;
    NRF_TIMER4->TASKS_START         = 1;
}

/**
 * @brief Disables the radio and timer
 */
void end_rtt()
{
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->SHORTS = 0;
    NRF_RADIO->INTENCLR = 0xFFFFFFFF;
    NRF_RADIO->EVENTS_DISABLED = 0;
    while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
    NRF_RADIO->POWER = (RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos);

    NRF_TIMER4->TASKS_STOP  = 1;
    NRF_TIMER4->EVENTS_COMPARE[0] = 0;
}

/**
 * @brief Do RTT measurements
 */
void do_rtt_measurement(void)
{
    volatile  uint32_t i;

    attempts = 0;

    /* Initializinf the radio for RTT */
    nrf_radio_init();

    /* Configure the timer */
    timer4_compare_init();

    while (!(NRF_TIMER4->EVENTS_COMPARE[0]))
    {
        attempts++;

        nrf_gpio_pin_set(DATAPIN_4);

        NRF_RADIO->PACKETPTR = (uint32_t)test_frame; /* Switch to rx buffer */

        // Enable radio and wait for ready
        NRF_RADIO->EVENTS_READY = 0U;
        NRF_RADIO->TASKS_TXEN = 0;
        NRF_RADIO->TASKS_RXEN = 1U;

        /* Wait for packet */
        while ((NRF_RADIO->EVENTS_READY == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        NRF_RADIO->EVENTS_END = 0U;

        /* Start listening and wait for address received event */
        NRF_RADIO->TASKS_START = 1U;
        while ((NRF_RADIO->EVENTS_END == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        /* Packet received, check CRC */
        rx_pkt_counter++;

        if(NRF_RADIO->CRCSTATUS>0)
        {
            /* CRC ok */
            rx_pkt_counter_crcok++;
            
            for(i=2;i<4;i++)
                response_test_frame[i]=test_frame[i];
        }
        else
        {
            /* CRC error */
            dbgcnt1++;

            /* Insert zeros as sequence number into the response packet indicating crc error to initiator */
            for(i=2;i<4;i++)
                response_test_frame[i]=0;
        }

        /* Switch to Tx asap and send response packet back to initiator */
        NRF_RADIO->PACKETPTR = (uint32_t)response_test_frame; /* Switch to tx buffer */
        NRF_RADIO->TASKS_RXEN = 0x0;
        NRF_RADIO->EVENTS_READY = 0U;
        NRF_RADIO->TASKS_TXEN   = 1;

        /* Wait for radio to be ready to transmit */
        while ((NRF_RADIO->EVENTS_READY == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        NRF_RADIO->EVENTS_END  = 0U;
        NRF_RADIO->TASKS_START = 1U;

        /* Remove short DISABLED->TXEN before Tx ends */
        NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                            (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);

        /* Wait for packet to be transmitted */
        while ((NRF_RADIO->EVENTS_END == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        // Disable radio
        NRF_RADIO->EVENTS_DISABLED = 0U;
        NRF_RADIO->TASKS_DISABLE = 1U;

        while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos) |
                (RADIO_SHORTS_DISABLED_TXEN_Enabled << RADIO_SHORTS_DISABLED_TXEN_Pos);

        tx_pkt_counter++;

        nrf_gpio_pin_clear(DATAPIN_4);
        }

    end_rtt();
}