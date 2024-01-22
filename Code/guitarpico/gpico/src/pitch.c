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
pitch_autocor_peak pitch_autocor[NUM_AUTOCOR_PEAKS];

volatile bool pitch_buf_reset = true;
volatile uint pitch_current_entry = 0;

uint pitch_autocor_size = 0;
bool pitch_current_negative = false;

void initialize_pitch(void)
{
    memset((void *)pitch_edges,'\000',sizeof(pitch_edges));
    pitch_current_negative = false;
    pitch_autocor_size = 0;
    pitch_current_entry = 0;
    pitch_buffer_reset();
}

static inline float fitaxis(float x1, float y1, float x2, float y2, float x3, float y3)
{
    float y32 = y3-y2;
    float y21 = y2-y1;
    float y13 = y1-y3;
    
    return (x1*x1*y32 + x2*x2*y13 + x3*x3*y21)*0.5f/(x1*y32+x2*y13+x3*y21);
}

int32_t pitch_estimate_peak_hz(void)
{
    int32_t autocor_max = 0;
    uint entry;
    for (uint i=0;i<pitch_autocor_size;i++)
    {
        if ((pitch_autocor[i].offset > PITCH_MIN_OFFSET) && (pitch_autocor[i].autocor > autocor_max))
        {
            autocor_max = pitch_autocor[i].autocor;
            entry = i;
        }
    }
    if (autocor_max == 0) return 0;
    
    uint32_t offset = pitch_autocor[entry].offset;
    float y1 = pitch_autocorrelation_sample(offset-1);
    float y2 = pitch_autocor[entry].autocor;
    float y3 = pitch_autocorrelation_sample(offset+1);
    float axisfit = 0.5f*(y1-y3)/(y3-2.0f*y2+y1);
    return (uint32_t)(( ((float)GUITARPICO_SAMPLERATE) / (axisfit + ((float)offset))));
}

int32_t pitch_autocorrelation_sample(uint32_t offset)
{
    uint read_pitch_current_entry = pitch_current_entry;
    int cur_entry_m = 0, l_entry_m = 0;
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
     return autocor;
}

void pitch_edge_autocorrelation(void)
{
    pitch_autocor_size = 0;
    uint32_t last_offset = 0;
    
    while (pitch_autocor_size < NUM_AUTOCOR_PEAKS)
    {
        uint32_t offset = 0, smallest_offset = 0xFFFFFFF;
        for (uint offset1=0;offset1<NUM_AUTOCOR_PEAKS_SORT;offset1++)
        {
            for (uint offset2=0;offset2<offset1;offset2++)
            {
                uint32_t test_offset = pitch_edges[offset1].counter - pitch_edges[offset2].counter;
                if ((test_offset > last_offset) && (test_offset < smallest_offset))
                {
                    smallest_offset = test_offset;
                    uint checkval = 0;
                    while (checkval < pitch_autocor_size)
                    {
                        if (pitch_autocor[checkval].offset == test_offset) break;
                        checkval++;
                    }
                    if (checkval == pitch_autocor_size) 
                    {
                        offset = test_offset;
                        break;
                    }
                }
            }
        }
        if (offset == 0) break;
        int32_t autocor = pitch_autocorrelation_sample(offset);
        last_offset = offset;
        pitch_autocor[pitch_autocor_size].offset = offset;
        pitch_autocor[pitch_autocor_size].autocor = autocor;
        pitch_autocor_size++;
    }
}