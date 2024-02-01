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
  //board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

/*
  while (1)
  {
    tud_task(); // tinyusb device task
    cdc_task();
  }
  */

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

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

// Variable that holds the current position in the sequence.
uint32_t note_pos = 0;

// Store example melody as an array of note values
uint8_t note_sequence[] =
{
  74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
  74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
  56,61,64,68,74,78,81,86,90,93,98,102
};

void midi_send_note(uint8_t note, uint8_t velocity)
{
    static uint8_t last_note = 0;

    uint8_t msg[3];

//    if (!tud_midi_n_mounted(2)) return;
    
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
  static uint32_t start_ms = 0;
  uint8_t msg[3];

  uint32_t bm = to_ms_since_boot(get_absolute_time());
  // send note every 1000 ms
  if (bm - start_ms < 286) return; // not enough time
  start_ms += 286;

  // Previous positions in the note sequence.
  int previous = note_pos - 1;

  // If we currently are at position 0, set the
  // previous position to the last note in the sequence.
  if (previous < 0) previous = sizeof(note_sequence) - 1;

  // Send Note On for current position at full velocity (127) on channel 1.
  msg[0] = 0x90;                    // Note On - Channel 1
  msg[1] = note_sequence[note_pos]; // Note Number
  msg[2] = 127;                     // Velocity
  tud_midi_n_stream_write(0, 0, msg, 3);

  // Send Note Off for previous note.
  msg[0] = 0x80;                    // Note Off - Channel 1
  msg[1] = note_sequence[previous]; // Note Number
  msg[2] = 0;                       // Velocity
  tud_midi_n_stream_write(0, 0, msg, 3);

  // Increment position
  note_pos++;

  // If we are at the end of the sequence, start over.
  if (note_pos >= sizeof(note_sequence)) note_pos = 0;
}

void usb_task(void)
{
    tud_task();
    cdc_task();
    //midi_task();
}

