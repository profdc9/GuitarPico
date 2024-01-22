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
#include "guitarpico.h"
#include "pitch.h"

pitch_edge         pitch_edges[NUM_PITCH_EDGES];
pitch_autocor_peak pitch_autocor[NUM_PITCH_EDGES];

volatile bool pitch_buf_reset = true;
volatile uint pitch_current_entry = 0;

uint pitch_autocor_size = 0;
int32_t pitch_last_sample;
bool pitch_current_negative = false;

void initialize_pitch(void)
{
    memset((void *)pitch_edges,'\000',sizeof(pitch_edges));
    pitch_current_negative = false;
    pitch_autocor_size = 0;
    pitch_current_entry = 0;
    pitch_buffer_reset();
}

void edge_autocorrelation(void)
{
    uint read_pitch_current_entry = pitch_current_entry;
    pitch_autocor_size = 0;    
    uint cur_entry = 0;
    while (cur_entry < read_pitch_current_entry) 
    {
        uint32_t offset = pitch_edges[cur_entry].counter - pitch_edges[0].counter;
        uint cur_entry_m = cur_entry, l_entry_m = 0;
        int32_t autocor = 0;
        for (;;)
        {
            pitch_edge *pt = &pitch_edges[l_entry_m];
            pitch_edge *ptd = &pitch_edges[cur_entry_m];
            bool signfix = (((pt->negative) && (ptd->negative)) || ((!pt->negative) && (!ptd->negative)));
            int32_t dif = (int32_t)(ptd->counter - pt->counter - offset);
            if (dif > 0)
            {
               if (signfix)
                   autocor -= dif;
               else
                   autocor += dif;
               if ((++l_entry_m) >= read_pitch_current_entry) break;
            } else
            {
               if (signfix)
                   autocor += dif;
               else
                   autocor -= dif;
               if ((++cur_entry_m) >= read_pitch_current_entry) break;
            }
        }
        pitch_autocor[pitch_autocor_size].offset = offset;
        pitch_autocor[pitch_autocor_size].autocor = autocor;
        pitch_autocor_size++;
        cur_entry++;
    }
}