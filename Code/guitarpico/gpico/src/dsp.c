/* dsp.c

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
#include "dsp.h"
#include "guitarpico.h"

#define QUANTIZATION_BITS 15
#define QUANTIZATION_MAX (1<<QUANTIZATION_BITS)
#define QUANTIZATION_MAX_FLOAT ((float)(1<<QUANTIZATION_BITS))

int32_t sine_table[SINE_TABLE_ENTRIES];

int sample_circ_buf_offset;
int16_t sample_circ_buf[SAMPLE_CIRC_BUF_SIZE];
int sample_circ_buf_clean_offset;
int16_t sample_circ_buf_clean[SAMPLE_CIRC_BUF_SIZE];

dsp_unit dsp_units[MAX_DSP_UNITS];
int32_t dsp_unit_result[MAX_DSP_UNITS+1];

void initialize_sine_table(void)
{
    for (int i=0;i<SINE_TABLE_ENTRIES;i++)
        sine_table[i] =  (int32_t)(QUANTIZATION_MAX_FLOAT * sinf((2.0f*MATH_PI_F/((float)SINE_TABLE_ENTRIES))*((float)i)));
}

void initialize_sample_circ_buf(void)
{
    memset((void *)sample_circ_buf, '\000', sizeof(sample_circ_buf));
    sample_circ_buf_offset = 0;
    memset((void *)sample_circ_buf_clean, '\000', sizeof(sample_circ_buf_clean));
    sample_circ_buf_clean_offset = 0;
}

void dsp_unit_struct_zero(dsp_unit *du)
{
    memset((void *)du,'\000',sizeof(dsp_unit));
}

/************Float to quantized integer offset instructions *******************************/

inline float nyquist_fraction_omega(uint16_t frequency)
{
    return ((float)frequency)*(2.0f*MATH_PI_F/((float)GUITARPICO_SAMPLERATE));
}

inline float Q_value(uint16_t Q)
{
    return (((float)Q)*0.01f);
}

/* Q = 100 means Q scaled to one */
inline float float_a_value(float omega_value, uint16_t Q)
{
    return sinf(omega_value)*(50.0f)/((float)Q);
}

inline int32_t float_to_sampled_int(float x)
{
    return (int32_t)(QUANTIZATION_MAX_FLOAT*x+0.5f);
}

inline int16_t fractional_int_remove_offset(int32_t x)
{
    return (int32_t)(x/QUANTIZATION_MAX);
}

/**************************** DSP_TYPE_NONE **************************************************/

int32_t dsp_type_process_none(int32_t sample, dsp_unit *du)
{
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_none[] = 
{
    { "SourceUnit",  offsetof(dsp_type_none,source_unit),        4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_none dsp_type_none_default = { 0, 0 };

/******************************DSP_TYPE_SINE_SYNTH*******************************************/

int32_t dsp_type_process_sin_synth(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtss.control_number1);
    if (abs(new_input - du->dtss.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtss.pot_value1 = new_input;
        du->dtss.frequency[0] = 40 + new_input/(POT_MAX_VALUE/2048);
    }
    new_input = read_potentiometer_value(du->dtss.control_number2);
    if (abs(new_input - du->dtss.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtss.pot_value2 = new_input;
        du->dtss.amplitude[0] = new_input/(POT_MAX_VALUE/256);
    }
    if (du->dtss.frequency[0] != du->dtss.last_frequency[0])
    {
        du->dtss.last_frequency[0] = du->dtss.frequency[0];
        du->dtss.sine_counter_inc[0] = (du->dtss.frequency[0]*65536) / GUITARPICO_SAMPLERATE;
    }
    if (du->dtss.frequency[1] != du->dtss.last_frequency[1])
    {
        du->dtss.last_frequency[1] = du->dtss.frequency[1];
        du->dtss.sine_counter_inc[1] = (du->dtss.frequency[1]*65536) / GUITARPICO_SAMPLERATE;
    }
    if (du->dtss.frequency[2] != du->dtss.last_frequency[2])
    {
        du->dtss.last_frequency[2] = du->dtss.frequency[2];
        du->dtss.sine_counter_inc[2] = (du->dtss.frequency[2]*65536) / GUITARPICO_SAMPLERATE;
    }
    uint ct = 1;
    int32_t sine_val = sample * ((int32_t)du->dtss.mixval);
    for (uint i=0;i<(sizeof(du->dtss.sine_counter)/sizeof(du->dtss.sine_counter[0]));i++)
    {
        if (du->dtss.amplitude[i] != 0) 
        {
            du->dtss.sine_counter[i] += du->dtss.sine_counter_inc[i];
            int32_t val = sine_table_entry((du->dtss.sine_counter[i] & 0xFFFF) / 256) / (QUANTIZATION_MAX / (ADC_PREC_VALUE/2));
            sine_val += val * ((int32_t)du->dtss.amplitude[i]);
            ct++;
        }
    }
    if (ct > 2)
    {
        sine_val /= (256 * 4);
    } else if (ct > 1)
    {
        sine_val /= (256 * 2);
    }
    return sine_val;
}


const dsp_type_configuration_entry dsp_type_configuration_entry_sin_synth[] = 
{
    { "Freq1",        offsetof(dsp_type_sine_synth,frequency[0]),     4, 4, 100, 4000 },
    { "Amplitude1",   offsetof(dsp_type_sine_synth,amplitude[0]),     4, 3, 0,   255  },
    { "Freq2",        offsetof(dsp_type_sine_synth,frequency[1]),     4, 4, 100, 4000 },
    { "Amplitude2",   offsetof(dsp_type_sine_synth,amplitude[1]),     4, 3, 0,   255 },
    { "Freq3",        offsetof(dsp_type_sine_synth,frequency[2]),     4, 4, 100, 4000 },
    { "Amplitude3",   offsetof(dsp_type_sine_synth,amplitude[2]),     4, 3, 0,   255 },
    { "Mixval",       offsetof(dsp_type_sine_synth,mixval),           4, 4, 0,   255 },
    { "FreqCtrl",     offsetof(dsp_type_sine_synth,control_number1),  4, 1, 0, 6 },
    { "AmpCtrl",      offsetof(dsp_type_sine_synth,control_number2),  4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_sine_synth dsp_type_sine_synth_default = { 0, 0, 255, { 300, 300, 300}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0, 0, 0, 0 };

/************************************DSP_TYPE_NOISEGATE**********************************/

int32_t dsp_type_process_noisegate(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtnoise.control_number1);
    if (abs(new_input - du->dtnoise.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtnoise.pot_value1 = new_input;
        du->dtnoise.threshold = (du->dtnoise.pot_value1 * (ADC_PREC_VALUE/2)) / (POT_MAX_VALUE*16);
    }
    
    uint32_t envfilt;
    uint32_t abssample = sample < 0 ? -sample : sample;
    switch (du->dtnoise.response)
    {
        case 1:  du->dtnoise.envelope = (du->dtnoise.envelope*127)/128 + abssample;
                 envfilt = du->dtnoise.envelope / 128;
                 break;
        case 2:  du->dtnoise.envelope = (du->dtnoise.envelope*255)/256 + abssample;
                 envfilt = du->dtnoise.envelope / 256;
                 break;
        case 3:  du->dtnoise.envelope = (du->dtnoise.envelope*511)/512 + abssample;
                 envfilt = du->dtnoise.envelope / 512;
                 break;
    }
    return (envfilt > du->dtnoise.threshold) ? sample : 0;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_noisegate[] = 
{
    { "Threshold",       offsetof(dsp_type_noisegate,threshold),           4, 5, 1, ADC_PREC_VALUE/2 },
    { "Response",        offsetof(dsp_type_noisegate,response),            4, 1, 1, 3 },
    { "ThresholdCtrl",   offsetof(dsp_type_noisegate,control_number1),     4, 1, 0, 6 },
    { "SourceUnit", offsetof(dsp_type_noisegate,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_noisegate dsp_type_noisegate_default = { 0, 0, 30, 3, 0, 0, 0  };

/************************************DSP_TYPE_DELAY*************************************/

int32_t dsp_type_process_delay(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtd.control_number1);
    if (abs(new_input - du->dtd.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtd.pot_value1 = new_input;
        du->dtd.delay_samples = (du->dtd.pot_value1 * SAMPLE_CIRC_BUF_SIZE) / POT_MAX_VALUE;
    }
    new_input = read_potentiometer_value(du->dtd.control_number2);
    if (abs(new_input - du->dtd.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtd.pot_value2 = new_input;
        du->dtd.echo_reduction = (du->dtd.pot_value2 * 256) / POT_MAX_VALUE;
    }
    sample = (sample + ((sample_circ_buf_value(du->dtd.delay_samples) * ((int16_t)du->dtd.echo_reduction)) / 256)) / 2;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_delay[] = 
{
    { "Samples",    offsetof(dsp_type_delay,delay_samples),   4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "EchoRed",    offsetof(dsp_type_delay,echo_reduction),  4, 3, 0, 255 },
    { "SampCtrl",   offsetof(dsp_type_delay,control_number1), 4, 1, 0, 6 },
    { "EchoCtrl",   offsetof(dsp_type_delay,control_number2), 4, 1, 0, 6 },
    { "SourceUnit", offsetof(dsp_type_delay,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_delay dsp_type_delay_default = { 0, 0, 10000, 192, 0, 0, 0, 0 };

/************************************DSP_TYPE_ROOM*************************************/

int32_t dsp_type_process_room(int32_t sample, dsp_unit *du)
{
    int count = 1;
    sample *= 256;
    for (int i=0;i<(sizeof(du->dtroom.delay_samples)/sizeof(du->dtroom.delay_samples[0]));i++)
    {
        if (du->dtroom.amplitude[i] != 0)
        {
            sample += sample_circ_buf_clean_value(du->dtroom.delay_samples[i]) * du->dtroom.amplitude[i];
            count++;
        }
    }
    if (count > 2)
        sample /= (4*256);
    else if (count > 1)
        sample /= (2*256);
    else sample /= 256;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_room[] = 
{
    { "Sample1",     offsetof(dsp_type_room,delay_samples[0]),   4, 5, 1, SAMPLE_CIRC_BUF_CLEAN_SIZE },
    { "Amplitude1",  offsetof(dsp_type_room,amplitude[0]),       4, 3, 0, 255 },
    { "Sample2",     offsetof(dsp_type_room,delay_samples[1]),   4, 5, 1, SAMPLE_CIRC_BUF_CLEAN_SIZE },
    { "Amplitude2",  offsetof(dsp_type_room,amplitude[1]),       4, 3, 0, 255 },
    { "Sample3",     offsetof(dsp_type_room,delay_samples[2]),   4, 5, 1, SAMPLE_CIRC_BUF_CLEAN_SIZE },
    { "Amplitude3",  offsetof(dsp_type_room,amplitude[2]),       4, 3, 0, 255 },
    { "SourceUnit",  offsetof(dsp_type_room,source_unit),        4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_room dsp_type_room_default = { 0, 0, { 2500, 0, 0, }, {255, 0, 0 } };

/************************************DSP_TYPE_COMBINE*************************************/

int32_t dsp_type_process_combine(int32_t sample, dsp_unit *du)
{
    int count = 0;
    if (du->dtcombine.prev_amplitude > 0)
    {
        sample *= du->dtcombine.prev_amplitude;
        count++;
    }
    for (int i=0;i<(sizeof(du->dtcombine.unit)/sizeof(du->dtcombine.unit[0]));i++)
    {
        if (du->dtcombine.amplitude[i] != 0)
        {
            if (du->dtcombine.signbit[i] != 0)
                sample -= dsp_unit_result[du->dtcombine.unit[i]-1] * du->dtcombine.amplitude[i];
            else
                sample += dsp_unit_result[du->dtcombine.unit[i]-1] * du->dtcombine.amplitude[i];
            count++;
        }
    }
    if (count > 2)
        sample /= (4*256);
    else if (count > 1)
        sample /= (2*256);
    else sample /= 256;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_combine[] = 
{
    { "Ampltiude",   offsetof(dsp_type_combine,prev_amplitude),     4, 3, 0, 255 },
    { "Unit1",       offsetof(dsp_type_combine,unit[0]),            4, 2, 1, MAX_DSP_UNITS },
    { "Amplitude1",  offsetof(dsp_type_combine,amplitude[0]),       4, 3, 0, 255 },
    { "Sign1",       offsetof(dsp_type_combine,signbit[0]),         4, 1, 0, 1 },
    { "Unit2",       offsetof(dsp_type_combine,unit[1]),            4, 2, 1, MAX_DSP_UNITS },
    { "Amplitude2",  offsetof(dsp_type_combine,amplitude[1]),       4, 3, 0, 255 },
    { "Sign2",       offsetof(dsp_type_combine,signbit[1]),         4, 1, 0, 1 },
    { "Unit3",       offsetof(dsp_type_combine,unit[2]),            4, 2, 1, MAX_DSP_UNITS },
    { "Amplitude3",  offsetof(dsp_type_combine,amplitude[2]),       4, 3, 0, 255 },
    { "Sign3",       offsetof(dsp_type_combine,signbit[2]),         4, 1, 0, 1 },
    { "SourceUnit",  offsetof(dsp_type_combine,source_unit),        4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_combine dsp_type_combine_default = { 0, 0, 255, { 2, 3, 4, }, {0, 0, 0 }, { 0, 0, 0 } };

/************************************DSP_TYPE_BANDPASS*************************************/

int32_t dsp_type_process_bandpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtbp.control_number1);
    if (abs(new_input - du->dtbp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtbp.pot_value1 = new_input;
        du->dtbp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
    }
    if ((du->dtbp.frequency != du->dtbp.last_frequency) || (du->dtbp.Q != du->dtbp.last_Q))
    {
        du->dtbp.last_frequency = du->dtbp.frequency;
        du->dtbp.last_Q = du->dtbp.Q;
        float w0 = nyquist_fraction_omega(du->dtbp.frequency);
        float a = float_a_value(w0,du->dtbp.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtbp.filtb0 = float_to_sampled_int(a * bfpa0);
        du->dtbp.filta1 = float_to_sampled_int(-2.0f*cosf(w0)*bfpa0);
        du->dtbp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dtbp.filtb0) * ((int32_t)sample - (int32_t)du->dtbp.sampledly2)
               - ((int32_t)du->dtbp.filta1) * ((int32_t)du->dtbp.filtdly1)
               - ((int32_t)du->dtbp.filta2) * ((int32_t)du->dtbp.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtbp.sampledly2 = du->dtbp.sampledly1;
    du->dtbp.sampledly1 = sample;
    du->dtbp.filtdly2 = du->dtbp.filtdly1;
    du->dtbp.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_bandpass[] = 
{
    { "Frequency",   offsetof(dsp_type_bandpass,frequency),       2, 4, 100, 4000 },
    { "Q",           offsetof(dsp_type_bandpass,Q),               2, 3, 50, 999 },
    { "FreqCntrl",   offsetof(dsp_type_bandpass,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",  offsetof(dsp_type_bandpass,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_bandpass dsp_type_bandpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_LOWPASS*************************************/

int32_t dsp_type_process_lowpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtlp.control_number1);
    if (abs(new_input - du->dtlp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtlp.pot_value1 = new_input;
        du->dtlp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
    }
    if ((du->dtlp.frequency != du->dtlp.last_frequency) || (du->dtlp.Q != du->dtlp.last_Q))
    {
        du->dtlp.last_frequency = du->dtlp.frequency;
        du->dtlp.last_Q = du->dtlp.Q;
        float w0 = nyquist_fraction_omega(du->dtlp.frequency);
        float c0 = cosf(w0);
        float a = float_a_value(w0,du->dtlp.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtlp.filtb1 = float_to_sampled_int((1.0f-c0)*bfpa0);
        du->dtlp.filtb0 = float_to_sampled_int(0.5*(1.0f-c0)*bfpa0);
        du->dtlp.filta1 = float_to_sampled_int(-2.0f*c0*bfpa0);
        du->dtlp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dtlp.filtb0) * ((int32_t)sample + (int32_t)du->dtlp.sampledly2)
               + ((int32_t)du->dtlp.filtb1) * ((int32_t)du->dtlp.sampledly1)  
               - ((int32_t)du->dtlp.filta1) * ((int32_t)du->dtlp.filtdly1)
               - ((int32_t)du->dtlp.filta2) * ((int32_t)du->dtlp.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtlp.sampledly2 = du->dtlp.sampledly1;
    du->dtlp.sampledly1 = sample;
    du->dtlp.filtdly2 = du->dtlp.filtdly1;
    du->dtlp.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_lowpass[] = 
{
    { "Frequency",   offsetof(dsp_type_lowpass,frequency),       2, 4, 100, 4000 },
    { "Q",           offsetof(dsp_type_lowpass,Q),               2, 3, 50, 999 },
    { "FreqCntrl",   offsetof(dsp_type_lowpass,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",  offsetof(dsp_type_lowpass,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_lowpass dsp_type_lowpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_HIGHPASS*************************************/

int32_t dsp_type_process_highpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dthp.control_number1);
    if (abs(new_input - du->dthp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dthp.pot_value1 = new_input;
        du->dthp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
    }
    if ((du->dthp.frequency != du->dthp.last_frequency) || (du->dthp.Q != du->dthp.last_Q))
    {
        du->dthp.last_frequency = du->dthp.frequency;
        du->dthp.last_Q = du->dthp.Q;
        float w0 = nyquist_fraction_omega(du->dthp.frequency);
        float c0 = cosf(w0);
        float a = float_a_value(w0,du->dthp.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dthp.filtb1 = float_to_sampled_int(-(1.0f+c0)*bfpa0);
        du->dthp.filtb0 = float_to_sampled_int(0.5*(1.0f+c0)*bfpa0);
        du->dthp.filta1 = float_to_sampled_int(-2.0f*c0*bfpa0);
        du->dthp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dthp.filtb0) * ((int32_t)sample + (int32_t)du->dthp.sampledly2)
               + ((int32_t)du->dthp.filtb1) * ((int32_t)du->dthp.sampledly1)  
               - ((int32_t)du->dthp.filta1) * ((int32_t)du->dthp.filtdly1)
               - ((int32_t)du->dthp.filta2) * ((int32_t)du->dthp.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dthp.sampledly2 = du->dthp.sampledly1;
    du->dthp.sampledly1 = sample;
    du->dthp.filtdly2 = du->dthp.filtdly1;
    du->dthp.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_highpass[] = 
{
    { "Frequency",   offsetof(dsp_type_highpass,frequency),       2, 4, 100, 4000 },
    { "Q",           offsetof(dsp_type_highpass,Q),               2, 3, 50, 999 },
    { "FreqCntrl",   offsetof(dsp_type_highpass,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",  offsetof(dsp_type_highpass,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_highpass dsp_type_highpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_ALLPASS*************************************/

int32_t dsp_type_process_allpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtap.control_number1);
    if (abs(new_input - du->dtap.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtap.pot_value1 = new_input;
        du->dtap.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
    }
    if ((du->dtap.frequency != du->dtap.last_frequency) || (du->dtap.Q != du->dtap.last_Q))
    {
        du->dtap.last_frequency = du->dtap.frequency;
        du->dtap.last_Q = du->dtap.Q;
        float w0 = nyquist_fraction_omega(du->dtap.frequency);
        float a = float_a_value(w0,du->dtap.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtap.filta1 = float_to_sampled_int(-2.0f*cosf(w0)*bfpa0);
        du->dtap.filta2 = float_to_sampled_int((1.0f-a) * bfpa0);
    }
    filtout =    ((int32_t)du->dtap.filta2) * ((int32_t)sample - (int32_t)du->dtap.filtdly2)
               + ((int32_t)du->dtap.filta1) * ((int32_t)du->dtap.sampledly1 - (int32_t)du->dtap.filtdly1)
               + ((int32_t)du->dtap.sampledly2) * float_to_sampled_int(1.0f);
    filtout = fractional_int_remove_offset(filtout);
    du->dtap.sampledly2 = du->dtap.sampledly1;
    du->dtap.sampledly1 = sample;
    du->dtap.filtdly2 = du->dtap.filtdly1;
    du->dtap.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_allpass[] = 
{
    { "Frequency",   offsetof(dsp_type_allpass,frequency),       2, 4, 100, 4000 },
    { "Q",           offsetof(dsp_type_allpass,Q),               2, 3, 50, 999 },
    { "FreqCntrl",   offsetof(dsp_type_allpass,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",  offsetof(dsp_type_allpass,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_allpass dsp_type_allpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_TREMOLO*************************************/

int32_t dsp_type_process_tremolo(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dttrem.control_number1);
    if (abs(new_input - du->dttrem.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dttrem.pot_value1 = new_input;
        du->dttrem.frequency = 1 + new_input/(POT_MAX_VALUE/32);
    }
    new_input = read_potentiometer_value(du->dttrem.control_number2);
    if (abs(new_input - du->dttrem.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dttrem.pot_value2 = new_input;
        du->dttrem.modulation = (du->dttrem.pot_value2 * 256) / POT_MAX_VALUE;
    }
    if (du->dttrem.frequency != du->dttrem.last_frequency)
    {
        du->dttrem.last_frequency = du->dttrem.frequency;
        du->dttrem.sine_counter_inc = (du->dttrem.frequency*65536) / GUITARPICO_SAMPLERATE;
    }
    du->dttrem.sine_counter += du->dttrem.sine_counter_inc;
    int32_t sine_val = sine_table_entry((du->dttrem.sine_counter & 0xFFFF) / 256);
    int32_t mod_val = ((sine_val * du->dttrem.modulation) + QUANTIZATION_MAX * 256) / 512;
    sample = (sample * mod_val) / QUANTIZATION_MAX;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_tremolo[] = 
{
    { "Frequency",    offsetof(dsp_type_tremolo,frequency),       4, 2, 1, 32 },
    { "Modulation",   offsetof(dsp_type_tremolo,modulation),      4, 3, 0, 255 },
    { "FreqCntrl",    offsetof(dsp_type_tremolo,control_number1), 4, 1, 0, 6 },
    { "ModCntrl",     offsetof(dsp_type_tremolo,control_number2), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_tremolo,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_tremolo dsp_type_tremolo_default = { 0, 0, 0, 0, 6, 0, 128, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_VIBRATO*************************************/

int32_t dsp_type_process_vibrato(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtvibr.control_number1);
    if (abs(new_input - du->dtvibr.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtvibr.pot_value1 = new_input;
        du->dtvibr.frequency = 1 + new_input/(POT_MAX_VALUE/32);
    }
    new_input = read_potentiometer_value(du->dtvibr.control_number2);
    if (abs(new_input - du->dtvibr.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtvibr.pot_value2 = new_input;
        du->dtvibr.modulation = (du->dtvibr.pot_value2 * 256) / POT_MAX_VALUE;
    }
    if (du->dtvibr.frequency != du->dtvibr.last_frequency)
    {
        du->dtvibr.last_frequency = du->dtvibr.frequency;
        du->dtvibr.sine_counter_inc = (du->dtvibr.frequency*65536) / GUITARPICO_SAMPLERATE;
    }
    du->dtvibr.sine_counter += du->dtvibr.sine_counter_inc;
    int32_t sine_val = sine_table_entry((du->dtvibr.sine_counter & 0xFFFF) / 256);
    int32_t mod_val = ((sine_val * du->dtvibr.modulation) + QUANTIZATION_MAX * 256) / 512;
    uint32_t delay_samples = (du->dtvibr.delay_samples * mod_val) / QUANTIZATION_MAX;
    sample = sample_circ_buf_clean_value(delay_samples);
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_vibrato[] = 
{
    { "Frequency",    offsetof(dsp_type_vibrato,frequency),       4, 2, 1, 32 },
    { "Modulation",   offsetof(dsp_type_vibrato,modulation),      4, 3, 0, 255 },
    { "Samples",      offsetof(dsp_type_vibrato,delay_samples),   4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "FreqCntrl",    offsetof(dsp_type_vibrato,control_number1), 4, 1, 0, 6 },
    { "ModCntrl",     offsetof(dsp_type_vibrato,control_number2), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_vibrato,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_vibrato dsp_type_vibrato_default = { 0, 0, 0, 0, 40, 6, 0, 128, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_WAH*************************************/

int32_t dsp_type_process_wah(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    if ((du->dtwah.freq1 != du->dtwah.last_freq1) || (du->dtwah.freq2 != du->dtwah.last_freq2) || (du->dtwah.Q != du->dtwah.last_Q))
    {
        du->dtwah.last_freq1 = du->dtwah.freq1;
        du->dtwah.last_freq2 = du->dtwah.freq2;
        du->dtwah.last_Q = du->dtwah.Q;
        float w1 = nyquist_fraction_omega(du->dtwah.freq1);
        float w2 = nyquist_fraction_omega(du->dtwah.freq2);
        if (w1 > w2)
        {
            float temp = w1;
            w1 = w2;
            w2 = temp;
        }
        float a = float_a_value(w2, du->dtwah.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtwah.filtb0 = float_to_sampled_int(a * bfpa0);
        du->dtwah.filta1_interp1 = du->dtwah.filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtwah.filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtwah.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    uint32_t new_input = read_potentiometer_value(du->dtwah.control_number1);
    if (abs(new_input - du->dtwah.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtwah.pot_value1 = new_input;
        du->dtwah.filta1 = du->dtwah.filta1_interp1 + ((du->dtwah.filta1_interp2 - du->dtwah.filta1_interp1) * 
                            (du->dtwah.reverse ? new_input : (POT_MAX_VALUE-1) - new_input )) / POT_MAX_VALUE;
    }
    filtout =    ((int32_t)du->dtwah.filtb0) * ((int32_t)sample - (int32_t)du->dtwah.sampledly2)
               - ((int32_t)du->dtwah.filta1) * ((int32_t)du->dtwah.filtdly1)
               - ((int32_t)du->dtwah.filta2) * ((int32_t)du->dtwah.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtwah.sampledly2 = du->dtwah.sampledly1;
    du->dtwah.sampledly1 = sample;
    du->dtwah.filtdly2 = du->dtwah.filtdly1;
    du->dtwah.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_wah[] = 
{
    { "Freq1",        offsetof(dsp_type_wah,freq1),           2, 4, 100, 4000 },
    { "Freq2",        offsetof(dsp_type_wah,freq2),           2, 4, 100, 4000 },
    { "Q",            offsetof(dsp_type_wah,Q),               2, 3, 50, 999 },
    { "FreqCntrl",    offsetof(dsp_type_wah,control_number1), 4, 1, 0, 6 },
    { "Reverse",      offsetof(dsp_type_wah,reverse),         4, 1, 0, 1 },
    { "SourceUnit",   offsetof(dsp_type_wah,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_wah dsp_type_wah_default = { 0, 0, 200, 900, 400, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

/************************************DSP_TYPE_AUTOWAH*************************************/

int32_t dsp_type_process_autowah(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtautowah.control_number1);
    if (abs(new_input - du->dtautowah.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtautowah.pot_value1 = new_input;
        du->dtautowah.frequency = 1 + new_input/(POT_MAX_VALUE/256);
    }
    if (du->dtautowah.frequency != du->dtautowah.last_frequency)
    {
        du->dtautowah.last_frequency = du->dtautowah.frequency;
        du->dtautowah.sine_counter_inc = du->dtautowah.frequency / 2;
    }
    if ((du->dtautowah.freq1 != du->dtautowah.last_freq1) || (du->dtautowah.freq2 != du->dtautowah.last_freq2) || (du->dtautowah.Q != du->dtautowah.last_Q))
    {
        du->dtautowah.last_freq1 = du->dtautowah.freq1;
        du->dtautowah.last_freq2 = du->dtautowah.freq2;
        du->dtautowah.last_Q = du->dtautowah.Q;
        float w1 = nyquist_fraction_omega(du->dtautowah.freq1);
        float w2 = nyquist_fraction_omega(du->dtautowah.freq2);
        if (w1 > w2)
        {
            float temp = w1;
            w1 = w2;
            w2 = temp;
        }
        float a = float_a_value(w2, du->dtautowah.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtautowah.filtb0 = float_to_sampled_int(a * bfpa0);
        du->dtautowah.filta1_interp1 = du->dtautowah.filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtautowah.filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtautowah.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }

    du->dtautowah.sine_counter += du->dtautowah.sine_counter_inc;
    int32_t sine_val = (QUANTIZATION_MAX - 1) - abs(sine_table_entry((du->dtautowah.sine_counter & 0xFFFF0) / 4096));

    
    du->dtautowah.filta1 = du->dtautowah.filta1_interp1 + ((du->dtautowah.filta1_interp2 - du->dtautowah.filta1_interp1) * 
                            sine_val) / QUANTIZATION_MAX;
 
    filtout =    ((int32_t)du->dtautowah.filtb0) * ((int32_t)sample - (int32_t)du->dtautowah.sampledly2)
               - ((int32_t)du->dtautowah.filta1) * ((int32_t)du->dtautowah.filtdly1)
               - ((int32_t)du->dtautowah.filta2) * ((int32_t)du->dtautowah.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtautowah.sampledly2 = du->dtautowah.sampledly1;
    du->dtautowah.sampledly1 = sample;
    du->dtautowah.filtdly2 = du->dtautowah.filtdly1;
    du->dtautowah.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_autowah[] = 
{
    { "Freq1",        offsetof(dsp_type_autowah,freq1),            2, 4, 100, 4000 },
    { "Freq2",        offsetof(dsp_type_autowah,freq2),            2, 4, 100, 4000 },
    { "Q",            offsetof(dsp_type_autowah,Q),                2, 3, 50,  999 },
    { "Speed",        offsetof(dsp_type_autowah,frequency),        4, 4, 1, 4095 },
    { "SpeedCntrl",   offsetof(dsp_type_autowah,control_number1),  4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_autowah,source_unit),      4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_autowah dsp_type_autowah_default = { 0, 0, 200, 900, 400, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_ENVELOPE*************************************/

int32_t dsp_type_process_envelope(int32_t sample, dsp_unit *du)
{
    int32_t filtout;

    if ((du->dtenv.freq1 != du->dtenv.last_freq1) || (du->dtenv.freq2 != du->dtenv.last_freq2) || (du->dtenv.Q != du->dtenv.last_Q))
    {
        du->dtenv.last_freq1 = du->dtenv.freq1;
        du->dtenv.last_freq2 = du->dtenv.freq2;
        du->dtenv.last_Q = du->dtenv.Q;
        float w1 = nyquist_fraction_omega(du->dtenv.freq1);
        float w2 = nyquist_fraction_omega(du->dtenv.freq2);
        if (w1 > w2)
        {
            float temp = w1;
            w1 = w2;
            w2 = temp;
        }
        float a = float_a_value(w2, du->dtenv.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtenv.filtb0 = float_to_sampled_int(a * bfpa0);
        du->dtenv.filta1_interp1 = du->dtenv.filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtenv.filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtenv.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }

    uint32_t envfilt;
    uint32_t abssample = sample < 0 ? -sample : sample;

    switch (du->dtenv.response)
    {
        case 1:  du->dtenv.envelope = (du->dtenv.envelope*127)/128 + abssample;
                 envfilt = du->dtenv.envelope / 128;
                 break;
        case 2:  du->dtenv.envelope = (du->dtenv.envelope*255)/256 + abssample;
                 envfilt = du->dtenv.envelope / 256;
                 break;
        case 3:  du->dtenv.envelope = (du->dtenv.envelope*511)/512 + abssample;
                 envfilt = du->dtenv.envelope / 512;
                 break;
    }
    switch (du->dtenv.sensitivity)
    {
        case 1:  if (envfilt > (ADC_PREC_VALUE/16-1)) envfilt = ADC_PREC_VALUE/16-1;    
                 envfilt =  ((envfilt * (SINE_TABLE_ENTRIES/4) ) / (ADC_PREC_VALUE/16));
                 break;
        case 2:  if (envfilt > (ADC_PREC_VALUE/8-1)) envfilt = ADC_PREC_VALUE/8-1;    
                 envfilt =  ((envfilt * (SINE_TABLE_ENTRIES/4) ) / (ADC_PREC_VALUE/8));
                 break;
        case 3:  if (envfilt > (ADC_PREC_VALUE/4-1)) envfilt = ADC_PREC_VALUE/4-1;    
                 envfilt =  ((envfilt * (SINE_TABLE_ENTRIES/4) ) / (ADC_PREC_VALUE/4));
                 break;
        case 4:  if (envfilt > (ADC_PREC_VALUE/2-1)) envfilt = ADC_PREC_VALUE/2-1;    
                 envfilt =  ((envfilt * (SINE_TABLE_ENTRIES/4) ) / (ADC_PREC_VALUE/2));
                 break;
    }           
    int32_t sin_val = (QUANTIZATION_MAX - 1) - sine_table_entry(envfilt  + (du->dtenv.reverse ? 0 : (SINE_TABLE_ENTRIES/4)));
    du->dtenv.filta1 = du->dtenv.filta1_interp1 + ((du->dtenv.filta1_interp2 - du->dtenv.filta1_interp1) * 
                            sin_val) / QUANTIZATION_MAX;
 
    filtout =    ((int32_t)du->dtenv.filtb0) * ((int32_t)sample - (int32_t)du->dtenv.sampledly2)
               - ((int32_t)du->dtenv.filta1) * ((int32_t)du->dtenv.filtdly1)
               - ((int32_t)du->dtenv.filta2) * ((int32_t)du->dtenv.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtenv.sampledly2 = du->dtenv.sampledly1;
    du->dtenv.sampledly1 = sample;
    du->dtenv.filtdly2 = du->dtenv.filtdly1;
    du->dtenv.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_envelope[] = 
{
    { "Freq1",        offsetof(dsp_type_envelope,freq1),            2, 4, 100, 4000 },
    { "Freq2",        offsetof(dsp_type_envelope,freq2),            2, 4, 100, 4000 },
    { "Q",            offsetof(dsp_type_envelope,Q),                2, 3, 50,  999 },
    { "Sensitivity",  offsetof(dsp_type_envelope,sensitivity),      4, 1, 1, 4 },
    { "Response",     offsetof(dsp_type_envelope,response),         4, 1, 1, 3 },
    { "Reverse",      offsetof(dsp_type_envelope,reverse),          4, 1, 0, 1 },
    { "SourceUnit",   offsetof(dsp_type_envelope,source_unit),      4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_envelope dsp_type_envelope_default = { 0, 0, 200, 1500, 400, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_DISTORTION*************************************/

int32_t dsp_type_process_distortion(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtdist.control_number1);
    if (abs(new_input - du->dtdist.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtdist.pot_value1 = new_input;
        du->dtdist.gain = (new_input * 256) / POT_MAX_VALUE; 
    }
    if (du->dtdist.noise_gate != du->dtdist.last_noise_gate)
    {
        du->dtdist.last_noise_gate = du->dtdist.noise_gate;
        du->dtdist.low_threshold = -((int32_t)du->dtdist.noise_gate)*(ADC_PREC_VALUE/256);
        du->dtdist.high_threshold = ((int32_t)du->dtdist.noise_gate)*(ADC_PREC_VALUE/256);
    }
    if (du->dtdist.sample_offset != du->dtdist.last_sample_offset)
    {
        du->dtdist.last_sample_offset = du->dtdist.sample_offset;
        du->dtdist.offset_value = ((int32_t)du->dtdist.sample_offset)*(ADC_PREC_VALUE/256);
    }
    if ((sample > du->dtdist.low_threshold) && (sample < du->dtdist.high_threshold))
        sample = 0;
    sample = ((sample+du->dtdist.offset_value) * (du->dtdist.gain + 32)) / 32;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_distortion[] = 
{
    { "Gain",         offsetof(dsp_type_distortion,gain),                    4, 3, 1, 255 },
    { "NoiseGate",    offsetof(dsp_type_distortion,noise_gate),              4, 3, 0, 255 },
    { "Offset",       offsetof(dsp_type_distortion,sample_offset),           4, 3, 0, 255 },
    { "GainCntrl",    offsetof(dsp_type_distortion,control_number1),         4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_distortion,source_unit),             4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_distortion dsp_type_distortion_default = { 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_OVERDRIVE*************************************/

int32_t dsp_type_process_overdrive(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtovr.control_number1);
    if (abs(new_input - du->dtovr.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtovr.pot_value1 = new_input;
        du->dtovr.threshold = (new_input * 256) / POT_MAX_VALUE; 
    }
    new_input = read_potentiometer_value(du->dtovr.control_number2);
    if (abs(new_input - du->dtovr.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtovr.pot_value2 = new_input;
        du->dtovr.amplitude = (new_input * 256) / POT_MAX_VALUE; 
    }
    if ((du->dtovr.threshold != du->dtovr.last_threshold) || (du->dtovr.amplitude != du->dtovr.last_amplitude))
    {
        du->dtovr.last_threshold = du->dtovr.threshold;
        du->dtovr.last_amplitude = du->dtovr.amplitude;
        du->dtovr.x0 = du->dtovr.threshold * (ADC_PREC_VALUE/256);
        du->dtovr.y0 = du->dtovr.amplitude * (ADC_PREC_VALUE/256);
        du->dtovr.y0x0 = du->dtovr.y0 * QUANTIZATION_MAX / du->dtovr.x0;
        du->dtovr.y01 = (ADC_PREC_VALUE - du->dtovr.y0) * QUANTIZATION_MAX / (ADC_PREC_VALUE - du->dtovr.x0);
    }
    int32_t absample = (sample < 0) ? -sample : sample;
    if (absample <= du->dtovr.x0)
    {
        absample = fractional_int_remove_offset(absample * du->dtovr.y0x0);
    } else
    {
        absample = fractional_int_remove_offset((absample - du->dtovr.x0) * du->dtovr.y01) + du->dtovr.y0;
    }
    if (absample > (ADC_PREC_VALUE/2-1)) absample = (ADC_PREC_VALUE/2-1);
    return (sample < 0) ? -absample : absample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_overdrive[] = 
{
    { "Threshold",    offsetof(dsp_type_overdrive,threshold),               4, 3, 1, 255 },
    { "Amplitude",    offsetof(dsp_type_overdrive,amplitude),               4, 3, 1, 255 },
    { "ThrshCntrl",   offsetof(dsp_type_overdrive,control_number1),         4, 1, 0, 6 },
    { "AmplCntrl",    offsetof(dsp_type_overdrive,control_number2),         4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_overdrive,source_unit),             4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_overdrive dsp_type_overdrive_default = { 0, 0, 64, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_COMPRESSOR*************************************/

int32_t dsp_type_process_compressor(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtcomp.control_number1);
    if (abs(new_input - du->dtcomp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtcomp.pot_value1 = new_input;
        du->dtcomp.attack = (new_input * 256) / POT_MAX_VALUE; 
    }
    new_input = read_potentiometer_value(du->dtcomp.control_number2);
    if (abs(new_input - du->dtcomp.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtcomp.pot_value2 = new_input;
        du->dtcomp.release = (new_input * 256) / POT_MAX_VALUE; 
    }

    sample = (sample * du->dtcomp.gain) / 4096;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    
    du->dtcomp.level = (du->dtcomp.level*511)/512 + abs(sample);
    du->dtcomp.nlevel = du->dtcomp.level / 512;
    
    if ((++du->dtcomp.skip)>=4)
    {
        du->dtcomp.skip = 0;
        if (du->dtcomp.nlevel > du->dtcomp.release_threshold)
            du->dtcomp.gain -= (du->dtcomp.gain * du->dtcomp.release) / 65536;
        else if (du->dtcomp.nlevel < du->dtcomp.attack_threshold)
            du->dtcomp.gain += (du->dtcomp.gain * du->dtcomp.attack) / 65536;
    }
    
    if (du->dtcomp.gain < 4096) du->dtcomp.gain = 4096;
    if (du->dtcomp.gain > 16384) du->dtcomp.gain = 16384;

    return sample;
}
const dsp_type_configuration_entry dsp_type_configuration_entry_compressor[] = 
{
    //{ "Gain",         offsetof(dsp_type_compressor,gain),                    4, 5, 1, 25555 },
    //{ "Level",        offsetof(dsp_type_compressor,nlevel),                   4, 5, 0, 60000 },
    { "Attack",       offsetof(dsp_type_compressor,attack),                  4, 3, 16, 255 },
    { "Release",      offsetof(dsp_type_compressor,release),                 4, 3, 16, 255 },
    { "AtkThresh",    offsetof(dsp_type_compressor,attack_threshold),        4, 5, 0, ADC_PREC_VALUE },
    { "RlsThresh",    offsetof(dsp_type_compressor,release_threshold),       4, 5, 0, ADC_PREC_VALUE },
    { "AtkCtrl",      offsetof(dsp_type_compressor,control_number1),         4, 1, 0, 6 },
    { "RlsCtrl",      offsetof(dsp_type_compressor,control_number2),         4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_compressor,source_unit),             4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_compressor dsp_type_compressor_default = { 0, 0, 40, 400, 100, 3000, 16384, 0, 0, 0, 0, 0, 0 ,0 };

/************************************DSP_TYPE_RING*************************************/

int32_t dsp_type_process_ring(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtring.control_number1);
    if (abs(new_input - du->dtring.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtring.pot_value1 = new_input;
        du->dtring.frequency = 1 + new_input/(POT_MAX_VALUE/256);
    }
    if (du->dtring.frequency != du->dtring.last_frequency)
    {
        du->dtring.last_frequency = du->dtring.frequency;
        du->dtring.sine_counter_inc = du->dtring.frequency;
    }

    du->dtring.sine_counter += du->dtring.sine_counter_inc;
    int32_t sine_val = sine_table_entry((du->dtring.sine_counter & 0xFFFF00) / 4096);
    if (!du->dtring.sine_mix)
    {
        sine_val *= 4;
        if (sine_val >= (QUANTIZATION_MAX-1)) sine_val = QUANTIZATION_MAX - 1;
        if (sine_val < (-QUANTIZATION_MAX)) sine_val = -QUANTIZATION_MAX;
    }
    sample = fractional_int_remove_offset(sample * sine_val);
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_ring[] = 
{
    { "Speed",        offsetof(dsp_type_ring,frequency),       4, 3, 1, 4095 },
    { "SineMix",      offsetof(dsp_type_ring,sine_mix),        4, 1, 0, 1 },
    { "SpeedCntrl",   offsetof(dsp_type_ring,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_ring,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_ring dsp_type_ring_default = { 0, 0, 0, 0, 60, 1, 0, 0, 0 };


/************************************DSP_TYPE_FLANGE*************************************/

int32_t dsp_type_process_flange(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtflng.control_number1);
    if (abs(new_input - du->dtflng.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtflng.pot_value1 = new_input;
        du->dtflng.frequency = 1 + new_input/(POT_MAX_VALUE/256);
    }
    new_input = read_potentiometer_value(du->dtflng.control_number2);
    if (abs(new_input - du->dtflng.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtflng.pot_value2 = new_input;
        du->dtflng.modulation = (du->dtflng.pot_value2 * 256) / POT_MAX_VALUE;
    }
    if (du->dtflng.frequency != du->dtflng.last_frequency)
    {
        du->dtflng.last_frequency = du->dtflng.frequency;
        du->dtflng.sine_counter_inc = du->dtflng.frequency; // (du->dtflng.frequency * 65536) / GUITARPICO_SAMPLERATE;
    }
    du->dtflng.sine_counter += du->dtflng.sine_counter_inc;
    int32_t sine_val = sine_table_entry((du->dtflng.sine_counter & 0xFFFF00) / 4096);
    int32_t mod_val = ((sine_val * du->dtflng.modulation) + QUANTIZATION_MAX * 256) / 512;
    uint32_t delay_samples = (du->dtflng.delay_samples * mod_val) / QUANTIZATION_MAX;
    sample = (sample_circ_buf_value(delay_samples) * ((int32_t)du->dtflng.feedback) + sample * ((int32_t)(255 - du->dtflng.feedback))) / 256;
    return sample;

}

const dsp_type_configuration_entry dsp_type_configuration_entry_flange[] = 
{
    { "Speed",        offsetof(dsp_type_flange,frequency),       4, 3, 1, 4095 },
    { "Modulation",   offsetof(dsp_type_flange,modulation),      4, 3, 0, 255 },
    { "Samples",      offsetof(dsp_type_flange,delay_samples),   4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "Feedback",     offsetof(dsp_type_flange,feedback),        4, 3, 0, 255 },
    { "SpeedCntrl",   offsetof(dsp_type_flange,control_number1), 4, 1, 0, 6 },
    { "ModCntrl",     offsetof(dsp_type_flange,control_number2), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_flange,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_flange dsp_type_flange_default = { 0, 0, 0, 0, 70, 32, 0,  255, 0, 128, 0, 0, 0, 0 };

/************************************DSP_TYPE_CHORUS*************************************/

int32_t dsp_type_process_chorus(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtchor.control_number1);
    if (abs(new_input - du->dtchor.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtchor.pot_value1 = new_input;
        du->dtchor.frequency = 1 + new_input/(POT_MAX_VALUE/256);
    }
    new_input = read_potentiometer_value(du->dtchor.control_number2);
    if (abs(new_input - du->dtchor.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtchor.pot_value2 = new_input;
        du->dtchor.modulation = (du->dtchor.pot_value2 * 256) / POT_MAX_VALUE;
    }
    if (du->dtchor.frequency != du->dtchor.last_frequency)
    {
        du->dtchor.last_frequency = du->dtchor.frequency;
        du->dtchor.sine_counter_inc = du->dtchor.frequency; // (du->dtchor.frequency * 65536) / GUITARPICO_SAMPLERATE;
    }
    du->dtchor.sine_counter += du->dtchor.sine_counter_inc;
    int32_t sine_val = sine_table_entry((du->dtchor.sine_counter & 0xFFFF00) / 4096);
    int32_t mod_val = ((sine_val * du->dtchor.modulation) + QUANTIZATION_MAX * 256) / 512;
    uint32_t delay_samples = (du->dtchor.delay_samples * mod_val) / QUANTIZATION_MAX;
    sample = (sample_circ_buf_clean_value(delay_samples) * ((int32_t)du->dtchor.mixval) + sample * ((int32_t)(255 - du->dtchor.mixval))) / 256;
    return sample;

}

const dsp_type_configuration_entry dsp_type_configuration_entry_chorus[] = 
{
    { "Speed",        offsetof(dsp_type_chorus,frequency),       4, 3, 1, 4095 },
    { "Modulation",   offsetof(dsp_type_chorus,modulation),      4, 3, 0, 255 },
    { "Samples",      offsetof(dsp_type_chorus,delay_samples),   4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "Mixval",       offsetof(dsp_type_chorus,mixval),          4, 3, 0, 255 },
    { "SpeedCntrl",   offsetof(dsp_type_chorus,control_number1), 4, 1, 0, 6 },
    { "ModCntrl",     offsetof(dsp_type_chorus,control_number2), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_chorus,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_chorus dsp_type_chorus_default = { 0, 0, 0, 0, 170, 32, 0,  128, 0, 200, 0, 0, 0, 0 };

/************************************DSP_TYPE_PHASER*************************************/

int32_t dsp_type_process_phaser(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtphaser.control_number1);
    if (abs(new_input - du->dtphaser.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtphaser.pot_value1 = new_input;
        du->dtphaser.frequency = 1 + new_input/(POT_MAX_VALUE/256);
    }
    if (du->dtphaser.frequency != du->dtphaser.last_frequency)
    {
        du->dtphaser.last_frequency = du->dtphaser.frequency;
        du->dtphaser.sine_counter_inc = du->dtphaser.frequency / 2; // (du->dtphaser.frequency * 65536) / GUITARPICO_SAMPLERATE;
    }
    if ((du->dtphaser.freq1 != du->dtphaser.last_freq1) || (du->dtphaser.freq2 != du->dtphaser.last_freq2) || (du->dtphaser.Q != du->dtphaser.last_Q))
    {
        du->dtphaser.last_freq1 = du->dtphaser.freq1;
        du->dtphaser.last_freq2 = du->dtphaser.freq2;
        du->dtphaser.last_Q = du->dtphaser.Q;
        
        float w1 = nyquist_fraction_omega(du->dtphaser.freq1);
        float w2 = nyquist_fraction_omega(du->dtphaser.freq2);
        if (w1 > w2)
        {
            float temp = w1;
            w1 = w2;
            w2 = temp;
        }
        float a = float_a_value(w1, du->dtphaser.Q);
        float bfpa0 = 0.999f/(1.0f+a);
        du->dtphaser.filta1_interp1 = du->dtphaser.filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtphaser.filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtphaser.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    
    du->dtphaser.sine_counter += du->dtphaser.sine_counter_inc;
    int32_t sine_val = QUANTIZATION_MAX - 1 - abs(sine_table_entry((du->dtphaser.sine_counter & 0xFFFF0) / 4096));
    du->dtphaser.filta1 = du->dtphaser.filta1_interp1 + ((du->dtphaser.filta1_interp2 - du->dtphaser.filta1_interp1) * 
                            sine_val) / QUANTIZATION_MAX;

    int32_t filtout = sample;
    for (uint stage=0;stage<du->dtphaser.stages;stage++)
    {
        int32_t last_filtout = filtout;
        filtout =     ((int32_t)du->dtphaser.filta2) * (((int32_t)last_filtout) - ((int32_t)du->dtphaser.filtdly2[stage]))
                            + ((int32_t)du->dtphaser.filta1) * (((int32_t)du->dtphaser.sampledly1[stage]) - ((int32_t)du->dtphaser.filtdly1[stage]))
                            + ((int32_t)du->dtphaser.sampledly2[stage]) * float_to_sampled_int(0.999f);
               
        filtout = fractional_int_remove_offset(filtout);
        du->dtphaser.sampledly2[stage] = du->dtphaser.sampledly1[stage];
        du->dtphaser.sampledly1[stage] = last_filtout;
        du->dtphaser.filtdly2[stage] = du->dtphaser.filtdly1[stage];
        du->dtphaser.filtdly1[stage] = filtout;
    }
    filtout = (filtout * ((int32_t)du->dtphaser.mixval) + sample * ((int32_t)(255 - du->dtphaser.mixval))) / 256;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_phaser[] = 
{
    { "Freq1",        offsetof(dsp_type_phaser,freq1),           2, 4, 100, 2000 },
    { "Freq2",        offsetof(dsp_type_phaser,freq2),           2, 4, 100, 2000 },
    { "Q",            offsetof(dsp_type_phaser,Q),               2, 3, 50, 999 },
    { "Speed",        offsetof(dsp_type_phaser,frequency),       4, 4, 1, 4095 },
    { "Stages",       offsetof(dsp_type_phaser,stages),          4, 1, 2, PHASER_STAGES },
    { "Mixval",       offsetof(dsp_type_phaser,mixval),          4, 3, 0, 255 },
    { "SpeedCntrl",   offsetof(dsp_type_phaser,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_phaser,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t freq1, freq2;
    uint16_t Q;
    uint32_t frequency;
    uint32_t mixval;
    uint32_t stages;
    uint32_t last_frequency;
    uint16_t last_freq1, last_freq2;
    uint16_t last_Q;
    int32_t filta1_interp1;
    int32_t filta1_interp2;
    int32_t filta1;
    int32_t filta2;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t control_number1;
    uint32_t pot_value1;

const dsp_type_phaser dsp_type_phaser_default = { 0, 0, 300, 600, 200, 60, 128, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
{ 0, 0, 0, 0, 0, 0, 0, 0 },      { 0, 0, 0, 0, 0, 0, 0, 0 },     { 0, 0, 0, 0, 0, 0, 0, 0 },     { 0, 0, 0, 0, 0, 0, 0, 0 } };

/************************************DSP_TYPE_BACKWARDS*********************************/

int32_t dsp_type_process_backwards(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtback.control_number1);
    if (abs(new_input - du->dtback.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtback.pot_value1 = new_input;
        du->dtback.backwards_samples = (du->dtback.pot_value1 * SAMPLE_CIRC_BUF_SIZE) / POT_MAX_VALUE;
    }
    new_input = read_potentiometer_value(du->dtback.control_number2);
    if (abs(new_input - du->dtback.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtback.pot_value2 = new_input;
        du->dtback.balance = (du->dtback.pot_value2 * 256) / POT_MAX_VALUE;
    }
    du->dtback.samples_count = (du->dtback.samples_count == 0) ? du->dtback.backwards_samples : (du->dtback.samples_count-1);
    sample = (sample * ((int32_t)(255 - du->dtback.balance)) + 
                       ((int32_t)sample_circ_buf_clean_value(du->dtback.samples_count)) * ((int32_t)du->dtback.balance)) / 256;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_backwards[] = 
{
    { "Samples",    offsetof(dsp_type_backwards,backwards_samples),   4, 5, 1, SAMPLE_CIRC_BUF_CLEAN_SIZE },
    { "Balance",     offsetof(dsp_type_backwards,balance),             4, 3, 0, 255 },
    { "SampCtrl",   offsetof(dsp_type_backwards,control_number1), 4, 1, 0, 6 },
    { "BalCtrl",   offsetof(dsp_type_backwards,control_number2), 4, 1, 0, 6 },
    { "SourceUnit", offsetof(dsp_type_backwards,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_backwards dsp_type_backwards_default = { 0, 0, 2000, 255, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_PITCHSHIFT*********************************/

int32_t dsp_type_process_pitchshift(int32_t sample, dsp_unit *du)
{
    uint32_t new_input = read_potentiometer_value(du->dtpitch.control_number2);
    if (abs(new_input - du->dtpitch.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtpitch.pot_value2 = new_input;
        du->dtpitch.balance = (du->dtpitch.pot_value2 * 256) / POT_MAX_VALUE;
    }
    new_input = read_potentiometer_value(du->dtpitch.control_number3);
    if (abs(new_input - du->dtpitch.pot_value3) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtpitch.pot_value3 = new_input;
        du->dtpitch.pitchshift_rate = (du->dtpitch.pot_value3 * 16384) / POT_MAX_VALUE;
    }
    if (du->dtpitch.pitchshift_samples != du->dtpitch.last_pitchshift_samples)
    {
        du->dtpitch.last_pitchshift_samples = du->dtpitch.pitchshift_samples;
        du->dtpitch.pitchshift_samples_2 = du->dtpitch.pitchshift_samples * 2;
        du->dtpitch.pitchshift_samples_12 = du->dtpitch.pitchshift_samples / 2;
        du->dtpitch.pitchshift_samples_32 = du->dtpitch.pitchshift_samples * 3 / 2;
        du->dtpitch.pitchshift_samples_4096 = du->dtpitch.pitchshift_samples * 4096;
        du->dtpitch.pitchshift_samples_8192 = du->dtpitch.pitchshift_samples * 8192;
        du->dtpitch.pitchshift_samples_scale = 32768 / du->dtpitch.pitchshift_samples;
        du->dtpitch.samples_count = du->dtpitch.pitchshift_samples_4096;
    }
    if ((du->dtpitch.frequency != du->dtpitch.last_frequency) || (du->dtpitch.Q != du->dtpitch.last_Q))
    {
        du->dtpitch.last_frequency = du->dtpitch.frequency;
        du->dtpitch.last_Q = du->dtpitch.Q;
        float w0 = nyquist_fraction_omega(du->dtpitch.frequency);
        float c0 = cosf(w0);
        float a = float_a_value(w0,du->dtpitch.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtpitch.filtb1 = float_to_sampled_int((1.0f-c0)*bfpa0);
        du->dtpitch.filtb0 = float_to_sampled_int(0.5*(1.0f-c0)*bfpa0);
        du->dtpitch.filta1 = float_to_sampled_int(-2.0f*c0*bfpa0);
        du->dtpitch.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    du->dtpitch.samples_count += (4096 - ((int32_t)du->dtpitch.pitchshift_rate));
    if (du->dtpitch.samples_count < 0)  du->dtpitch.samples_count += du->dtpitch.pitchshift_samples_4096;
    if (du->dtpitch.samples_count >= du->dtpitch.pitchshift_samples_8192) du->dtpitch.samples_count -= du->dtpitch.pitchshift_samples_4096;
    int32_t current_sample = du->dtpitch.samples_count / 4096, val;
    if (current_sample < du->dtpitch.pitchshift_samples_12)
    {
        val = ((sample_circ_buf_clean_value(current_sample)*current_sample + 
               sample_circ_buf_clean_value(current_sample + du->dtpitch.pitchshift_samples_12)*(du->dtpitch.pitchshift_samples_12 - current_sample))*
               du->dtpitch.pitchshift_samples_scale) / 16384;
    } else if (current_sample < du->dtpitch.pitchshift_samples)
    {
        val = ((sample_circ_buf_clean_value(current_sample)*(du->dtpitch.pitchshift_samples - current_sample) + 
               sample_circ_buf_clean_value(current_sample - du->dtpitch.pitchshift_samples_12)*(current_sample - du->dtpitch.pitchshift_samples_12))*
               du->dtpitch.pitchshift_samples_scale) / 16384;
    } else if (current_sample < du->dtpitch.pitchshift_samples_32)
    {
        val = ((sample_circ_buf_clean_value(current_sample)*(current_sample - du->dtpitch.pitchshift_samples) + 
               sample_circ_buf_clean_value(current_sample + du->dtpitch.pitchshift_samples_12)*(du->dtpitch.pitchshift_samples_32 - current_sample))*
               du->dtpitch.pitchshift_samples_scale) / 16384;
    } else
    {
        val = ((sample_circ_buf_clean_value(current_sample)*(du->dtpitch.pitchshift_samples_2 - current_sample) + 
               sample_circ_buf_clean_value(current_sample - du->dtpitch.pitchshift_samples_12)*(current_sample - du->dtpitch.pitchshift_samples_32))*
               du->dtpitch.pitchshift_samples_scale) / 16384;
    }
    sample = (sample * ((int32_t)(255 - du->dtpitch.balance)) + val * ((int32_t)du->dtpitch.balance)) / 256;
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    
    int32_t filtout =    ((int32_t)du->dtpitch.filtb0) * ((int32_t)sample + (int32_t)du->dtpitch.sampledly2)
                       + ((int32_t)du->dtpitch.filtb1) * ((int32_t)du->dtpitch.sampledly1)  
                       - ((int32_t)du->dtpitch.filta1) * ((int32_t)du->dtpitch.filtdly1)
                       - ((int32_t)du->dtpitch.filta2) * ((int32_t)du->dtpitch.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtpitch.sampledly2 = du->dtpitch.sampledly1;
    du->dtpitch.sampledly1 = sample;
    du->dtpitch.filtdly2 = du->dtpitch.filtdly1;
    du->dtpitch.filtdly1 = filtout;
    return filtout;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_pitchshift[] = 
{
    { "Samples",    offsetof(dsp_type_pitchshift,pitchshift_samples),   4, 5, 1, 2048 },
    { "Rate",       offsetof(dsp_type_pitchshift,pitchshift_rate),      4, 5, 1, 16384 },
    { "Balance",    offsetof(dsp_type_pitchshift,balance),             4, 3, 0, 255 },
    { "Frequency",   offsetof(dsp_type_pitchshift,frequency),       2, 4, 100, 4000 },
    { "Q",           offsetof(dsp_type_pitchshift,Q),               2, 3, 50, 999 },
    { "RateCtrl",   offsetof(dsp_type_pitchshift,control_number3),  4, 1, 0, 6 },
    { "BalCtrl",   offsetof(dsp_type_pitchshift,control_number2),  4, 1, 0, 6 },
    { "SourceUnit", offsetof(dsp_type_pitchshift,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_pitchshift dsp_type_pitchshift_default = { 0, 0, 800, 4096, 255, 2000, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_OCTAVE*********************************/

int32_t dsp_type_process_octave(int32_t sample, dsp_unit *du)
{
    if (du->dtoct.rectify) 
    {
        sample = abs(sample);
        du->dtoct.sample_avg = (du->dtoct.sample_avg*511)/512 + sample;
        sample -= (du->dtoct.sample_avg / 512);
    }
    
    if (du->dtoct.multiplier > 1)
    {
        int32_t multval = sample * ((int32_t)du->dtoct.multiplier);
        if (multval >= 0)
        {
            uint32_t rem = multval / (ADC_PREC_VALUE/2);
            multval = ((int32_t)(((uint32_t)multval) % (ADC_PREC_VALUE/2)));
            sample = ((rem & 0x01) == 1) ? ((ADC_PREC_VALUE/2) - 1 )- multval : multval;
        } else
        {
            multval = -multval;
            uint32_t rem = ((uint32_t)multval) / (ADC_PREC_VALUE/2);
            multval = ((int32_t)((uint32_t)multval) % (ADC_PREC_VALUE/2));
            sample = ((rem & 0x01) == 1) ? -(((ADC_PREC_VALUE/2) - 1 )- multval) : -multval;
        }
        du->dtoct.sample_avg2 = (du->dtoct.sample_avg2*511)/512 + sample;
        sample -= (du->dtoct.sample_avg2 / 512);
    }
    
    if (sample > (ADC_PREC_VALUE/2-1)) sample=ADC_PREC_VALUE/2-1;
    if (sample < (-ADC_PREC_VALUE/2)) sample=-ADC_PREC_VALUE/2;
    
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_octave[] = 
{
    { "Rectify",    offsetof(dsp_type_octave,rectify),   4, 1, 0, 1},
    { "Multplr",    offsetof(dsp_type_octave,multiplier), 4, 2, 1, 99 },
    { "SourceUnit", offsetof(dsp_type_octave,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_octave dsp_type_octave_default = { 0, 0, 1, 1, 0, 0 };

/************STRUCTURES FOR ALL DSP TYPES *****************************/

const char * const dtnames[] = 
{
    "None",
    "NoiseGate",
    "Delay",
    "Room",
    "Combine",
    "Bandpass",
    "LowPass",
    "HighPass",
    "AllPass",
    "Tremolo",
    "Vibrato",
    "Wah",
    "AutoWah",
    "Envelope",
    "Distortion",
    "Overdrive",
    "Compressor",
    "Ring",
    "Flanger",
    "Chorus",
    "Phaser",
    "Backwards",
    "PitchShift",
    "Octave",
    "Sin Synth",
    NULL
};

const dsp_type_configuration_entry * const dtce[] = 
{
    dsp_type_configuration_entry_none,
    dsp_type_configuration_entry_noisegate, 
    dsp_type_configuration_entry_delay, 
    dsp_type_configuration_entry_room, 
    dsp_type_configuration_entry_combine, 
    dsp_type_configuration_entry_bandpass, 
    dsp_type_configuration_entry_lowpass, 
    dsp_type_configuration_entry_highpass, 
    dsp_type_configuration_entry_allpass, 
    dsp_type_configuration_entry_tremolo, 
    dsp_type_configuration_entry_vibrato, 
    dsp_type_configuration_entry_wah, 
    dsp_type_configuration_entry_autowah, 
    dsp_type_configuration_entry_envelope, 
    dsp_type_configuration_entry_distortion, 
    dsp_type_configuration_entry_overdrive, 
    dsp_type_configuration_entry_compressor, 
    dsp_type_configuration_entry_ring, 
    dsp_type_configuration_entry_flange, 
    dsp_type_configuration_entry_chorus, 
    dsp_type_configuration_entry_phaser, 
    dsp_type_configuration_entry_backwards, 
    dsp_type_configuration_entry_pitchshift, 
    dsp_type_configuration_entry_octave, 
    dsp_type_configuration_entry_sin_synth, 
    NULL
};

dsp_type_process * const dtp[] = {
    dsp_type_process_none,
    dsp_type_process_noisegate,
    dsp_type_process_delay,
    dsp_type_process_room,
    dsp_type_process_combine,
    dsp_type_process_bandpass,
    dsp_type_process_lowpass,
    dsp_type_process_highpass,
    dsp_type_process_allpass,
    dsp_type_process_tremolo,
    dsp_type_process_vibrato,
    dsp_type_process_wah,
    dsp_type_process_autowah,
    dsp_type_process_envelope,
    dsp_type_process_distortion,
    dsp_type_process_overdrive,
    dsp_type_process_compressor,
    dsp_type_process_ring,
    dsp_type_process_flange,
    dsp_type_process_chorus,
    dsp_type_process_phaser,
    dsp_type_process_backwards,
    dsp_type_process_pitchshift,
    dsp_type_process_octave,
    dsp_type_process_sin_synth,
};

const void * const dsp_unit_struct_defaults[] =
{
    (void *) &dsp_type_none_default,
    (void *) &dsp_type_noisegate_default,
    (void *) &dsp_type_delay_default,
    (void *) &dsp_type_room_default,
    (void *) &dsp_type_combine_default,
    (void *) &dsp_type_bandpass_default,
    (void *) &dsp_type_lowpass_default,
    (void *) &dsp_type_highpass_default,
    (void *) &dsp_type_allpass_default,
    (void *) &dsp_type_tremolo_default,
    (void *) &dsp_type_vibrato_default,
    (void *) &dsp_type_wah_default,
    (void *) &dsp_type_autowah_default,
    (void *) &dsp_type_envelope_default,
    (void *) &dsp_type_distortion_default,
    (void *) &dsp_type_overdrive_default,
    (void *) &dsp_type_compressor_default,
    (void *) &dsp_type_ring_default,
    (void *) &dsp_type_flange_default,
    (void *) &dsp_type_chorus_default,
    (void *) &dsp_type_phaser_default,
    (void *) &dsp_type_backwards_default,
    (void *) &dsp_type_pitchshift_default,
    (void *) &dsp_type_octave_default,
    (void *) &dsp_type_sine_synth_default
};

/********************* DSP PROCESS STRUCTURE *******************************************/

uint32_t dsp_read_value_prec(void *v, int prec)
{
    if (prec == 1)
        return *((uint8_t *)v);
    if (prec == 2)
        return *((uint16_t *)v);
    return *((uint32_t *)v);
}

void dsp_set_value_prec(void *v, int prec, uint32_t val)
{
    DMB();
    if (prec == 1)
    {
        *((uint8_t *)v) = val;
        return;
    } else if (prec == 2)
    {
        *((uint16_t *)v) = val;
        return;
    } else
    {
        *((uint32_t *)v) = val;
    }
    DMB();

}

void dsp_unit_initialize(int dsp_unit_number, dsp_unit_type dut)
{
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    
    if (dut >= DSP_TYPE_MAX_ENTRY) return;
    
    dsp_unit_struct_zero(du);
    memcpy((void *)du, dsp_unit_struct_defaults[dut], sizeof(dsp_unit));
    du->dtn.source_unit = dsp_unit_number + 1;
    DMB();
    du->dtn.dut = dut;
    DMB();
}

void dsp_unit_reset(int dsp_unit_number)
{
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    
    dsp_unit temp = *du;
    dsp_unit_struct_zero(du);
    memcpy((void *)du, dsp_unit_struct_defaults[temp.dtn.dut], sizeof(dsp_unit));
    du->dtn.dut = temp.dtn.dut;
    du->dtn.source_unit = temp.dtn.source_unit;
    const dsp_type_configuration_entry *dtce_l = dtce[temp.dtn.dut];
    while (dtce_l->desc != NULL)
    {
       
       uint32_t value = dsp_read_value_prec((void *)(((uint8_t *)&temp) + dtce_l->offset), dtce_l->size);
       dsp_set_value_prec((void *)(((uint8_t *)du) + dtce_l->offset), dtce_l->size, value); 
       dtce_l++;
    }
}

void dsp_unit_reset_all(void)
{
    for (int i=0;i<MAX_DSP_UNITS;i++)
        dsp_unit_reset(i);
}

static inline int32_t dsp_process(int32_t sample, dsp_unit *du)
{
    return dtp[(int)du->dtn.dut](sample, du);
}

void initialize_dsp(void)
{
    initialize_sine_table();
    initialize_sample_circ_buf();
    for (int unit_number=0;unit_number<MAX_DSP_UNITS;unit_number++) 
        dsp_unit_initialize(unit_number, DSP_TYPE_NONE);
}

int32_t dsp_process_all_units(int32_t sample)
{
    dsp_unit_result[0] = sample;
    for (int unit_no=0;unit_no<MAX_DSP_UNITS;unit_no++)
    {
        dsp_unit *du = dsp_unit_entry(unit_no);
        dsp_unit_result[unit_no+1] = dsp_process(dsp_unit_result[du->dtn.source_unit-1], du);
    }
    return dsp_unit_result[MAX_DSP_UNITS];
}

dsp_unit_type dsp_unit_get_type(uint dsp_unit_number)
{
    if (dsp_unit_number >= MAX_DSP_UNITS) return DSP_TYPE_MAX_ENTRY;
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    return du->dtn.dut;
}

const dsp_type_configuration_entry *dsp_unit_get_configuration_entry(uint dsp_unit_number, uint num)
{
    if (dsp_unit_number >= MAX_DSP_UNITS) return NULL;
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    const dsp_type_configuration_entry *dtce_l = dtce[du->dtn.dut];
    while (dtce_l->desc != NULL)
    {
        if (num == 0) return dtce_l;
        num--;
        dtce_l++;
    }
    return NULL;
}

bool dsp_unit_set_value(uint dsp_unit_number, const char *desc, uint32_t value)
{
    if (dsp_unit_number >= MAX_DSP_UNITS) return NULL;
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    const dsp_type_configuration_entry *dtce_l = dtce[du->dtn.dut];
    while (dtce_l->desc != NULL)
    {
        if (!strcmp(dtce_l->desc,desc))
        {
           if ((value >= dtce_l->minval) && (value <= dtce_l->maxval))
           {
                dsp_set_value_prec((void *)(((uint8_t *)dsp_unit_entry(dsp_unit_number)) + dtce_l->offset), dtce_l->size, value); 
                return true;
           } else return false;
            
        }
        dtce_l++;
    }
    return false;
}

bool dsp_unit_get_value(uint dsp_unit_number, const char *desc, uint32_t *value)
{
    if (dsp_unit_number >= MAX_DSP_UNITS) return NULL;
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    const dsp_type_configuration_entry *dtce_l = dtce[du->dtn.dut];
    while (dtce_l->desc != NULL)
    {
        if (!strcmp(dtce_l->desc,desc))
        {
           uint32_t v = dsp_read_value_prec((void *)(((uint8_t *)dsp_unit_entry(dsp_unit_number)) + dtce_l->offset), dtce_l->size);
           *value = v;
           return true;
        }
        dtce_l++;
    }
    return false;
}
