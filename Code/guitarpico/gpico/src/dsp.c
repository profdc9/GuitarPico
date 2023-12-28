#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "dsp.h"
#include "guitarpico.h"

int16_t sine_table[SINE_TABLE_ENTRIES];
int16_t sample_circ_buf[SAMPLE_CIRC_BUF_SIZE];
int sample_circ_buf_offset;

dsp_unit dsp_units[MAX_DSP_UNITS];

void initialize_sine_table(void)
{
	for (int i=0;i<SINE_TABLE_ENTRIES;i++)
		sine_table[i] =  (int16_t)(16384.0f * sinf((2.0f*MATH_PI_F/((float)SINE_TABLE_ENTRIES))*((float)i)));
}

void initialize_sample_circ_buf(void)
{
	memset((void *)sample_circ_buf, '\000', sizeof(sample_circ_buf));
	sample_circ_buf_offset = 0;
}

void zero_dsp_unit_struct(dsp_unit *du)
{
	memset((void *)du,'\000',sizeof(dsp_unit));
}

bool initialize_dsp_type_none(void *initialization_data, dsp_unit *du)
{
	return true;
}

int32_t dsp_type_process_none(int32_t sample, dsp_unit *du)
{
	return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_none[] = 
{
	{ NULL,        0, 4, 0, 0,   1    }
};


bool initialize_dsp_type_sin_synth(void *initialization_data, dsp_unit *du)
{
	dsp_type_sine_synth *dtss = (void *)initialization_data;
	du->dtss.sine_counter_inc = dtss->sine_counter_inc;
	return true;
}

int32_t dsp_type_process_sin_synth(int32_t sample, dsp_unit *du)
{
	int32_t new_input = read_potentiometer_value(du->dtss.control_number);
	if (abs(new_input - du->dtss.pot_value) >= POTENTIOMETER_VALUE_SENSITIVITY)
	{
		du->dtss.pot_value = new_input;
		du->dtss.frequency = 40 + (du->dtss.pot_value >> 3);
		du->dtss.sine_counter_inc = 0;
	}
	if (du->dtss.sine_counter_inc == 0)
		du->dtss.sine_counter_inc = (du->dtss.frequency*65536) / GUITARPICO_SAMPLERATE;
	du->dtss.sine_counter += du->dtss.sine_counter_inc;
	int16_t sine_val = sine_table_entry((du->dtss.sine_counter & 0xFFFF) / 256);
	return sine_val;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_sin_synth[] = 
{
	{ "Freqn",  offsetof(dsp_type_sine_synth,sine_counter_inc), 4, 0, 100, 4000 },
	{ "Cntrl",  offsetof(dsp_type_sine_synth,control_number), 4, 0, 0, 6 },
	{ NULL,     0, 4, 0, 0,   1    }
};

bool initialize_dsp_type_delay(void *initialization_data, dsp_unit *du)
{
	dsp_type_delay *dtd = (void *)initialization_data;
	du->dtd.delay_samples = dtd->delay_samples;
	du->dtd.echo_reduction = dtd->echo_reduction;
	return true;
}

int32_t divide_log(uint32_t n, uint b)
{
	uint32_t sign_bit = (n & 0x80000000);
	while (b > 1)
	{
		n = (n >> 1) | sign_bit;
		b >>= 1;
	}
	return (int32_t) n;
}

int32_t dsp_type_process_delay(int32_t sample, dsp_unit *du)
{
	int32_t new_input = read_potentiometer_value(du->dtd.control_number1);
	if (abs(new_input - du->dtd.pot_value1) >= POTENTIOMETER_VALUE_SENSITIVITY)
	{
		du->dtd.pot_value1 = new_input;
		du->dtd.delay_samples = (du->dtd.pot_value1 * SAMPLE_CIRC_BUF_SIZE) / 16384;
	}
	new_input = read_potentiometer_value(du->dtd.control_number2);
	if (abs(new_input - du->dtd.pot_value2) >= POTENTIOMETER_VALUE_SENSITIVITY)
	{
		du->dtd.pot_value2 = new_input;
		du->dtd.echo_reduction = (255 * SAMPLE_CIRC_BUF_SIZE) / 16384;
	}
	sample += (sample_circ_buf_value(du->dtd.delay_samples) * ((int16_t)du->dtd.echo_reduction)) / 256;
	sample >>= 1;
	return sample;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_delay[] = 
{
	{ "Samples",    offsetof(dsp_type_delay,delay_samples),  4, 0, 1, SAMPLE_CIRC_BUF_SIZE },
	{ "EchoRed",    offsetof(dsp_type_delay,echo_reduction), 4, 0, 0, 255 },
	{ "Cntrl1",     offsetof(dsp_type_delay,control_number1), 4, 0, 0, 6 },
	{ "Cntrl2",     offsetof(dsp_type_delay,control_number2), 4, 0, 0, 6 },
	{ NULL,         0, 4, 0, 0,   1    }
};

const dsp_type_configuration_entry * const dtce[] = 
{
	dsp_type_configuration_entry_none,
	dsp_type_configuration_entry_sin_synth,	
	dsp_type_configuration_entry_delay,	
	NULL
};

 dsp_type_initialize * const dti[] = { 
	initialize_dsp_type_none,
	initialize_dsp_type_sin_synth,
	initialize_dsp_type_delay
};

 dsp_type_process * const dtp[] = {
	dsp_type_process_none,
	dsp_type_process_sin_synth,
	dsp_type_process_delay
};

bool dsp_initialize(dsp_unit_type dut, void *initialization_data, dsp_unit *du)
{
	zero_dsp_unit_struct(du);
	du->dtn.dut = dut;
	return dti[(int)dut](initialization_data, du);
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
		sample = dsp_process(sample, &dsp_units[i]);
	return sample;
}