/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//#include "bsp/board.h"
#include "hardware/timer.h"
#include "tusb.h"
#include "usbmain.h"

//------------- prototypes -------------//
void cdc_task(void);

/*------------- MAIN -------------*/
int usb_init(void)
{
  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  return 0;
}

static uint usb_did_write = 0;
static uint32_t usb_write_last_flush = 0;


void usb_write_char(uint8_t ch)
{
    while ((tud_cdc_n_connected(0)) && (!tud_cdc_n_write_available(0))) 
        usb_task();
    if (tud_cdc_n_connected(0))
    {
        
        tud_cdc_n_write_char(0, ch);
        if (tud_cdc_n_write_available(0) == 0)
        {
            tud_cdc_n_write_flush(0);
            usb_write_last_flush = time_us_32();
            usb_did_write = 0;
        } else usb_did_write = 1;
    }
}

int usb_read_character(void)
{
    if (!tud_cdc_n_connected(0)) return -1;
    return tud_cdc_n_read_char(0);
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
    if (!tud_cdc_n_connected(0))
        return;
    if (usb_did_write)
    {
        uint32_t microtime = time_us_32();
        if ((microtime - usb_write_last_flush) > 1000u)
        {
            tud_cdc_n_write_flush(0);
            usb_write_last_flush = microtime;
            usb_did_write = 0;
        }
    }      
}

void midi_send_note(uint8_t note, uint8_t velocity)
{
    static uint8_t last_note = 0;

    uint8_t msg[3];

    if (note > 0)
    {
        msg[0] = 0x90;                    // Note On - Channel 1
        msg[1] = note;                    // Note Number
        msg[2] = velocity;                // Velocity
        tud_midi_n_stream_write(0, 0, msg, 3);
    }
    if (last_note > 0)
    {
        msg[0] = 0x80;                    // Note Off - Channel 1
        msg[1] = last_note;               // Note Number
        msg[2] = 0;                       // Velocity
        tud_midi_n_stream_write(0, 0, msg, 3);
    }
    last_note = note;
}

void midi_task(void)
{
}

void usb_task(void)
{
    tud_task();
    cdc_task();
    //midi_task();
}

