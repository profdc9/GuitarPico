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
int32_t dsp_unit_result[MAX_DSP_UNITS];

void initialize_sine_table(void)
{
    for (int i=0;i<SINE_TABLE_ENTRIES;i++)
        sine_table[i] =  (int32_t)(QUANTIZATION_MAX_FLOAT * sinf((2.0f*MATH_PI_F/((float)SINE_TABLE_ENTRIES))*((float)i)));
}

void initialize_sample_circ_buf(void)
{
    memset((void *)sample_circ_buf, '\000', sizeof(sample_circ_buf));
    sample_circ_buf_offset = 0;
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
    { "Freqn", offsetof(dsp_type_sine_synth,sine_counter_inc), 4, 4, 100, 4000 },
    { "Cntrl", offsetof(dsp_type_sine_synth,control_number),   4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_sine_synth dsp_type_sine_synth_default = { 0, 0, 0, 3000, 0, 0, 0, 0 };

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
    sample += (sample_circ_buf_value(du->dtd.delay_samples) * ((int16_t)du->dtd.echo_reduction)) / 256;
    sample >>= 1;
    return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_delay[] = 
{
    { "Samples", offsetof(dsp_type_delay,delay_samples),  4, 5, 1, SAMPLE_CIRC_BUF_SIZE },
    { "EchoRed", offsetof(dsp_type_delay,echo_reduction), 4, 3, 0, 255 },
    { "Cntrl1", offsetof(dsp_type_delay,control_number1), 4, 1, 0, 6 },
    { "Cntrl2", offsetof(dsp_type_delay,control_number2), 4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_delay dsp_type_delay_default = { 0, 0, 1000, 128, 0, 0, 0, 0 };

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
        du->dtbp.filtb2 = -du->dtbp.filtb0;
        du->dtbp.filta1 = float_to_sampled_int(-2.0f*cosf(w0)*bfpa0);
        du->dtbp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dtbp.filtb0) * ((int32_t)sample)
               + ((int32_t)du->dtbp.filtb2) * ((int32_t)du->dtbp.sampledly2) 
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
    { "Frequency", offsetof(dsp_type_bandpass,frequency),       2, 4, 100, 4000 },
    { "Q",         offsetof(dsp_type_bandpass,Q),               2, 3, 100, 999 },
    { "Cntrl1",    offsetof(dsp_type_bandpass,control_number1), 4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_bandpass dsp_type_bandpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_LOWPASS*************************************/

int32_t dsp_type_process_lowpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtlp.control_number1);
    if (abs(new_input - du->dtlp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtlp.pot_value1 = new_input;
        du->dtbp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
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
        du->dtlp.filtb0 = du->dtlp.filtb2 = float_to_sampled_int(0.5*(1.0f-c0)*bfpa0);
        du->dtlp.filta1 = float_to_sampled_int(-2.0f*c0*bfpa0);
        du->dtlp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dtlp.filtb0) * ((int32_t)sample)
               + ((int32_t)du->dtlp.filtb1) * ((int32_t)du->dtlp.sampledly1)  
               + ((int32_t)du->dtlp.filtb2) * ((int32_t)du->dtlp.sampledly2) 
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
    { "Frequency", offsetof(dsp_type_lowpass,frequency),       2, 4, 100, 4000 },
    { "Q",         offsetof(dsp_type_lowpass,Q),               2, 3, 100, 999 },
    { "Cntrl1",    offsetof(dsp_type_lowpass,control_number1), 4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_lowpass dsp_type_lowpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_HIGHPASS*************************************/

int32_t dsp_type_process_highpass(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dthp.control_number1);
    if (abs(new_input - du->dthp.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dthp.pot_value1 = new_input;
        du->dtbp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
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
        du->dthp.filtb0 = du->dthp.filtb2 = float_to_sampled_int(0.5*(1.0f+c0)*bfpa0);
        du->dthp.filta1 = float_to_sampled_int(-2.0f*c0*bfpa0);
        du->dthp.filta2 = float_to_sampled_int((1.0f-a)*bfpa0);
    }
    filtout =    ((int32_t)du->dthp.filtb0) * ((int32_t)sample)
               + ((int32_t)du->dthp.filtb1) * ((int32_t)du->dthp.sampledly1)  
               + ((int32_t)du->dthp.filtb2) * ((int32_t)du->dthp.sampledly2) 
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
    { "Frequency", offsetof(dsp_type_highpass,frequency),       2, 4, 100, 4000 },
    { "Q",         offsetof(dsp_type_highpass,Q),               2, 3, 100, 999 },
    { "Cntrl1",    offsetof(dsp_type_highpass,control_number1), 4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_highpass dsp_type_highpass_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************************************DSP_TYPE_PHASER*************************************/

int32_t dsp_type_process_phaser(int32_t sample, dsp_unit *du)
{
    int32_t filtout;
    uint32_t new_input = read_potentiometer_value(du->dtphs.control_number1);
    if (abs(new_input - du->dtphs.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
    {
        du->dtphs.pot_value1 = new_input;
        du->dtbp.frequency = 100 + (new_input / (POT_MAX_VALUE / 2048));
    }
    if ((du->dtphs.frequency != du->dtphs.last_frequency) || (du->dtphs.Q != du->dtphs.last_Q))
    {
        du->dtphs.last_frequency = du->dtphs.frequency;
        du->dtphs.last_Q = du->dtphs.Q;
        float w0 = nyquist_fraction_omega(du->dtphs.frequency);
        float a = float_a_value(w0,du->dtphs.Q);
        float bfpa0 = 1.0f/(1.0f+a);
        du->dtphs.filtb1 = du->dtphs.filta1 = float_to_sampled_int(-2.0f*cosf(w0)*bfpa0);
        du->dtphs.filtb0 = du->dtphs.filta2 = float_to_sampled_int((1.0f-a) * bfpa0);
    }
    filtout =    ((int32_t)du->dtphs.filtb0) * ((int32_t)sample)
               + ((int32_t)du->dtphs.filtb1) * ((int32_t)du->dtphs.sampledly1)  
               + ((int32_t)du->dtphs.sampledly2) * float_to_sampled_int(1.0f)
               - ((int32_t)du->dtphs.filta1) * ((int32_t)du->dtphs.filtdly1)
               - ((int32_t)du->dtphs.filta2) * ((int32_t)du->dtphs.filtdly2);
    filtout = fractional_int_remove_offset(filtout);
    du->dtphs.sampledly2 = du->dtphs.sampledly1;
    du->dtphs.sampledly1 = sample;
    du->dtphs.filtdly2 = du->dtphs.filtdly1;
    du->dtphs.filtdly1 = filtout;
    return (filtout+sample)/2;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_phaser[] = 
{
    { "Frequency", offsetof(dsp_type_phaser,frequency),       2, 4, 100, 4000 },
    { "Q",         offsetof(dsp_type_phaser,Q),               2, 3, 100, 999 },
    { "Cntrl1",    offsetof(dsp_type_phaser,control_number1), 4, 1, 0, 6 },
    { NULL, 0, 4, 0, 0,   1    }
};

const dsp_type_phaser dsp_type_phaser_default = {0, 0, 400, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/************STRUCTURES FOR ALL DSP TYPES *****************************/

const char * const dtnames[] = 
{
    "None",
    "Sin Synth",
    "Delay",
    "Bandpass",
    "LowPass",
    "HighPass",
    "Phaser",
    NULL
};

const dsp_type_configuration_entry * const dtce[] = 
{
    dsp_type_configuration_entry_none,
    dsp_type_configuration_entry_sin_synth, 
    dsp_type_configuration_entry_delay, 
    dsp_type_configuration_entry_bandpass, 
    dsp_type_configuration_entry_lowpass, 
    dsp_type_configuration_entry_highpass, 
    dsp_type_configuration_entry_phaser, 
    NULL
};

dsp_type_process * const dtp[] = {
    dsp_type_process_none,
    dsp_type_process_sin_synth,
    dsp_type_process_delay,
    dsp_type_process_bandpass,
    dsp_type_process_lowpass,
    dsp_type_process_highpass,
    dsp_type_process_phaser,
};

const void * const dsp_unit_struct_defaults[] =
{
    (void *) &dsp_type_none_default,
    (void *) &dsp_type_sine_synth_default,
    (void *) &dsp_type_delay_default,
    (void *) &dsp_type_bandpass_default,
    (void *) &dsp_type_lowpass_default,
    (void *) &dsp_type_highpass_default,
    (void *) &dsp_type_phaser_default
};

/********************* DSP PROCESS STRUCTURE *******************************************/

void dsp_unit_initialize(dsp_unit *du, dsp_unit_type dut)
{
    dsp_unit_struct_zero(du);
    memcpy((void *)du, dsp_unit_struct_defaults[dut], sizeof(dsp_unit));
    du->dtn.dut = dut;
}

int32_t dsp_process(int32_t sample, dsp_unit *du)
{
    return dtp[(int)du->dtn.dut](sample, du);
}

void initialize_dsp(void)
{
    initialize_sine_table();
    initialize_sample_circ_buf();
    memset((void *)dsp_units,'\000',sizeof(dsp_units));
}

int32_t dsp_process_all_units(int32_t sample)
{
    for (int i=0;i<MAX_DSP_UNITS;i++)
        dsp_unit_result[i] = sample = dsp_process(sample, &dsp_units[i]);
    return sample;
}