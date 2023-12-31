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
    uint32_t new_input = read_potentiometer_value(du->dtss.control_number);
    if (abs(new_input - du->dtss.pot_value) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtss.pot_value = new_input;
        du->dtss.frequency = 40 + new_input/(POT_MAX_VALUE/2048);
    }
    if (du->dtss.frequency != du->dtss.last_frequency)
    {
        du->dtss.last_frequency = du->dtss.frequency;
        du->dtss.sine_counter_inc = (du->dtss.frequency*65536) / GUITARPICO_SAMPLERATE;
    }
    du->dtss.sine_counter += du->dtss.sine_counter_inc;
    int16_t sine_val = sine_table_entry((du->dtss.sine_counter & 0xFFFF) / 256) / (QUANTIZATION_MAX / (ADC_PREC_VALUE/2));
    return sine_val;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_sin_synth[] = 
{
    { "Freqn",    offsetof(dsp_type_sine_synth,sine_counter_inc), 4, 4, 100, 4000 },
    { "FreqCtrl", offsetof(dsp_type_sine_synth,control_number),   4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_sine_synth dsp_type_sine_synth_default = { 0, 0, 0, 3000, 0, 0, 0, 0 };

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
    { "Ampltiude",    offsetof(dsp_type_overdrive,amplitude),               4, 3, 1, 255 },
    { "ThrshCntrl",   offsetof(dsp_type_overdrive,control_number1),         4, 1, 0, 6 },
    { "AmplCntrl",    offsetof(dsp_type_overdrive,control_number2),         4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_overdrive,source_unit),             4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_overdrive dsp_type_overdrive_default = { 0, 0, 64, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
    sample = (sample_circ_buf_clean_value(delay_samples) * ((int32_t)du->dtflng.mixval) + sample * ((int32_t)(255 - du->dtflng.mixval))) / 256;
    return sample;

}

const dsp_type_configuration_entry dsp_type_configuration_entry_flange[] = 
{
    { "Speed",        offsetof(dsp_type_flange,frequency),       4, 3, 1, 4095 },
    { "Modulation",   offsetof(dsp_type_flange,modulation),      4, 3, 0, 255 },
    { "Samples",      offsetof(dsp_type_flange,delay_samples),   4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "Mixval",       offsetof(dsp_type_flange,mixval),          4, 3, 0, 255 },
    { "SpeedCntrl",   offsetof(dsp_type_flange,control_number1), 4, 1, 0, 6 },
    { "ModCntrl",     offsetof(dsp_type_flange,control_number2), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_flange,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_flange dsp_type_flange_default = { 0, 0, 0, 0, 70, 96, 0, 128, 0, 128, 0, 0, 0, 0 };

/************************************DSP_TYPE_PHASER*************************************/

int32_t dsp_type_process_phaser(int32_t sample, dsp_unit *du)
{
    int32_t filtout, filtout2;
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
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtphaser.a_filta1_interp1 = du->dtphaser.a_filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtphaser.a_filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtphaser.a_filta2 = float_to_sampled_int((1.0f-a)*bfpa0);

        w1 = nyquist_fraction_omega(du->dtphaser.freq1*3/2);
        w2 = nyquist_fraction_omega(du->dtphaser.freq2*3/2);
        if (w1 > w2)
        {
            float temp = w1;
            w1 = w2;
            w2 = temp;
        }
        a = float_a_value(w1, du->dtphaser.Q);
        bfpa0 = 1.0f/(1.0f+a);
        du->dtphaser.b_filta1_interp1 = du->dtphaser.b_filta1 = float_to_sampled_int(-2.0f*cosf(w1)*bfpa0);
        du->dtphaser.b_filta1_interp2 = float_to_sampled_int(-2.0f*cosf(w2)*bfpa0);
        du->dtphaser.b_filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    du->dtphaser.sine_counter += du->dtphaser.sine_counter_inc;
    int32_t sine_val = QUANTIZATION_MAX - 1 - abs(sine_table_entry((du->dtphaser.sine_counter & 0xFFFF0) / 4096));
    du->dtphaser.a_filta1 = du->dtphaser.a_filta1_interp1 + ((du->dtphaser.a_filta1_interp2 - du->dtphaser.a_filta1_interp1) * 
                            sine_val) / QUANTIZATION_MAX;

    filtout =    ((int32_t)du->dtphaser.a_filta2) * (((int32_t)sample) - ((int32_t)du->dtphaser.a_filtdly2))
               + ((int32_t)du->dtphaser.a_filta1) * (((int32_t)du->dtphaser.a_sampledly1) - ((int32_t)du->dtphaser.a_filtdly1))
               + ((int32_t)du->dtphaser.a_sampledly2) * float_to_sampled_int(1.0f);
               
    filtout = fractional_int_remove_offset(filtout);
    du->dtphaser.a_sampledly2 = du->dtphaser.a_sampledly1;
    du->dtphaser.a_sampledly1 = sample;
    du->dtphaser.a_filtdly2 = du->dtphaser.a_filtdly1;
    du->dtphaser.a_filtdly1 = filtout;

    du->dtphaser.b_filta1 = du->dtphaser.b_filta1_interp1 + ((du->dtphaser.b_filta1_interp2 - du->dtphaser.b_filta1_interp1) * 
                            sine_val) / QUANTIZATION_MAX;

    filtout2 =   ((int32_t)du->dtphaser.b_filta2) * (((int32_t)filtout) - ((int32_t)du->dtphaser.b_filtdly2))
               + ((int32_t)du->dtphaser.b_filta1) * (((int32_t)du->dtphaser.b_sampledly1) - ((int32_t)du->dtphaser.b_filtdly1))
               + ((int32_t)du->dtphaser.b_sampledly2) * float_to_sampled_int(1.0f);

    filtout2 = fractional_int_remove_offset(filtout2);
    du->dtphaser.b_sampledly2 = du->dtphaser.b_sampledly1;
    du->dtphaser.b_sampledly1 = filtout;
    du->dtphaser.b_filtdly2 = du->dtphaser.b_filtdly1;
    du->dtphaser.b_filtdly1 = filtout2;

    filtout2 = (filtout2 * ((int32_t)du->dtphaser.mixval) + sample * ((int32_t)(255 - du->dtphaser.mixval))) / 256;

    return filtout2;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_phaser[] = 
{
    { "Freq1",        offsetof(dsp_type_phaser,freq1),           2, 4, 100, 4000 },
    { "Freq2",        offsetof(dsp_type_phaser,freq2),           2, 4, 100, 4000 },
    { "Q",            offsetof(dsp_type_phaser,Q),               2, 3, 50, 999 },
    { "Speed",        offsetof(dsp_type_phaser,frequency),       4, 4, 1, 4095 },
    { "Mixval",       offsetof(dsp_type_phaser,mixval),          4, 3, 0, 255 },
    { "SpeedCntrl",   offsetof(dsp_type_phaser,control_number1), 4, 1, 0, 6 },
    { "SourceUnit",   offsetof(dsp_type_phaser,source_unit),     4, 2, 1, MAX_DSP_UNITS },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_phaser dsp_type_phaser_default = { 0, 0, 200, 800, 200, 60, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  } ;

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
    "Ring",
    "Flange",
    "Phaser",
    "Backwards",
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
    dsp_type_configuration_entry_ring, 
    dsp_type_configuration_entry_flange, 
    dsp_type_configuration_entry_phaser, 
    dsp_type_configuration_entry_backwards, 
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
    dsp_type_process_ring,
    dsp_type_process_flange,
    dsp_type_process_phaser,
    dsp_type_process_backwards,
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
    (void *) &dsp_type_ring_default,
    (void *) &dsp_type_flange_default,
    (void *) &dsp_type_phaser_default,
    (void *) &dsp_type_backwards_default,
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
    if (prec == 1)
    {
        *((uint8_t *)v) = val;
        return;
    }
    if (prec == 2)
    {
        *((uint16_t *)v) = val;
        return;
    }
    *((uint32_t *)v) = val;
}

void dsp_unit_initialize(int dsp_unit_number, dsp_unit_type dut)
{
    dsp_unit *du = dsp_unit_entry(dsp_unit_number);
    
    if (dut >= DSP_TYPE_MAX_ENTRY) return;
    
    dsp_unit_struct_zero(du);
    memcpy((void *)du, dsp_unit_struct_defaults[dut], sizeof(dsp_unit));
    du->dtn.dut = dut;
    du->dtn.source_unit = dsp_unit_number + 1;
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
