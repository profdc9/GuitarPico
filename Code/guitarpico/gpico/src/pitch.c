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

const note_struct notes[] = 
{
    {    "C1",      33 },
    {    "C#1/Db1", 35 },
    {    "D1",      37 },
    {    "D#1/Eb1", 39 },
    {    "E1",      41 },
    {    "F1",      44 },
    {    "F#1/Gb1", 46 }, 
    {    "G1",      49 },
    {    "G#1/Ab1", 52 },
    {    "A1",      55 },
    {    "A#1/Bb1", 58 },
    {    "B1",      62 },
    
    {    "C2",      65 },
    {    "C#2/Db2", 69 },
    {    "D2",      73 },
    {    "D#2/Eb2", 78 },
    {    "E2",      82 },
    {    "F2",      87 },
    {    "F#2/Gb2", 93 }, 
    {    "G2",      98 },
    {    "G#2/Ab2", 104 },
    {    "A2",      110 },
    {    "A#2/Bb2", 117 },
    {    "B2",      123 },

    {    "C3",      131 },
    {    "C#3/Db3", 139 },
    {    "D3",      147 },
    {    "D#3/Eb3", 156 },
    {    "E3",      165 },
    {    "F3",      175 },
    {    "F#3/Gb3", 185 }, 
    {    "G3",      196 },
    {    "G#3/Ab3", 208 },
    {    "A3",      220 },
    {    "A#3/Bb3", 233 },
    {    "B3",      247 },

    {    "C4",      262 },
    {    "C#4/Db4", 277 },
    {    "D4",      294 },
    {    "D#4/Eb4", 311 },
    {    "E4",      330 },
    {    "F4",      349 },
    {    "F#4/Gb4", 370 }, 
    {    "G4",      392 },
    {    "G#4/Ab4", 415 },
    {    "A4",      440 },
    {    "A#4/Bb4", 466 },
    {    "B4",      494 },

    {    "C5",      523 },
    {    "C#5/Db5", 554 },
    {    "D5",      587 },
    {    "D#5/Eb5", 622 },
    {    "E5",      659 },
    {    "F5",      698 },
    {    "F#5/Gb5", 740 }, 
    {    "G5",      784 },
    {    "G#5/Ab5", 831 },
    {    "A5",      880 },
    {    "A#5/Bb5", 932 },
    {    "B5",      988 },

    {    "C6",      1047 },
    {    "C#6/Db6", 1109 },
    {    "D6",      1175 },
    {    "D#6/Eb6", 1244 },
    {    "E6",      1319 },
    {    "F6",      1397 },
    {    "F#6/Gb6", 1480 }, 
    {    "G6",      1568 }
};

int32_t pitch_find_note(uint32_t freq_hz)
{
    uint t = 0;
    uint b = ((sizeof(notes)/sizeof(notes[0]))-1);
    
    if (freq_hz < notes[0].frequency_hz) return (-1);
    if (freq_hz > notes[(sizeof(notes)/sizeof(notes[0]))-1].frequency_hz) return (-1);
    
    while (t < b)
    {
        uint m = (t+b) / 2;
        if (notes[m].frequency_hz < freq_hz)
        {
            t = m + 1;
        } else if (notes[m].frequency_hz > freq_hz)
        {
            b = m - 1;
        } else
        {
          t = m;
          break;
        }
    }
    uint32_t dif = abs(((int32_t)freq_hz - ((int32_t)notes[t].frequency_hz)));
    uint32_t difm = t > 0 ? abs(((int32_t)freq_hz - ((int32_t)notes[t-1].frequency_hz))) : 0xFFFFFFFF;
    uint32_t difp = t < ((sizeof(notes)/sizeof(notes[0]))-1) ? abs(((int32_t)freq_hz - ((int32_t)notes[t+1].frequency_hz))) : 0xFFFFFFFF;
    if ((dif < difm) && (dif < difp)) return t;
    if ((difm < dif) && (difm < difp)) return (t-1);
    return (t+1);
}

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
    uint32_t last_offset = PITCH_MIN_OFFSET;
    
    while (pitch_autocor_size < NUM_AUTOCOR_PEAKS)
    {
        uint32_t offset = 0xFFFFFFFF;
        for (uint offset1=0;offset1<NUM_AUTOCOR_PEAKS_SORT;offset1++)
        {
            for (uint offset2=0;offset2<offset1;offset2++)
            {
                uint32_t test_offset = pitch_edges[offset1].counter - pitch_edges[offset2].counter;
                if ((test_offset > last_offset) && (test_offset < offset))
                    offset = test_offset;
            }
        }
        if (offset == 0xFFFFFFFF) break;
        int32_t autocor = pitch_autocorrelation_sample(offset);
        last_offset = offset;
        pitch_autocor[pitch_autocor_size].offset = offset;
        pitch_autocor[pitch_autocor_size].autocor = autocor;
        pitch_autocor_size++;
    }
}