/*pitch.c

*/

/*
   Copyright (c) 2024 Daniel Marks

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "pitch.h"
#include "guitarpico.h"

pitch_edge         pitch_edges[NUM_PITCH_EDGES];
pitch_autocor_peak pitch_autocor[NUM_PITCH_EDGES];

volatile uint pitch_fifo_head = 0;
volatile uint pitch_fifo_tail = 0;
uint autocor_size = 0;
int32_t last_sample;
bool current_negative = false;

void initialize_pitch(void)
{
    memset((void *)pitch_edges,'\000',sizeof(pitch_edges));
    pitch_fifo_head = pitch_fifo_tail = 0;
    current_negative = false;
    autocor_size = 0;
}

bool advance_tail(void)
{
    uint new_tail = pitch_fifo_tail + 1;
    if (new_tail >= NUM_PITCH_EDGES) new_tail = 0;
    if (new_tail == pitch_fifo_head) return false;
    pitch_fifo_tail = new_tail;
    return true;
}

void advance_tail_to_front(void)
{
    while (advance_tail()) {};
}

void edge_autocorrelation(void)
{
    uint current_head = pitch_fifo_head;
    uint current_tail = pitch_fifo_tail;
    autocor_size = 0;
    
    uint dly_tail = current_tail;
    while (dly_tail != current_head)
    {
        uint32_t offset = pitch_edges[dly_tail].counter - pitch_edges[current_tail].counter;
        uint a_tail = current_tail, a_d_tail = dly_tail;
        int32_t autocor = 0;
        for (;;)
        {
            pitch_edge *pt = &pitch_edges[a_tail];
            pitch_edge *ptd = &pitch_edges[a_d_tail];
            bool signfix = (((pt->negative) && (ptd->negative)) || ((!pt->negative) && (!ptd->negative)));
            int32_t dif = (int32_t)(ptd->counter - pt->counter - offset);
            if (dif > 0)
            {
               if (signfix)
                   autocor += dif;
               else
                   autocor -= dif;
               if (a_tail == current_head) break;
               if ((++a_tail) >= NUM_PITCH_EDGES) a_tail = 0;
            } else
            {
               if (signfix)
                   autocor -= dif;
               else
                   autocor += dif;
               if (a_d_tail == current_head) break;
               if ((++a_d_tail) >= NUM_PITCH_EDGES) a_d_tail = 0;
            }
        }
        pitch_autocor[autocor_size].offset = offset;
        pitch_autocor[autocor_size].autocor = autocor;
        autocor_size++;
        if ((++dly_tail) >= NUM_PITCH_EDGES) dly_tail = 0;
    }
}