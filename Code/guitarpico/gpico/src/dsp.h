#ifndef __DSP_H
#define __DSP_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_DSP_UNITS 16

#define MATH_PI_F 3.1415926535f
#define SINE_TABLE_ENTRIES (1 << 8)

inline int16_t float_to_sampled_int(float x)
{
	return (int16_t)(16384.0f*x+0.5f);
}

inline int16_t fractional_int_remove_offset(int32_t x)
{
	return (int16_t)(x/16384);
}

extern int16_t sine_table[];
void initialize_sine_table(void);
inline int16_t sine_table_entry(int n)
{
	return sine_table[n & (SINE_TABLE_ENTRIES-1)];
};

#define SAMPLE_CIRC_BUF_SIZE (1u<<15)
#define SAMPLE_CIRC_BUF_MASK (SAMPLE_CIRC_BUF_SIZE-1)

extern int16_t sample_circ_buf[];
extern int sample_circ_buf_offset;

inline void insert_sample_circ_buf(int16_t insert_val)
{
	sample_circ_buf_offset = (sample_circ_buf_offset+1) & (SAMPLE_CIRC_BUF_SIZE-1);
	sample_circ_buf[sample_circ_buf_offset] = insert_val;
};

inline int16_t sample_circ_buf_value(int offset)
{
	return sample_circ_buf[(sample_circ_buf_offset - offset) & (SAMPLE_CIRC_BUF_SIZE-1)];
};

void initialize_sample_circ_buf(void);

void initialize_dsp(void);

typedef enum 
{
	DSP_TYPE_NONE = 0,
	DSP_TYPE_SINE_SYNTH,
	DSP_TYPE_DELAY
} dsp_unit_type;

typedef struct
{
	dsp_unit_type  dut;
	bool is_changed;
} dsp_type_none;

typedef struct
{
	dsp_unit_type  dut;
	bool is_changed;
	uint32_t sine_counter;
	uint32_t sine_counter_inc;
	uint32_t control_number;
	uint32_t pot_value;
	uint32_t frequency;
} dsp_type_sine_synth;

typedef struct
{
	dsp_unit_type  dut;
	bool is_changed;
	uint32_t delay_samples;
	uint32_t echo_reduction;
	uint32_t control_number1;
	uint32_t pot_value1;
	uint32_t control_number2;
	uint32_t pot_value2;
} dsp_type_delay;

typedef union 
{
	dsp_type_none         dtn;
	dsp_type_sine_synth   dtss;
	dsp_type_delay        dtd;
} dsp_unit;

typedef bool    (dsp_type_initialize)(void *initialization_data, dsp_unit *du);
typedef int32_t (dsp_type_process)(int32_t sample, dsp_unit *du);

int32_t dsp_process_all_units(int32_t sample);
bool dsp_initialize(dsp_unit_type dut, void *initialization_data, dsp_unit *du);

extern dsp_unit dsp_units[];

inline dsp_unit *dsp_unit_entry(uint e)
{
	return &dsp_units[e];
}

typedef struct
{
	const char *desc;
	uint32_t   offset;
	uint8_t    size;
	bool       is_signed;
	int32_t    minval;
	int32_t    maxval;
} dsp_type_configuration_entry;

extern const dsp_type_configuration_entry * const dtce[];

#ifdef __cplusplus
}
#endif

#endif /* __DSP_H */