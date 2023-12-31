#ifndef __DSP_H
#define __DSP_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_DSP_UNITS 16

#define MATH_PI_F 3.1415926535f
#define SINE_TABLE_ENTRIES (1 << 8)

extern int32_t sine_table[];
void initialize_sine_table(void);
inline int32_t sine_table_entry(int n)
{
    return sine_table[n & (SINE_TABLE_ENTRIES-1)];
};

#define SAMPLE_CIRC_BUF_SIZE (1u<<15)
#define SAMPLE_CIRC_BUF_CLEAN_SIZE (1u<<15)

extern int16_t sample_circ_buf[];
extern int16_t sample_circ_buf_clean[];
extern int sample_circ_buf_offset;
extern int sample_circ_buf_clean_offset;

inline void insert_sample_circ_buf(int16_t insert_val)
{
    sample_circ_buf_offset = (sample_circ_buf_offset+1) & (SAMPLE_CIRC_BUF_SIZE-1);
    sample_circ_buf[sample_circ_buf_offset] = insert_val;
};

inline void insert_sample_circ_buf_clean(int16_t insert_val)
{
    sample_circ_buf_clean_offset = (sample_circ_buf_clean_offset+1) & (SAMPLE_CIRC_BUF_CLEAN_SIZE-1);
    sample_circ_buf_clean[sample_circ_buf_clean_offset] = insert_val;
};

inline int16_t sample_circ_buf_value(int offset)
{
    return sample_circ_buf[(sample_circ_buf_offset - offset) & (SAMPLE_CIRC_BUF_SIZE-1)];
};

inline int16_t sample_circ_buf_clean_value(int offset)
{
    return sample_circ_buf_clean[(sample_circ_buf_clean_offset - offset) & (SAMPLE_CIRC_BUF_CLEAN_SIZE-1)];
};

void initialize_sample_circ_buf(void);

void initialize_dsp(void);

typedef enum 
{
    DSP_TYPE_NONE = 0,
    DSP_TYPE_SINE_SYNTH,
    DSP_TYPE_DELAY,
    DSP_TYPE_BANDPASS
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
    uint32_t last_frequency;
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

typedef struct
{
    dsp_unit_type  dut;
    bool is_changed;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filtb2;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_bandpass;

typedef struct
{
    dsp_unit_type  dut;
    bool is_changed;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filtb2;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_lowpass;

typedef struct
{
    dsp_unit_type  dut;
    bool is_changed;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filtb2;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_highpass;

typedef struct
{
    dsp_unit_type  dut;
    bool is_changed;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filtb2;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_phaser;

typedef union 
{
    dsp_type_none         dtn;
    dsp_type_sine_synth   dtss;
    dsp_type_delay        dtd;
    dsp_type_bandpass     dtbp;
    dsp_type_lowpass      dtlp;
    dsp_type_highpass     dthp;
    dsp_type_highpass     dtphs;
} dsp_unit;

typedef bool    (dsp_type_initialize)(void *initialization_data, dsp_unit *du);
typedef int32_t (dsp_type_process)(int32_t sample, dsp_unit *du);

int32_t dsp_process_all_units(int32_t sample);
void dsp_unit_struct_zero(dsp_unit *du);
void dsp_unit_initialize(dsp_unit *du, dsp_unit_type dut);

extern const void * const dsp_unit_struct_defaults[];
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
    uint8_t    digits;
    uint32_t   minval;
    uint32_t   maxval;
} dsp_type_configuration_entry;

extern const dsp_type_configuration_entry * const dtce[];
extern const char * const dtnames[];

#ifdef __cplusplus
}
#endif

#endif /* __DSP_H */