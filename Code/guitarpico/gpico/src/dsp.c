#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "dsp.h"

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

bool initialize_dsp_type_sin_synth(void *initialization_data, dsp_unit *du)
{
	dsp_type_sine_synth *dtss = (void *)initialization_data;
	du->dtss.sine_counter_inc = dtss->sine_counter_inc;
	return true;
}

int32_t dsp_type_process_sin_synth(int32_t sample, dsp_unit *du)
{
	du->dtss.sine_counter += du->dtss.sine_counter_inc;
	int16_t sine_val = sine_table_entry(du->dtss.sine_counter / 256);
	return sine_val;
}

const dsp_type_configuration_entry dsp_type_configuration_entry_none[] = 
{
	{ NULL,        0, 4, 0, 0,   1    }
};

const dsp_type_configuration_entry dsp_type_configuration_entry_sin_synth[] = 
{
	{ "Frequency",  offsetof(dsp_type_sine_synth,sine_counter_inc), 4, 0, 100, 4000 },
	{ NULL,        0, 4, 0, 0,   1    }
};

const dsp_type_configuration_entry * const dtce[] = 
{
	dsp_type_configuration_entry_none,
	dsp_type_configuration_entry_sin_synth,	
	NULL
};

 dsp_type_initialize * const dti[] = { 
	initialize_dsp_type_none,
	initialize_dsp_type_sin_synth
};

 dsp_type_process * const dtp[] = {
	dsp_type_process_none,
	dsp_type_process_sin_synth
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