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
    DSP_TYPE_NOISEGATE,
    DSP_TYPE_DELAY,
    DSP_TYPE_ROOM,
    DSP_TYPE_COMBINE,
    DSP_TYPE_BANDPASS,
    DSP_TYPE_LOWPASS,
    DSP_TYPE_HIGHPASS,
    DSP_TYPE_ALLPASS,
    DSP_TYPE_TREMOLO,
    DSP_TYPE_VIBRATO,
    DSP_TYPE_WAH,
    DSP_TYPE_AUTOWAH,
    DSP_TYPE_ENVELOPE,
    DSP_TYPE_DISTORTION,
    DSP_TYPE_OVERDRIVE,
    DSP_TYPE_RING,
    DSP_TYPE_FLANGER,
    DSP_TYPE_CHORUS,
    DSP_TYPE_PHASER,
    DSP_TYPE_BACKWARDS,
    DSP_TYPE_PITCHSHIFT,
    DSP_TYPE_OCTAVE,
    DSP_TYPE_SINE_SYNTH,
    DSP_TYPE_MAX_ENTRY
} dsp_unit_type;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
} dsp_type_none;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
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
    uint32_t source_unit;
    uint32_t threshold;
    uint32_t response;
    uint32_t envelope;
    uint32_t control_number1;
    uint32_t pot_value1;
} dsp_type_noisegate;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
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
    uint32_t source_unit;
    uint32_t delay_samples[3];
    uint32_t amplitude[3];
} dsp_type_room;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t prev_amplitude;
    uint32_t unit[3];
    uint32_t amplitude[3];
    uint32_t signbit[3];
} dsp_type_combine;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_bandpass;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_lowpass;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filtb1;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_highpass;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t frequency;
    uint16_t Q;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_frequency;
    uint16_t last_Q;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_allpass;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t frequency;
    uint32_t last_frequency;
    uint32_t modulation;
    uint32_t last_modulation;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_tremolo;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t delay_samples;
    uint32_t frequency;
    uint32_t last_frequency;
    uint32_t modulation;
    uint32_t last_modulation;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_vibrato;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t freq1, freq2;
    uint16_t Q;
    uint32_t reverse;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_freq1, last_freq2;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filta1;
    int32_t filta2;
    int32_t filta1_interp1;
    int32_t filta1_interp2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_wah;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t freq1, freq2;
    uint16_t Q;
    uint32_t frequency;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint16_t last_freq1, last_freq2;
    uint16_t last_Q;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t last_frequency;
    int32_t filtb0;
    int32_t filta1;
    int32_t filta2;
    int32_t filta1_interp1;
    int32_t filta1_interp2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
} dsp_type_autowah;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint16_t freq1, freq2;
    uint16_t Q;
    uint32_t response;
    uint32_t sensitivity;
    uint32_t reverse;
    uint16_t last_freq1, last_freq2;
    uint16_t last_Q;
    int32_t filtb0;
    int32_t filta1;
    int32_t filta2;
    int32_t filta1_interp1;
    int32_t filta1_interp2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;
    uint32_t envelope;
} dsp_type_envelope;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    int32_t gain;
    uint32_t noise_gate;
    uint32_t sample_offset;
    uint32_t last_noise_gate;
    uint32_t last_sample_offset;
    int32_t low_threshold;
    int32_t high_threshold;
    int32_t offset_value;
    uint32_t control_number1;
    uint32_t pot_value1;
} dsp_type_distortion;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t threshold;
    uint32_t amplitude;
    uint32_t last_threshold, last_amplitude;
    int32_t  x0;
    int32_t  y0;
    int32_t  y0x0;
    int32_t  y01;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_overdrive;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t frequency;
    uint32_t sine_mix;
    uint32_t last_frequency;
    uint32_t control_number1;
    uint32_t pot_value1;
} dsp_type_ring;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t delay_samples;
    uint32_t frequency;
    uint32_t last_frequency;
    uint32_t modulation;
    uint32_t last_modulation;
    uint32_t feedback;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_flange;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t sine_counter;
    uint32_t sine_counter_inc;
    uint32_t delay_samples;
    uint32_t frequency;
    uint32_t last_frequency;
    uint32_t modulation;
    uint32_t last_modulation;
    uint32_t mixval;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_chorus;

#define PHASER_STAGES 8

typedef struct
{
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
    int32_t sampledly1[PHASER_STAGES], sampledly2[PHASER_STAGES], filtdly1[PHASER_STAGES], filtdly2[PHASER_STAGES];
} dsp_type_phaser;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t backwards_samples;
    uint32_t balance;
    uint32_t samples_count;
    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
} dsp_type_backwards;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    int32_t pitchshift_samples;
    uint32_t pitchshift_rate;
    uint32_t balance;
    uint32_t frequency;
    uint32_t Q;
    int32_t  samples_count;

    uint32_t control_number1;
    uint32_t pot_value1;
    uint32_t control_number2;
    uint32_t pot_value2;
    uint32_t control_number3;
    uint32_t pot_value3;

    int32_t pitchshift_samples_2;
    int32_t pitchshift_samples_12;
    int32_t pitchshift_samples_32;
    int32_t pitchshift_samples_4096;
    int32_t pitchshift_samples_8192;
    int32_t  pitchshift_samples_scale;
    
    uint32_t last_pitchshift_samples;
    int32_t  last_sample;
    uint32_t last_frequency;
    uint32_t last_Q;

    int32_t filtb0;
    int32_t filtb1;
    int32_t filta1;
    int32_t filta2;
    int32_t sampledly1, sampledly2, filtdly1, filtdly2;

} dsp_type_pitchshift;

typedef struct
{
    dsp_unit_type  dut;
    uint32_t source_unit;
    uint32_t rectify;
    uint32_t multiplier;
    int32_t  sample_avg;
    int32_t  sample_avg2;
} dsp_type_octave;

typedef union 
{
    dsp_type_none         dtn;
    dsp_type_sine_synth   dtss;
    dsp_type_noisegate    dtnoise;
    dsp_type_delay        dtd;
    dsp_type_room         dtroom;
    dsp_type_combine      dtcombine;
    dsp_type_bandpass     dtbp;
    dsp_type_lowpass      dtlp;
    dsp_type_highpass     dthp;
    dsp_type_allpass      dtap;
    dsp_type_tremolo      dttrem;
    dsp_type_vibrato      dtvibr;
    dsp_type_wah          dtwah;
    dsp_type_autowah      dtautowah;
    dsp_type_envelope     dtenv;
    dsp_type_distortion   dtdist;
    dsp_type_overdrive    dtovr;
    dsp_type_ring         dtring;
    dsp_type_flange       dtflng;
    dsp_type_chorus       dtchor;
    dsp_type_phaser       dtphaser;
    dsp_type_backwards    dtback;
    dsp_type_pitchshift   dtpitch;
    dsp_type_octave       dtoct;
} dsp_unit;

typedef bool    (dsp_type_initialize)(void *initialization_data, dsp_unit *du);
typedef int32_t (dsp_type_process)(int32_t sample, dsp_unit *du);

int32_t dsp_process_all_units(int32_t sample);
void dsp_unit_struct_zero(dsp_unit *du);
void dsp_unit_initialize(int dsp_unit_number, dsp_unit_type dut);

extern const void * const dsp_unit_struct_defaults[];
extern dsp_unit dsp_units[MAX_DSP_UNITS];

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

bool dsp_unit_set_value(uint dsp_unit_number, const char *desc, uint32_t value);
bool dsp_unit_get_value(uint dsp_unit_number, const char *desc, uint32_t *value);
dsp_unit_type dsp_unit_get_type(uint dsp_unit_number);
const dsp_type_configuration_entry *dsp_unit_get_configuration_entry(uint dsp_unit_number, uint num);

extern const dsp_type_configuration_entry * const dtce[];
extern const char * const dtnames[];

uint32_t dsp_read_value_prec(void *v, int prec);
void dsp_set_value_prec(void *v, int prec, uint32_t val);

void dsp_unit_reset(int dsp_unit_number);
void dsp_unit_reset_all(void);

#ifdef __cplusplus
}
#endif

#endif /* __DSP_H */