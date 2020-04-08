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

#define GPIO_NUMBER_LED0 13
#define GPIO_NUMBER_LED1 14
#define RX_TIMEOUT 40 /** Time to wait for response  x16us */
#define TRX_PERIOD  45 /** Time from packet sent to Tx enable  x16us */

#define DATABASE 0x20001000 /** Base address for measurement database */
#define DATA_SIZE 128 
#define NUM_BINS 128 
#define NUMBER_OF_MEASUREMENTS 10
#define ITERATIONS_DELAY 256

#define DATAPIN_4 NRF_GPIO_PIN_MAP(1, 12)

#define BLE2M

static uint8_t test_frame[255] = {0x00, 0x04, 0xFF, 0xC1, 0xFB, 0xE8};

static uint32_t tx_pkt_counter = 0;
static uint32_t radio_freq = 78;

static uint32_t timeout;
static uint32_t it_delay;
static uint32_t telp;
static uint32_t rx_pkt_counter = 0;
static uint32_t rx_pkt_counter_crcok = 0;
static uint32_t rx_timeouts = 0;
static uint32_t rx_ignored = 0;
static uint8_t rx_test_frame[256];
static bool timed_out = false;

static uint32_t database[DATA_SIZE] __attribute__((section(".ARM.__at_DATABASE")));
static uint32_t dbptr=0;
static uint32_t bincnt[NUM_BINS];

static uint32_t highper=0;
static uint32_t txcntw=0;

/**
 * @brief Initializes the radio
 */
void nrf_radio_init(void)
{
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                        (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
                    
    NRF_RADIO->TIFS = 210;
            

#if defined(BLE2M)
    NRF_RADIO->MODE = 4 << RADIO_MODE_MODE_Pos;
    
    uint32_t aa_address = 0x71764129;
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
#endif

    NRF_RADIO->FREQUENCY = (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos)  +
                         ((radio_freq << RADIO_FREQUENCY_FREQUENCY_Pos) & RADIO_FREQUENCY_FREQUENCY_Msk);

    NRF_RADIO->PACKETPTR = (uint32_t)test_frame;
    NRF_RADIO->EVENTS_DISABLED = 0;

    NRF_RADIO->TXPOWER=0x0;

    NVIC_EnableIRQ(RADIO_IRQn);
}

void timer2_capture_init(uint32_t prescaler)
{
    NRF_RADIO->POWER                = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->BITMODE = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->CC[0] = 0x0000;
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
    NRF_TIMER2->EVENTS_COMPARE[1] = 0;
    NRF_TIMER2->EVENTS_COMPARE[2] = 0;
    NRF_TIMER2->EVENTS_COMPARE[3] = 0;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->SHORTS = 0;
    NRF_TIMER2->PRESCALER = prescaler << TIMER_PRESCALER_PRESCALER_Pos;
    NRF_TIMER2->TASKS_CLEAR = 1;
}

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
 */
float calc_dist()
{
    float val = 0;
    int sum = 0;
    for(int i = 0; i < NUM_BINS; i++)
    {
        val += database[i]*(i+1);
        sum += database[i];
    }
    val = val/sum;
    //val = 0.5*18.737*val;
    return val;
}


void timeslot_status_true()
{
    timed_out = false;
}

void timeslot_status_false()
{
    timed_out = true;
}


void end_rtt()
{
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->SHORTS = 0;
    NRF_RADIO->INTENCLR = 0xFFFFFFFF;
    NRF_RADIO->EVENTS_DISABLED = 0;
    while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
    NRF_RADIO->POWER = (RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos);


    NRF_TIMER2->TASKS_STOP = 1;

    NRF_PPI->CHENCLR =  (1 << 6) | (1 << 7);
    NRF_TIMER4->TASKS_STOP  = 1;
    NRF_TIMER4->EVENTS_COMPARE[0] = 0;
}

void do_rtt_measurement(void)
{
    timed_out = false;
    uint32_t attempts,tempval, tempval1;
    int j, binNum;

    nrf_ppi_config();
    nrf_radio_init();
    /* Puts zeros into bincnt */
    memset(bincnt, 0, sizeof bincnt);

    /* Configure the timer with prescaler 0, counts every 1 cycle of timer clock (16MHz) */
    timer2_capture_init(0);

    timer4_compare_init();

    tx_pkt_counter = 0;
    attempts = 0;

    while ((attempts < NUMBER_OF_MEASUREMENTS) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
    {
        nrf_gpio_pin_set(DATAPIN_4);

        NRF_RADIO->PACKETPTR = (uint32_t) test_frame; /* Switch to tx buffer */
        NRF_RADIO->TASKS_RXEN = 0x0;
        
        /* Copy the tx packet counter into the payload */
        test_frame[2]=(tx_pkt_counter & 0x0000FF00) >> 8;
        test_frame[3]=(tx_pkt_counter & 0x000000FF);
        
        NRF_TIMER2->TASKS_STOP = 1;
        NRF_TIMER2->TASKS_CLEAR = 1;
                
        /* Start Tx */
        NRF_RADIO->TASKS_TXEN = 0x1;

        /* Wait for transmision to begin */
        while ((NRF_RADIO->EVENTS_READY == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        NRF_RADIO->EVENTS_READY = 0;
            
        /* Packet is sent */
        while ((NRF_RADIO->EVENTS_END == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        NRF_RADIO->EVENTS_END = 0;
        nrf_gpio_pin_clear(DATAPIN_4);

        /* Disable radio */
        while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        NRF_RADIO->EVENTS_DISABLED = 0;
        
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
         * Note: there is a small delay inserted on the B side to avoid race here
         */
        NRF_RADIO->TASKS_TXEN = 0x0;
        NRF_RADIO->TASKS_RXEN = 0x1;
        NRF_RADIO->PACKETPTR = (uint32_t) rx_test_frame; /* Switch to rx buffer*/
        
        /* Wait for response or timeout */
        timeout=0; 
        while ((NRF_RADIO->EVENTS_DISABLED == 0) && (timeout<2048) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
        { 
            timeout++;
        }
        
        /* Now, did we time out? */
        if(timeout>=2048)
        {
            /* Timeout, stop radio manually */
            NRF_RADIO->TASKS_STOP = 1;
            NRF_RADIO->TASKS_DISABLE = 1;
            while ((NRF_RADIO->EVENTS_DISABLED == 0) && !(NRF_TIMER4->EVENTS_COMPARE[0]))
            rx_timeouts++;
        }
        else
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
                    rx_ignored++;
                else
                {
                    /* Packet is good, update stats */
                    NRF_TIMER2->TASKS_STOP = 1;
                    telp = NRF_TIMER2->CC[0];  
                    binNum = telp - 4613; /* Magic number to trim away dwell time in device B, etc */
                    
                    if((binNum >= 0) && (binNum < NUM_BINS))
                            bincnt[binNum]++;
                    
                    dbptr++;
                    NRF_TIMER2->TASKS_CLEAR = 1;
                }
            }
        }
        attempts++;
        NRF_RADIO->EVENTS_DISABLED = 0;
    }

    dbptr = 0;
    
    /* Loading measurements in to database */
    for(j = 0; j < NUM_BINS; j++)
    {
        database[j] = bincnt[j];
        bincnt[j] = 0;
    }

    end_rtt();
}

void HardFault_Handler(void)
{
    while (true)
    {
    }
}

void MemoryManagement_Handler(void)
{
    while (true)
    {
    }
}

void BusFault_Handler(void)
{
    while (true)
    {
    }
}

void UsageFault_Handler(void)
{
    while (true)
    {
    }
}