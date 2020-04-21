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

#include "radio_001.h"
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
#include "nrf_clock.h"
#include "rtt_parameters.h"

#define GPIO_NUMBER_LED0       13 /* Pin number for LED0 */
#define GPIO_NUMBER_LED1       14 /* Pin number for LED1 */
#define DATABASE               0x20001000 /* Base address for measurement database */
#define NUM_BINS               128 /* Number of bins in database */
#define TIMER2_PRESCALE_VAL    0 /* 16 MHz */
#define OFFSET                 69.96 /* Offset found by linear regression */

static uint8_t  test_frame[255] = {0x00, 0x04, 0xFF, 0xC1, 0xFB, 0xE8};
static uint32_t tx_pkt_counter = 0;
static uint32_t radio_freq = 78;
static uint32_t telp;
static uint32_t rx_pkt_counter = 0;
static uint32_t rx_pkt_counter_crcok = 0;
static uint32_t rx_timeouts = 0;
static uint32_t rx_ignored = 0;
static uint8_t  rx_test_frame[256];
static uint32_t highper=0;
static uint32_t txcntw=0;
static uint32_t database[NUM_BINS] __attribute__((section(".ARM.__at_DATABASE")));
static uint32_t dbptr=0;
static uint32_t bincnt[NUM_BINS];

/**
 * @brief Initializes the radio
 */
void nrf_radio_init(void)
{
    uint32_t aa_address = 0x71764129;
    NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                        (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
    NRF_RADIO->TIFS = 210;
    NRF_RADIO->MODE = 4 << RADIO_MODE_MODE_Pos;
    NRF_RADIO->BASE0 = aa_address << 8;
    NRF_RADIO->PREFIX0 = (0xffffff00 | aa_address >> 24);        
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->RXADDRESSES = 1;
    NRF_RADIO->DATAWHITEIV = 39;        
    NRF_RADIO->PCNF0 = 0x01000108;
    NRF_RADIO->PCNF1 = 0x000300FF; /* sw:turn off whitening */
    NRF_RADIO->CRCPOLY = 0x65B;
    NRF_RADIO->CRCINIT = 0x555555;
    NRF_RADIO->CRCCNF = 0x103;
    NRF_RADIO->FREQUENCY = (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos)  +
                         ((radio_freq << RADIO_FREQUENCY_FREQUENCY_Pos) & RADIO_FREQUENCY_FREQUENCY_Msk);
    NRF_RADIO->PACKETPTR = (uint32_t)test_frame;
    NRF_RADIO->TXPOWER=0x0;
}

/**
 * @brief Initializing TIMER2 for measuring RTT.
 * 
 * @param[in] Prescaler
 */
void timer2_capture_init(uint32_t prescaler)
{
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->BITMODE = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->CC[0] = 0x0000;
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->SHORTS = 0;
    NRF_TIMER2->PRESCALER = prescaler << TIMER_PRESCALER_PRESCALER_Pos;
    NRF_TIMER2->TASKS_CLEAR = 1;
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
    NRF_TIMER4->CC[0]               = (DO_RTT_LENGTH_US);
    NRF_TIMER4->BITMODE             = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
    NRF_TIMER4->PRESCALER           = 4;
    NRF_TIMER4->TASKS_START         = 1;
}

/**
 * @brief Setting up ppi for capturing time of flight
 */
void nrf_ppi_config (void)
{
    NRF_PPI->CH[6].TEP = (uint32_t)(&NRF_TIMER2->TASKS_CAPTURE[0]);
    NRF_PPI->CH[6].EEP = (uint32_t)(&NRF_RADIO->EVENTS_ADDRESS); 

    NRF_PPI->CH[7].TEP = (uint32_t)(&NRF_TIMER2->TASKS_START);
    NRF_PPI->CH[7].EEP = (uint32_t)(&NRF_RADIO->EVENTS_ADDRESS);
 
    NRF_PPI->CHENSET =  (1 << 6) | (1 << 7);
}

/**
 * @brief Calculates and returns distance in meters
 * 
 * @return Distance [m]
 * 
 * Must be called after do_rtt_measurement.
 */
float calc_dist(void)
{
    float val = 0;
    int sum = 0;
    for(int i = 0; i < NUM_BINS; i++)
    {
        val += database[i]*(i+1);
        sum += database[i];
    }
    val = val/sum;
    val = 0.5*18.737*val - OFFSET;
    return val;
}

/**
 * @brief Disables the radio, PPI and timers
 */
void end_rtt()
{
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->SHORTS = 0;
    NRF_RADIO->INTENCLR = 0xFFFFFFFF;
    NRF_RADIO->EVENTS_DISABLED = 0;
    while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
    NRF_RADIO->POWER = (RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos);

    NRF_PPI->CHENCLR =  (1 << 6) | (1 << 7);

    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER4->TASKS_STOP  = 1;
    NRF_TIMER4->EVENTS_COMPARE[0] = 0;
}


/**
 * @brief Do RTT measurements
 */
void do_rtt_measurement(void)
{
    uint32_t attempts,tempval, tempval1;
    int j, binNum;

    tx_pkt_counter = 0;
    attempts = 0;

    /* Configure PPI */
    nrf_ppi_config();

    /* Initialize the radio */
    nrf_radio_init();

    /* Configure the timers */
    timer2_capture_init(TIMER2_PRESCALE_VAL);
    timer4_compare_init();

    /* Puts zeros into bincnt */
    memset(bincnt, 0, sizeof bincnt);

    /* Wait to make sure radio_002 is ready */
    nrf_delay_us(CATCH_UP_DELAY_US);

    while (!(NRF_TIMER4->EVENTS_COMPARE[0]))
    {
        nrf_gpio_pin_set(DATAPIN_4);

        NRF_RADIO->PACKETPTR = (uint32_t) test_frame; /* Switch to tx buffer */

        /* Copy the tx packet counter into the payload */
        test_frame[2]=(tx_pkt_counter & 0x0000FF00) >> 8;
        test_frame[3]=(tx_pkt_counter & 0x000000FF);
        
        NRF_TIMER2->TASKS_STOP = 1;
        NRF_TIMER2->TASKS_CLEAR = 1;

        /* Start Tx */
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_TXEN = 0x1;

        /* Wait for transmision to begin */
        while ((NRF_RADIO->EVENTS_READY == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->TASKS_START = 1U;

        /* Packet is sent */
        while ((NRF_RADIO->EVENTS_END == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        tx_pkt_counter++;
        txcntw++;
        
        if(txcntw > 50)
        {
            txcntw = 0;
            if(rx_timeouts > 10)
                // PER>=20pct
                highper = 1;
            else
                highper=0;
            
            rx_timeouts = 0;
        }

        /** 
         * Packet sent, switch to Rx asap 
         */
        NRF_RADIO->PACKETPTR = (uint32_t) rx_test_frame; /* Switch to rx buffer*/
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_TXEN = 0x0;
        NRF_RADIO->TASKS_RXEN = 0x1;

        /* Wait for packet */
        while ((NRF_RADIO->EVENTS_READY == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }
        NRF_RADIO->EVENTS_END = 0U;

        /* Start listening and wait for end event */
        NRF_RADIO->TASKS_START = 1U;
        while ((NRF_RADIO->EVENTS_END == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        if(!(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
            rx_pkt_counter++;
            if(NRF_RADIO->CRCSTATUS>0)
            {
                /**
                 * Process the received packet
                 * Check the sequence number in the received packet against our tx packet counter
                 */
                rx_pkt_counter_crcok++;
                tempval = ((rx_test_frame[2] << 8) + (rx_test_frame[3]));
                tempval1 = tx_pkt_counter-1;
                if(tempval != (tempval1&0x0000FFFF))
                {
                    rx_ignored++;
                }
                else
                {
                    /* Packet is good, update stats */
                    NRF_TIMER2->TASKS_STOP = 1;
                    telp = NRF_TIMER2->CC[0];  
                    binNum = telp - 4150; /* Magic number to trim away dwell time in device B, etc */
                    
                    if((binNum >= 0) && (binNum < NUM_BINS))
                            bincnt[binNum]++;
                    
                    dbptr++;
                    NRF_TIMER2->TASKS_CLEAR = 1;
                }
            }
        }

        attempts++;

        NRF_RADIO->EVENTS_DISABLED = 0U;
        NRF_RADIO->TASKS_DISABLE = 1U;

        while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        {
        }

        nrf_gpio_pin_clear(DATAPIN_4);
    }

    dbptr = 0;
    end_rtt();

    /* Loading measurements in to database */
    for(j = 0; j < NUM_BINS; j++)
    {
        database[j] = bincnt[j];
        bincnt[j] = 0;
    }

    /* Print the result. I suggest averaging more estimations before printing. */
    // NRF_LOG_INFO(NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(calc_dist()));
}