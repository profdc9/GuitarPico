/* pitch.h

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

#ifndef __PITCH_H
#define __PITCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#define NUM_PITCH_EDGES 32
#define NUM_AUTOCOR_PEAKS 64
#define NUM_AUTOCOR_PEAKS_SORT 13
#define PITCH_MIN_OFFSET 18
#define EDGE_HYSTERESIS (ADC_PREC_VALUE/64)

typedef struct
{
    bool    negative;
    int32_t sample;
    uint32_t counter;
} pitch_edge;

typedef struct
{
    uint32_t offset;
    int32_t  autocor;
} pitch_autocor_peak;

typedef struct
{
    const char *note;
    uint32_t frequency_hz;
} note_struct;

void initialize_pitch(void);
void pitch_edge_autocorrelation(void);
int32_t pitch_autocorrelation_sample(uint32_t offset);
int32_t pitch_autocorrelation_max(int32_t min_offset);
uint32_t pitch_estimate_peak_hz(uint32_t entry);
int32_t pitch_find_note(uint32_t freq_hz);

extern const note_struct notes[];

extern   pitch_edge pitch_edges[NUM_PITCH_EDGES];
extern   pitch_autocor_peak pitch_autocor[NUM_AUTOCOR_PEAKS];

extern volatile uint pitch_current_entry;
extern uint pitch_autocor_size;
extern int32_t pitch_last_sample;
extern bool pitch_current_negative;

extern int32_t pitch_low_hysteresis;
extern int32_t pitch_high_hysteresis;


static inline void pitch_buffer_reset(void)
{
    DMB();
    pitch_current_entry = 0;
//    DMB();
}

static inline void insert_pitch_edge(int32_t sample, uint32_t counter)
{
    int32_t sample_thr = sample / 2;
    if (sample_thr > 0)
    {
        if (sample_thr > pitch_high_hysteresis)
            pitch_high_hysteresis = sample_thr;
    } else
    {
        if (sample_thr < pitch_low_hysteresis)
            pitch_low_hysteresis = sample_thr;
    }
    
    pitch_low_hysteresis = (pitch_low_hysteresis * 1023) / 1024;
    if (pitch_low_hysteresis > -EDGE_HYSTERESIS)
        pitch_low_hysteresis = -EDGE_HYSTERESIS;
    pitch_high_hysteresis = (pitch_high_hysteresis * 1023) / 1024;
    if (pitch_high_hysteresis < EDGE_HYSTERESIS)
        pitch_high_hysteresis = EDGE_HYSTERESIS;
    
    if (pitch_current_entry < NUM_PITCH_EDGES)
    {
        if (((!pitch_current_negative) && (sample < pitch_low_hysteresis)) || ((pitch_current_negative) && (sample> pitch_high_hysteresis)))
        {
            pitch_edge *p = &pitch_edges[pitch_current_entry];
            pitch_current_negative = !pitch_current_negative;
            p->negative = pitch_current_negative;
            p->sample = sample;
            p->counter = counter;
            pitch_current_entry++;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __PITCH_H */