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

typedef uint32_t uint_pitch;

#define NUM_AUTOCOR_PEAKS 20
#define NUM_PITCH_BITS 2048
#define NUM_PITCH_BITS_PER_UINT 32
#define NUM_PITCH_UINTS (NUM_PITCH_BITS/NUM_PITCH_BITS_PER_UINT)
#define EDGE_HYSTERESIS 100

typedef struct
{
    uint32_t offset;
    int32_t  autocor;
} pitch_autocor_peak;

void initialize_pitch(void);
void edge_autocorrelation(void);
bool advance_tail(void);
void advance_tail_to_front(void);

extern   uint_pitch  pitch_sign[NUM_PITCH_UINTS];

extern   pitch_autocor_peak pitch_autocor[NUM_PITCH_EDGES];

extern volatile bool pitch_buf_reset;
extern volatile uint pitch_current_entry;
extern uint pitch_autocor_size;
extern bool pitch_current_negative;

static inline void pitch_buffer_reset(void)
{
    DMB();
    pitch_buf_reset = 1;
    DMB();
}

static inline uint32_t count_bits(uint32_t i)
{
#ifdef __GNUC__
     return __builtin_popcount(i);
#else
     // C or C++: use uint32_t
     i = i - ((i >> 1) & 0x55555555);                 // add pairs of bits
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
     i = (i + (i >> 4)) & 0x0F0F0F0F;                 // groups of 8
     i *= 0x01010101;                                 // horizontal sum of bytes
     return  i >> 24;                                 // return just that top byte (after truncating to 32-bit even when int is wider than uint32_t)
#endif
}


static inline void insert_pitch_edge(int32_t sample, uint32_t counter)
{
    pitch_last_sample = sample;
    if (pitch_buf_reset)
    {
        pitch_current_entry = 0;
        pitch_buf_reset = 0;
    }
    if (pitch_current_entry < NUM_PITCH_BITS)
    {
        if (((!pitch_current_negative) && (sample < (-EDGE_HYSTERESIS))) || ((pitch_current_negative) && (sample > EDGE_HYSTERESIS)))
        {
            pitch_current_negative = !pitch_current_negative;
            uint pce = pitch_current_entry;
            if (pitch_current_negative)
                pitch_sign[pce / NUM_PITCH_BITS_PER_UINT] |= (1u << (pce & (NUM_PITCH_BITS_PER_UINT-1)));
            pitch_current_entry++;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __PITCH_H */