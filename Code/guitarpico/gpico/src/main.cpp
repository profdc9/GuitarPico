/* main.cpp

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

#include "main.h"
#include "guitarpico.h"
#include "ssd1306_i2c.h"
#include "buttons.h"
#include "dsp.h"
#include "pitch.h"
#include "ui.h"
#include "tinycl.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "usbmain.h"

void initialize_video(void);
void halt_video(void);

#define UNCLAIMED_ALARM 0xFFFFFFFF

uint dac_pwm_b3_slice_num, dac_pwm_b2_slice_num, dac_pwm_b1_slice_num, dac_pwm_b0_slice_num;
uint claimed_alarm_num = UNCLAIMED_ALARM;

const u8* Fonts[5] = { FontBold8x8, FontGame8x8, FontIbm8x8, FontItalic8x8, FontThin8x8 };

volatile uint32_t counter = 0;

extern "C" {
    
void idle_task(void)
{
    buttons_poll();
    usb_task();
}

}

void initialize_pwm(void)
{
    gpio_set_function(DAC_PWM_B3, GPIO_FUNC_PWM);
    gpio_set_function(DAC_PWM_B2, GPIO_FUNC_PWM);
    gpio_set_function(DAC_PWM_B1, GPIO_FUNC_PWM);
    gpio_set_function(DAC_PWM_B0, GPIO_FUNC_PWM);

    dac_pwm_b3_slice_num = pwm_gpio_to_slice_num(DAC_PWM_B3);
    dac_pwm_b2_slice_num = pwm_gpio_to_slice_num(DAC_PWM_B2);
    dac_pwm_b1_slice_num = pwm_gpio_to_slice_num(DAC_PWM_B1);
    dac_pwm_b0_slice_num = pwm_gpio_to_slice_num(DAC_PWM_B0);
    
    pwm_set_clkdiv_int_frac(dac_pwm_b3_slice_num, 1, 0);
    pwm_set_clkdiv_int_frac(dac_pwm_b2_slice_num, 1, 0);
    pwm_set_clkdiv_int_frac(dac_pwm_b1_slice_num, 1, 0);
    pwm_set_clkdiv_int_frac(dac_pwm_b0_slice_num, 1, 0);
    
    pwm_set_clkdiv_mode(dac_pwm_b3_slice_num, PWM_DIV_FREE_RUNNING);
    pwm_set_clkdiv_mode(dac_pwm_b2_slice_num, PWM_DIV_FREE_RUNNING);
    pwm_set_clkdiv_mode(dac_pwm_b1_slice_num, PWM_DIV_FREE_RUNNING);
    pwm_set_clkdiv_mode(dac_pwm_b0_slice_num, PWM_DIV_FREE_RUNNING);

    pwm_set_phase_correct(dac_pwm_b3_slice_num, false);
    pwm_set_phase_correct(dac_pwm_b2_slice_num, false);
    pwm_set_phase_correct(dac_pwm_b1_slice_num, false);
    pwm_set_phase_correct(dac_pwm_b0_slice_num, false);
    
    pwm_set_wrap(dac_pwm_b3_slice_num, DAC_PWM_WRAP_VALUE-1);
    pwm_set_wrap(dac_pwm_b2_slice_num, DAC_PWM_WRAP_VALUE-1);
    pwm_set_wrap(dac_pwm_b1_slice_num, DAC_PWM_WRAP_VALUE-1);
    pwm_set_wrap(dac_pwm_b0_slice_num, DAC_PWM_WRAP_VALUE-1);
    
    pwm_set_output_polarity(dac_pwm_b3_slice_num, false, false);
    pwm_set_output_polarity(dac_pwm_b2_slice_num, false, false);
    pwm_set_output_polarity(dac_pwm_b1_slice_num, false, false);
    pwm_set_output_polarity(dac_pwm_b0_slice_num, false, false);
    
    pwm_set_enabled(dac_pwm_b3_slice_num, true);
    pwm_set_enabled(dac_pwm_b2_slice_num, true);
    pwm_set_enabled(dac_pwm_b1_slice_num, true);
    pwm_set_enabled(dac_pwm_b0_slice_num, true);
}

uint current_input;
uint control_sample_no;
uint16_t control_samples[8];

const int potvalues[6] = { 2, 1, 0, 3, 4, 6 };

uint16_t read_potentiometer_value(uint v)
{
    if ((v == 0) || (v > (sizeof(potvalues)/sizeof(potvalues[0]))))
        return 0;
    return (uint16_t)(control_samples[potvalues[v-1]]*(POT_MAX_VALUE/ADC_MAX_VALUE));
}

volatile uint32_t last1=0,last2=0,last3=0;
volatile uint32_t dly1,dly2,dly3;

volatile uint16_t next_sample = 0;

absolute_time_t last_time;

static inline absolute_time_t update_next_timeout(const absolute_time_t last_time, uint32_t us, uint32_t min_us)
{
    absolute_time_t next_time = delayed_by_us(last_time, us);
    absolute_time_t next_time_sooner = make_timeout_time_us(min_us);
    return (absolute_time_diff_us(next_time, next_time_sooner) < 0) ? next_time : next_time_sooner;
}

int32_t sample_avg;
uint32_t mag_avg;

static void __no_inline_not_in_flash_func(alarm_func)(uint alarm_num)
{
    uint16_t sample;
    uint32_t cur_time;
    absolute_time_t next_alarm_time;

    sample = adc_hw->result;
    cur_time = timer_hw->timelr;
    current_input = (current_input+1) & 0x01;
    
    adc_select_input(current_input);
    if (current_input == 0)
    {
        pwm_set_both_levels(dac_pwm_b3_slice_num, next_sample, next_sample);
        pwm_set_both_levels(dac_pwm_b1_slice_num, next_sample, next_sample);
        dly3 = cur_time-last3;
        control_samples[control_sample_no] = sample;
        control_sample_no = (control_sample_no >= 7) ? 0 : (control_sample_no+1);
        gpio_put(GPIO_ADC_SEL0, (control_sample_no & 0x01) == 0);
        gpio_put(GPIO_ADC_SEL1, (control_sample_no & 0x02) == 0);
        gpio_put(GPIO_ADC_SEL2, (control_sample_no & 0x04) == 0);
        last_time = delayed_by_us(last_time, 10);
        do
        {
            next_alarm_time = update_next_timeout(last_time, 0, 8);
        } while (hardware_alarm_set_target(claimed_alarm_num, next_alarm_time));
        last2 = cur_time;
        return;
    } 

    dly1 = cur_time-last1;
    last1 = cur_time;
    last3 = cur_time;
    dly2 = cur_time-last2;

    int16_t s = (sample - (ADC_MAX_VALUE/2))*(ADC_PREC_VALUE/ADC_MAX_VALUE);
    sample_avg = (sample_avg*511)/512 + s;
    s -= (sample_avg / 512);
    if (s < (-ADC_PREC_VALUE/2)) s = (-ADC_PREC_VALUE/2);
    if (s > (ADC_PREC_VALUE/2-1)) s = (ADC_PREC_VALUE/2-1);
    mag_avg = (mag_avg*511)/512 + ((uint32_t)abs(s));
    if (mag_avg < (512*ADC_PREC_VALUE/512))
        pitch_current_entry = 0;
    insert_pitch_edge(s, counter);
    insert_sample_circ_buf_clean(s);
    s = dsp_process_all_units(s);
    insert_sample_circ_buf(s);
    if (s > (ADC_PREC_VALUE/2-1)) next_sample = DAC_PWM_WRAP_VALUE-1;
    else if (s < (-ADC_PREC_VALUE/2)) next_sample = 0;
    else next_sample = (s+(ADC_PREC_VALUE/2)) / (ADC_PREC_VALUE/DAC_PWM_WRAP_VALUE);
    last_time = delayed_by_us(last_time, 30);
    do
    {
        next_alarm_time = update_next_timeout(last_time, 0, 8);
    } while (hardware_alarm_set_target(claimed_alarm_num, next_alarm_time));
    counter++;
}

void reset_periodic_alarm(void)
{
    if (claimed_alarm_num == UNCLAIMED_ALARM) return;
    
    hardware_alarm_cancel(claimed_alarm_num);
    control_sample_no = 0;
    current_input = 0;
    hardware_alarm_set_callback(claimed_alarm_num, alarm_func);
    last_time = make_timeout_time_us(1000);
    absolute_time_t next_alarm_time = update_next_timeout(last_time, 0, 8);
    //absolute_time_t next_alarm_time = make_timeout_time_us(1000);
    hardware_alarm_set_target(claimed_alarm_num, next_alarm_time);
}


void initialize_periodic_alarm(void)
{
    if (claimed_alarm_num != UNCLAIMED_ALARM)
    {
        reset_periodic_alarm();
        return;
    }
    claimed_alarm_num = hardware_alarm_claim_unused(true);
    reset_periodic_alarm();
    /*for (uint alarm_no=0;alarm_no<4;alarm_no++)
    {
        if (!hardware_alarm_is_claimed(alarm_no))
        {
            hardware_alarm_claim_unused(alarm_no);
            claimed_alarm_num = alarm_no;
            reset_periodic_alarm();
            break;
        }
    } */
}

void initialize_adc(void)
{
    
    adc_init();
    adc_gpio_init(ADC_AUDIO_IN);
    adc_gpio_init(ADC_CONTROL_IN);
    adc_set_clkdiv(100);
    adc_set_round_robin(0);
    adc_run(false);
    adc_select_input(0);
    current_input = 0;
    adc_run(true);
}

void initialize_gpio(void)
{
    // initialize debug LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(GPIO_ADC_SEL0);
    gpio_set_dir(GPIO_ADC_SEL0, GPIO_OUT);
    gpio_init(GPIO_ADC_SEL1);
    gpio_set_dir(GPIO_ADC_SEL1, GPIO_OUT);
    gpio_init(GPIO_ADC_SEL2);
    gpio_set_dir(GPIO_ADC_SEL2, GPIO_OUT);

    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 1);
}

char buttonpressed(uint8_t b)
{
    return button_readbutton(b) ? '1' : '0';
}

void adjust_parms(uint8_t unit_no)
{
    int sel = 0, redraw = 1;
    button_clear();
    for (;;)
    {
        const dsp_type_configuration_entry *d = dtce[dsp_units[unit_no].dtn.dut];
        idle_task();
        if (redraw)
        {
            if (sel == 0)
            {
                write_str_with_spaces(0,2,"Type",16);
                write_str_with_spaces(0,3,dtnames[dsp_units[unit_no].dtn.dut],16);
            } else
            {
                char s[20];
                write_str_with_spaces(0,2,d[sel-1].desc,16);
                char *c = number_str(s, 
                    dsp_read_value_prec((void *)(((uint8_t *)&dsp_units[unit_no]) + d[sel-1].offset), d[sel-1].size), 
                    d[sel-1].digits, 0);
                write_str_with_spaces(0,3,c,16);
            }
            display_refresh();
            redraw = 0;            
        }
        if (button_left())
        {
            button_clear();
            break;
        }
        else if (button_down() && (sel > 0))
        {
            sel--;
            redraw = 1;
        } else if (button_up())
        {
            if (d[sel].desc != NULL)
            {
                sel++;
                redraw = 1;
            }
        } else if (button_right() || button_enter())
        {
            if (sel == 0)
            {
                write_str_with_spaces(0,2,"Type select",16);
                menu_str mst = { dtnames,0,3,10,0,0 };
                mst.item = mst.itemesc = dsp_units[unit_no].dtn.dut;
                int res;
                do_show_menu_item(&mst);
                do
                {
                    idle_task();
                    res = do_menu(&mst);
                } while (res == 0);
                if (res == 3)
                {
                    if (mst.item != dsp_units[unit_no].dtn.dut)
                    {
                        dsp_unit_initialize(unit_no, (dsp_unit_type)mst.item);
                        dsp_unit_reset_all();
                    }
                }
            } else
            {
                scroll_number_dat snd = { 0, 3, 
                                          d[sel-1].digits,
                                          0,
                                          d[sel-1].minval,
                                          d[sel-1].maxval,
                                          0,
                                          dsp_read_value_prec((void *)(((uint8_t *)&dsp_units[unit_no]) + d[sel-1].offset), d[sel-1].size),
                                          0, 0 };
                scroll_number_start(&snd);
                do
                {
                    idle_task();
                    scroll_number_key(&snd);
                } while (!snd.entered);
                if (snd.changed)
                   dsp_set_value_prec((void *)(((uint8_t *)&dsp_units[unit_no]) + d[sel-1].offset), d[sel-1].size, snd.n);
            }
            redraw = 1;
        }
    }
    
    
}

void adjust_dsp_params(void)
{
    int unit_no = 0, redraw = 1;
    char s[20];

    button_clear();
    for (;;)
    {
        idle_task();
        if (redraw)
        {
            clear_display();
            write_str(0,0,"DSP Adj");
            sprintf(s,"Unit #%d", unit_no+1);
            write_str_with_spaces(0,1,s,16);
            write_str_with_spaces(0,2,dtnames[dsp_units[unit_no].dtn.dut],16);
            display_refresh();
            redraw = 0;
        }
        if (button_left())
        {
            button_clear();
            break;
        }
        else if (button_down() && (unit_no > 0))
        {
            unit_no--;
            redraw = 1;
        } else if (button_up() && (unit_no < (MAX_DSP_UNITS-1)))
        {
            unit_no++;
            redraw = 1;
        } else if (button_right() || button_enter())
        {
           sprintf(s,"Unit #%d select", unit_no+1);
           write_str_with_spaces(0,1,s,16);
           adjust_parms(unit_no);
           redraw = 1;
        }
    }
}

int main2();

#define FLASH_BANKS 10
#define FLASH_PAGE_BYTES 4096u
#define FLASH_OFFSET_STORED (2*1024*1024)
#define FLASH_BASE_ADR 0x10000000
#define FLASH_MAGIC_NUMBER 0xFEEDEEFD

#define FLASH_PAGES(x) ((((x)+(FLASH_PAGE_BYTES-1))/FLASH_PAGE_BYTES)*FLASH_PAGE_BYTES)

uint32_t last_gen_no;
uint8_t  desc[16];

typedef struct _flash_layout_data
{
    uint32_t magic_number;
    uint32_t gen_no;
    uint8_t  desc[16];
    dsp_unit dsp_units[MAX_DSP_UNITS];
} flash_layout_data;

typedef union _flash_layout
{ 
    flash_layout_data fld;
    uint8_t      space[FLASH_PAGE_BYTES];
} flash_layout;

inline static uint32_t flash_offset_bank(uint bankno)
{
    return (FLASH_OFFSET_STORED - FLASH_PAGES(sizeof(flash_layout)) * (bankno+1));
}

inline static const uint8_t *flash_offset_address_bank(uint bankno)
{
    const uint8_t *const flashadr = (const uint8_t *const) FLASH_BASE_ADR;
    return &flashadr[flash_offset_bank(bankno)];
}

void message_to_display(const char *msg)
{
    write_str_with_spaces(0,5,msg,16);
    display_refresh();

    buttons_clear();
    for (;;)
    {
       idle_task();
       if (button_enter()) break;
     }
}

int write_data_to_flash(uint32_t flash_offset, const uint8_t *data, uint core, uint32_t length)
{
    length = FLASH_PAGES(length);                // pad up to next 4096 byte boundary
    flash_offset = FLASH_PAGES(flash_offset);
    if (!multicore_lockout_victim_is_initialized(core))
        return -1;
    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offset, length);
    flash_range_program(flash_offset, data, length);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();
    reset_periodic_alarm();
    return 0;
}

uint select_bankno(void)
{
    uint bankno = 1;
    bool notend = true;
    
    while (notend)
    {
        char s[20];
        sprintf(s,"Sel Bank: %02d", bankno);          
        write_str_with_spaces(0,4,s,16);
        flash_layout *fl = (flash_layout *) flash_offset_address_bank(bankno-1);
        write_str_with_spaces(0,5,fl->fld.magic_number == FLASH_MAGIC_NUMBER ? (char *)fl->fld.desc : "No Description",15);
        display_refresh();
        buttons_clear();
        for (;;)
        {
            idle_task();
            if (button_enter())
            {
                notend = false;
                break;
            }
            if (button_left()) return 0;
            if ((button_up()) && (bankno < (FLASH_BANKS))) 
            {
                bankno++;
                break;
            }
            if ((button_down()) && (bankno > 1)) 
            {
                bankno--;
                break;
            }
        }
    }
    return bankno;
}

int flash_load_bank(uint bankno)
{
    flash_layout *fl = (flash_layout *) flash_offset_address_bank(bankno);

    if (bankno >= FLASH_BANKS) return -1;
    if (fl->fld.magic_number == FLASH_MAGIC_NUMBER)
    {
        if (fl->fld.gen_no > last_gen_no)
            last_gen_no = fl->fld.gen_no;
        memcpy((void *)dsp_units, (void *) &fl->fld.dsp_units, sizeof(dsp_units));
        memcpy(desc, fl->fld.desc, sizeof(desc));
    } else return -1;
    return 0;
}

int flash_load(void)
{
    uint bankno;
    if ((bankno = select_bankno()) == 0) return -1;
    message_to_display(flash_load_bank(bankno-1) ? "Not Loaded" : "Loaded");
    return 0;
}

void flash_load_most_recent(void)
{
    uint32_t newest_gen_no = 0;
    uint load_bankno = FLASH_BANKS;

    last_gen_no = 0;
    memset(desc,'\000',sizeof(desc));

    for (uint bankno = 0;bankno < FLASH_BANKS; bankno++)
    {
        flash_layout *fl = (flash_layout *) flash_offset_address_bank(bankno);
        if (fl->fld.magic_number == FLASH_MAGIC_NUMBER)
        {
            if (fl->fld.gen_no >= newest_gen_no)
            {
                newest_gen_no = fl->fld.gen_no;
                load_bankno = bankno;
            }
        }
    }
    if (load_bankno < FLASH_BANKS) flash_load_bank(load_bankno);
}

 const uint8_t validchars[] = { ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 
                                'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7' ,'8', '9', '-', '/', '.', '!', '?' };

int flash_save_bank(uint bankno)
{
    flash_layout *fl;

    if (bankno >= FLASH_BANKS) return -1;
    if ((fl = (flash_layout *)malloc(sizeof(flash_layout))) == NULL) return -1;
    memset((void *)fl,'\000',sizeof(flash_layout));
    fl->fld.magic_number = FLASH_MAGIC_NUMBER;
    fl->fld.gen_no = (++last_gen_no);
    memcpy(fl->fld.desc, desc, sizeof(fl->fld.desc));
    memcpy((void *)&fl->fld.dsp_units, (void *)dsp_units, sizeof(fl->fld.dsp_units));
    int ret = write_data_to_flash(flash_offset_bank(bankno), (uint8_t *) fl, 1, sizeof(flash_layout));
    free(fl);
    return ret;
}

void flash_save(void)
{
    uint bankno;
    scroll_alpha_dat sad = { 0, 5, sizeof(desc)-1, sizeof(desc)-1, desc, validchars, sizeof(validchars), 0, 0, 0, 0, 0 };

    if ((bankno = select_bankno()) == 0) return;
    write_str_with_spaces(0,4,"Description:",15);
    scroll_alpha_start(&sad);
    for (;;)
    {
        idle_task();
        scroll_alpha_key(&sad);
        if (sad.exited) return;
        if (sad.entered) break;
    }    
    write_str_with_spaces(0,4,"",15);
    message_to_display(flash_save_bank(bankno-1) ? "Save Failed" : "Save Succeeded");      
}

void pitch_measure(void)
{

    char str[80];
    bool endloop = false;
    uint32_t last_time = 0;

    uint32_t hz = 0;
    int32_t note_no = -1;
    uint32_t mag_note_thr = 0;
    bool playing_note = false;
    bool another_note = true;
    
    clear_display();
    pitch_buffer_reset();
    buttons_clear();
    int32_t min_offset = PITCH_MIN_OFFSET;
    
    while (!endloop)
    {
        idle_task();
        uint32_t cur_mag_avg = mag_avg;
        if (pitch_current_entry >= NUM_PITCH_EDGES)
        {
            pitch_edge_autocorrelation();
            pitch_buffer_reset();
            int32_t entry = pitch_autocorrelation_max(min_offset);
            if (entry >= 0)
            {
                uint32_t hzn = pitch_estimate_peak_hz(entry);
                if (hzn > 0)
                {
                    min_offset = pitch_autocor[entry].offset / 2;
                    int32_t note_no_n = pitch_find_note(hzn);
                    if ((note_no_n >= 0) && (another_note))
                    {
                        mag_note_thr = cur_mag_avg*3/4;
                        note_no = note_no_n;
                        hz = hzn;
                        uint32_t velocity = cur_mag_avg / (4* ADC_PREC_VALUE / 128);
                        if (velocity > 127) velocity = 127;
                        gpio_put(LED_PIN, 1);
                        midi_send_note(note_no+24, velocity);
                        usb_task();
                        playing_note = true;
                        another_note = false;
                    }
                }
            }
        }
        if (cur_mag_avg < mag_note_thr) another_note = true;
        if (mag_avg < (512*40))
        {
            min_offset = PITCH_MIN_OFFSET;
            gpio_put(LED_PIN, 0);
            //pitch_buffer_reset();
            if (playing_note)
            {
                midi_send_note(0,0);
                usb_task();
                playing_note = false;
                another_note = true;
                mag_note_thr = 0;
            }
        } 
        if (button_enter() || button_left()) 
        {
            endloop = true;
            break;
        }
        if ((time_us_32() - last_time) > 250000)
        {
            last_time = time_us_32();
            sprintf(str,"Hz: %u",hz);
            write_str_with_spaces(0,0,str,16);
            sprintf(str,"Nt: %s %u", note_no < 0 ? "None" : notes[note_no].note, note_no < 0 ? 0 : notes[note_no].frequency_hz);
            write_str_with_spaces(0,1,str,16);
            sprintf(str,"Mag: %u %u",mag_avg,mag_note_thr);
            write_str_with_spaces(0,3,str,16);
            display_refresh();
        }
    }
    midi_send_note(0,0);
    gpio_put(LED_PIN, 0);
}


void debugstuff(void)
{
    char str[40];
    bool endloop = false;
    
    while (!endloop)
    {
        uint32_t last_time = time_us_32();
        while ((time_us_32() - last_time) < 100000)
        {
            idle_task();
            if (button_enter() || button_left()) 
            {
                endloop = true;
                break;
            }
        }
        ssd1306_Clear_Buffer();
        sprintf(str,"%u %c%c%c%c%c",counter,buttonpressed(0),buttonpressed(1),buttonpressed(2),buttonpressed(3),buttonpressed(4));
        ssd1306_set_cursor(0,0);
        ssd1306_printstring(str);
        sprintf(str,"dlys: %u %u %u",dly1,dly2,dly3);
        ssd1306_set_cursor(0,1);
        ssd1306_printstring(str);
        sprintf(str,"c1: %u %u %u",control_samples[0],control_samples[1],control_samples[2]);
        ssd1306_set_cursor(0,2);
        ssd1306_printstring(str);
        sprintf(str,"c2: %u %u %u",control_samples[3],control_samples[4],control_samples[5]);
        ssd1306_set_cursor(0,3);
        ssd1306_printstring(str);
        sprintf(str,"buf: %d",sample_circ_buf_value(0));
        ssd1306_set_cursor(0,4);
        ssd1306_printstring(str);
         sprintf(str,"rate %u",(uint32_t)((((uint64_t)counter)*1000000)/time_us_32()));
        ssd1306_set_cursor(0,5);
        ssd1306_printstring(str);
        ssd1306_render();
    }
}

const char * const mainmenu[] = { "Adjust", "Pitch", "Debug", "Load", "Save", NULL };

menu_str mainmenu_str = { mainmenu, 0, 2, 15, 0, 0 };

int test_cmd(int args, tinycl_parameter* tp, void *v)
{
  char s[20];
  tinycl_put_string("TEST=");
  sprintf(s,"%d\r\n",counter,counter);
  tinycl_put_string(s);
  return 1;
}

void type_cmd_write(uint unit_no, dsp_unit_type dut)
{
  char s[40];
  if (dut >= DSP_TYPE_MAX_ENTRY) return;
  sprintf(s,"TYPE %u %u ",unit_no+1, (uint)dut);
  tinycl_put_string(s);
  tinycl_put_string(dtnames[(uint)dut]);
  tinycl_put_string("\r\n");    
}

int type_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint unit_no=tp[0].ti.i;
  dsp_unit_type dut = DSP_TYPE_NONE;
  
  if (unit_no == 0)
  {
      do
      {
          dut = dsp_unit_get_type(unit_no);
          type_cmd_write(unit_no, dut);
          unit_no++;
      } while (dut < DSP_TYPE_MAX_ENTRY);
      return 1;
  }
  unit_no--;
  dut = dsp_unit_get_type(unit_no);
  if (dut < DSP_TYPE_MAX_ENTRY)
      type_cmd_write(unit_no, dut);
  return 1;
}

int init_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint unit_no=tp[0].ti.i;
  uint type_no=tp[1].ti.i;  
  
  if (unit_no > 0)
  {
    dsp_unit_initialize(unit_no-1, (dsp_unit_type) type_no);
    dsp_unit_reset_all();
  }
  return 1;
}

void conf_entry_print(uint unit_no, const dsp_type_configuration_entry *dtce)
{
    uint32_t value;
    char s[60];
    sprintf(s,"SET %u ",unit_no+1);
    tinycl_put_string(s);    
    tinycl_put_string(dtce->desc);
    if (!dsp_unit_get_value(unit_no, dtce->desc, &value)) value = 0;
    sprintf(s," %u %u %u\r\n",value,dtce->minval,dtce->maxval);
    tinycl_put_string(s);
}

 const dsp_type_configuration_entry *conf_entry_lookup_print(uint unit_no, uint entry_no)
 {
    const dsp_type_configuration_entry *dtce = dsp_unit_get_configuration_entry(unit_no, entry_no);
    if (dtce == NULL) return NULL;
    conf_entry_print(unit_no, dtce);
    return dtce;
 }

int conf_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint unit_no=tp[0].ti.i;
  uint entry_no=tp[1].ti.i;
 
  if (unit_no > 0)
  {
    unit_no--;
    if (entry_no == 0)
    {
        dsp_unit_type dut = dsp_unit_get_type(unit_no);
        type_cmd_write(unit_no, dut);    
        while (conf_entry_lookup_print(unit_no, entry_no) != NULL) entry_no++;
    } else
        conf_entry_lookup_print(unit_no, entry_no-1);
  } else
  {
    for (;;)
    {
       dsp_unit_type dut = dsp_unit_get_type(unit_no);
       if (dut >= DSP_TYPE_MAX_ENTRY) break;
       type_cmd_write(unit_no, dut);    
       entry_no = 0;
       while (conf_entry_lookup_print(unit_no, entry_no) != NULL) entry_no++;
       unit_no++;
    }
  }
  tinycl_put_string("END 0 END\r\n");
  return 1;
}

int set_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint unit_no=tp[0].ti.i;
  const char *parm = tp[1].ts.str;
  uint value=tp[2].ti.i;

  tinycl_put_string((unit_no > 0) && dsp_unit_set_value(unit_no-1, parm, value) ? "Set\r\n" : "Error\r\n");
  return 1;
}

int get_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint unit_no=tp[0].ti.i;
  const char *parm = tp[1].ts.str;
  uint32_t value;
    
  if ((unit_no > 0) && dsp_unit_get_value(unit_no-1, parm, &value))
  {
      char s[40];
      sprintf(s,"%u\r\n",value);
      tinycl_put_string(s);
  } else
      tinycl_put_string("Error\r\n");
  return 1;
}

int save_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint bankno=tp[0].ti.i;
 
  tinycl_put_string((bankno == 0) || flash_save_bank(bankno-1) ? "Not Saved\r\n" : "Saved\r\n");
  return 1;
}

int load_cmd(int args, tinycl_parameter* tp, void *v)
{
  uint bankno=tp[0].ti.i;
 
  tinycl_put_string((bankno == 0) || flash_load_bank(bankno-1) ? "Not Loaded\r\n" : "Loaded\r\n");
  return 1;
}

void copyspaces(char *c, const char *d, int n)
{
    while ((n>0) && (*d != '\000'))
    {
        for (uint i=0;i<(sizeof(validchars)/sizeof(validchars[0]));i++)
            if (((uint8_t) *d) == validchars[i])
            {
                *c++ = *d;
                n--;
                break;
            }
        d++; 
    }
    while (n>0)
    {
        *c++ = ' ';
        n--;
    }
    *c = '\000';
}

int setdesc_cmd(int args, tinycl_parameter* tp, void *v)
{
  const char *parm = tp[0].ts.str;
  
  copyspaces((char *)desc,parm,sizeof(desc)-1);
 
  tinycl_put_string("Description: '");
  tinycl_put_string((const char *)desc);
  tinycl_put_string("'\r\n");
  return 1;
}

int getdesc_cmd(int args, tinycl_parameter* tp, void *v)
{
  tinycl_put_string("Description: '");
  tinycl_put_string((const char *)desc);
  tinycl_put_string("'\r\n");
  return 1;
}

int a_cmd(int args, tinycl_parameter* tp, void *v)
{
    char s[256];
    absolute_time_t at = get_absolute_time();
    uint32_t l = to_us_since_boot(at);
    pitch_edge_autocorrelation();
    at = get_absolute_time();
    uint32_t m = to_us_since_boot(at);
    for (uint i=0;i<pitch_current_entry;i++)
    {
        sprintf(s,"%u edge: neg: %u sample: %d  counter: %d\r\n", i, pitch_edges[i].negative, pitch_edges[i].sample, pitch_edges[i].counter-pitch_edges[0].counter);
        tinycl_put_string(s);
    }
    for (uint i=pitch_autocor_size;i>0;)
    {
        i--;
        sprintf(s,"%u offset: %u autocor: %d", i, pitch_autocor[i].offset, pitch_autocor[i].autocor);
        tinycl_put_string(s);
        sprintf(s,"     %d %d %d\r\n",pitch_autocorrelation_sample(pitch_autocor[i].offset-1),
                                 pitch_autocorrelation_sample(pitch_autocor[i].offset),
                                 pitch_autocorrelation_sample(pitch_autocor[i].offset+1));
        tinycl_put_string(s);
    }
    sprintf(s,"total us: %u\r\n", (uint32_t)(m-l));
    tinycl_put_string(s);
    pitch_buffer_reset();
    return 1;
}

int help_cmd(int args, tinycl_parameter *tp, void *v);

const tinycl_command tcmds[] =
{
  { "GETDESC", "Get description", getdesc_cmd, TINYCL_PARM_END },
  { "SETDESC", "Set description", setdesc_cmd, TINYCL_PARM_STR, TINYCL_PARM_END },
  { "LOAD", "Load configuration", load_cmd, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "SAVE", "Save configuration", save_cmd, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "GET",  "Get configuration entry", get_cmd, TINYCL_PARM_INT, TINYCL_PARM_STR, TINYCL_PARM_END },
  { "SET",  "Set configuration entry", set_cmd, TINYCL_PARM_INT, TINYCL_PARM_STR, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "CONF", "Get configuration list", conf_cmd, TINYCL_PARM_INT, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "INIT", "Set type of effect", init_cmd, TINYCL_PARM_INT, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "TYPE", "Get type of effect", type_cmd, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "A", "Test autocorrelation", a_cmd, TINYCL_PARM_END },
  { "TEST", "Test", test_cmd, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "HELP", "Display This Help", help_cmd, {TINYCL_PARM_END } }
};

int help_cmd(int args, tinycl_parameter *tp, void *v)
{
  tinycl_print_commands(sizeof(tcmds) / sizeof(tinycl_command), tcmds);
  return 1;
}

int main()
{
    usb_init();    
    //stdio_init_all();
    initialize_dsp();
    initialize_pitch();
    initialize_gpio();
    buttons_initialize();
    initialize_pwm();
    ssd1306_Initialize();
    initialize_video();
    initialize_adc();
    initialize_periodic_alarm();
    flash_load_most_recent();
    
    for (;;)
    {
        clear_display();
        write_str(0,0,"GuitarPico");
        write_str_with_spaces(0,1,(char *)desc,15);
        do_show_menu_item(&mainmenu_str);
        buttons_clear();
        for (;;)
        {
            if (tinycl_task(sizeof(tcmds) / sizeof(tinycl_command), tcmds, NULL))
            {
                tinycl_do_echo = 1;
                tinycl_put_string("> ");
            }
            idle_task();
            uint val = do_menu(&mainmenu_str);
            if ((val == 2) || (val == 3)) break;
        }
        switch (mainmenu_str.item)
        {
            case 0:  adjust_dsp_params();
                     break;
            case 1:  pitch_measure();
                     break;
            case 2:  debugstuff();
                     break;
            case 3:  flash_load();
                     break;
            case 4:  flash_save();
                     break;
        }
    }
}


// Draw box
#if FORMAT==CANVAS_PLANE2
u8 Box[2*WIDTHBYTE*HEIGHT] __attribute__ ((aligned(4)));
#elif FORMAT==CANVAS_ATTRIB8
u8 Box[WIDTHBYTE*HEIGHT + WIDTHBYTE*((HEIGHT+7)/8)] __attribute__ ((aligned(4)));
#else
u8 Box[WIDTHBYTE*HEIGHT] __attribute__ ((aligned(4)));
#endif
//sCanvas Canvas; // canvas of draw box

// image (RPi)
u8 Img[1280] __attribute__ ((aligned(4)));
sCanvas ImgCanvas; // canvas of image

// image (Peter)
u8 Img2[1024] __attribute__ ((aligned(4)));
sCanvas Img2Canvas; // canvas of image

// 16-color palette translation table
//u16 Pal16Trans[256];

// 4-color palette translation table
u32 Pal4Trans[256];

// initialize videomode
void VideoInit()
{
    // setup videomode
    VgaCfgDef(&Cfg); // get default configuration
    Cfg.video = &DRV; // video timings
    Cfg.width = WIDTH; // screen width
    Cfg.height = HEIGHT; // screen height
#ifdef DOUBLEY
    Cfg.dbly = true;
#endif
    VgaCfg(&Cfg, &Vmode); // calculate videomode setup

    // initialize base layer 0
    ScreenClear(pScreen);
    sStrip* t = ScreenAddStrip(pScreen, HEIGHT);

    // draw box
    sSegm* g = ScreenAddSegm(t, WIDTH);

#if FORMAT==CANVAS_8
    ScreenSegmGraph8(g, Box, WIDTHBYTE);
    memcpy(Img, RPi8Img, sizeof(RPi8Img));
    memcpy(Img2, Peter8Img, sizeof(Peter8Img));

#elif FORMAT==CANVAS_4
    GenPal16Trans(Pal16Trans, DefPal16); // generate palette translation table
    ScreenSegmGraph4(g, Box, Pal16Trans, WIDTHBYTE);

    memcpy(Img, RPi4Img, sizeof(RPi4Img));
    memcpy(Img2, Peter4Img, sizeof(Peter4Img));

#elif FORMAT==CANVAS_2
    GenPal4Trans(Pal4Trans, PalCGA6); // generate palette translation table
    ScreenSegmGraph2(g, Box, Pal4Trans, WIDTHBYTE);

    memcpy(Img, RPi2Img, sizeof(RPi2Img));
    memcpy(Img2, Peter2Img, sizeof(Peter2Img));

#elif FORMAT==CANVAS_1
    ScreenSegmGraph1(g, Box, COL_BLACK, COL_WHITE, WIDTHBYTE);

    memcpy(Img, RPi1Img, sizeof(RPi1Img));
    ImgInvert(Img, sizeof(RPi1Img));
    memcpy(Img2, Peter1Img, sizeof(Peter1Img));
    ImgInvert(Img2, sizeof(Peter1Img));

#elif FORMAT==CANVAS_PLANE2
    GenPal4Plane(Pal4Trans, PalCGA2); // generate palette translation table
    ScreenSegmPlane2(g, Box, WIDTHBYTE*HEIGHT, Pal4Trans, WIDTHBYTE);
    Canvas.img2 = &Box[WIDTHBYTE*HEIGHT];

    ImgCanvas.img2 = &Img[IMGWIDTHBYTE*IMGHEIGHT];
    Plane2Conv(Img, ImgCanvas.img2, RPi2Img, IMGWIDTH, IMGHEIGHT);

    Img2Canvas.img2 = &Img2[IMG2WIDTHBYTE*IMG2HEIGHT];
    Plane2Conv(Img2, Img2Canvas.img2, Peter2Img, IMG2WIDTH, IMG2HEIGHT);

#elif FORMAT==CANVAS_ATTRIB8
    ScreenSegmAttrib8(g, Box, &Box[WIDTHBYTE*HEIGHT], DefPal16, WIDTHBYTE);
    Canvas.img2 = &Box[WIDTHBYTE*HEIGHT];

    ImgCanvas.img2 = &Img[IMGWIDTHBYTE*IMGHEIGHT];
    Attr8Conv(Img, ImgCanvas.img2, RPi4Img, IMGWIDTH, IMGHEIGHT, DefPal16);

    Img2Canvas.img2 = &Img2[IMG2WIDTHBYTE*IMG2HEIGHT];
    Attr8Conv(Img2, Img2Canvas.img2, Peter4Img, IMG2WIDTH, IMG2HEIGHT, DefPal16);
#endif

    // canvas of draw box
    Canvas.img = Box;
    Canvas.w = WIDTH;
    Canvas.h = HEIGHT;
    Canvas.wb = WIDTHBYTE;
    Canvas.format = FORMAT;

    // canvas of image
    ImgCanvas.img = Img;
    ImgCanvas.w = IMGWIDTH;
    ImgCanvas.h = IMGHEIGHT;
    ImgCanvas.wb = IMGWIDTHBYTE;
    ImgCanvas.format = FORMAT;

    // canvas of image 2
    Img2Canvas.img = Img2;
    Img2Canvas.w = IMG2WIDTH;
    Img2Canvas.h = IMG2HEIGHT;
    Img2Canvas.wb = IMG2WIDTHBYTE;
    Img2Canvas.format = FORMAT;

    // initialize system clock
    set_sys_clock_pll(Vmode.vco*1000, Vmode.pd1, Vmode.pd2);

    // initialize videomode
    VgaInitReq(&Vmode);

}

void initialize_video(void)
{
    // run VGA core
    StartVgaCore();

    // initialize videomode
    VideoInit();
}

void halt_video(void)
{
    VgaInitReq(NULL);
}

int main2()
{
    Bool slow = True;
    u32 t, t2;

    initialize_video();

    // initialize debug LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // main loop
    t = time_us_32();
    while (true)
    {
        switch (RandU8Max(7))
        {
        case 0:
            DrawRect(&Canvas, RandS16MinMax(-50, WIDTH), RandS16MinMax(-50, HEIGHT),
                RandU16MinMax(1, 100), RandU16MinMax(1, 100), RandU8Max(MAXCOL));
            break;

        case 1:
            DrawFrame(&Canvas, RandS16MinMax(-50, WIDTH), RandS16MinMax(-50, HEIGHT),
                RandU16MinMax(4, 100), RandU16MinMax(4, 100), RandU8Max(MAXCOL));
            break;

        case 2:
            DrawPoint(&Canvas, RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50), RandU8Max(MAXCOL));
            break;

        case 3:
            DrawLine(&Canvas, RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50),
                RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50), RandU8Max(MAXCOL));
            break;

        case 4:
            DrawFillCircle(&Canvas, RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50),
                RandU16MinMax(1, 50), RandU8Max(MAXCOL), RandU8());
            break;

        case 5:
            DrawCircle(&Canvas, RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50),
                RandU16MinMax(1, 50), RandU8Max(MAXCOL), RandU8());
            break;

        case 6:
            DrawText(&Canvas, "Hello!", RandS16MinMax(-50, WIDTH+50), RandS16MinMax(-50, HEIGHT+50), RandU8Max(MAXCOL),
                Fonts[RandU8Max(4)], 8, RandS8MinMax(1,4), RandS8MinMax(1,4));
            break;

        case 7:
            if (RandU8Max(1)==0)
            {
#if FORMAT == CANVAS_ATTRIB8 
                DrawImg(&Canvas, &Img2Canvas, RandS16MinMax(-50, WIDTH-10) & ~7, RandS16MinMax(-50, HEIGHT-10) & ~7,
                    0, 0, IMG2WIDTH, IMG2HEIGHT);
#else
                DrawImg(&Canvas, &Img2Canvas, RandS16MinMax(-50, WIDTH-10), RandS16MinMax(-50, HEIGHT-10),
                    0, 0, IMG2WIDTH, IMG2HEIGHT);
#endif
            }
            else
            {
#if FORMAT == CANVAS_ATTRIB8 
                DrawBlit(&Canvas, &ImgCanvas, RandS16MinMax(-50, WIDTH-10) & ~7, RandS16MinMax(-50, HEIGHT-10) & ~7,
                    0, 0, IMGWIDTH, IMGHEIGHT, MAXCOL);
#else
                DrawBlit(&Canvas, &ImgCanvas, RandS16MinMax(-50, WIDTH-10), RandS16MinMax(-50, HEIGHT-10),
                    0, 0, IMGWIDTH, IMGHEIGHT, MAXCOL);
#endif
            }
            break;
        }

        // change mode after some time
        t2 = time_us_32();
        if (slow)
        {
            if ((t2 - t) > 8000000)
            {
                slow = False;
                t = t2;
            }
            sleep_ms(50);
        }
        else
        {
            if ((t2 - t) > 2000000)
            {
                slow = True;
                t = t2;
            }
        }
        gpio_put(LED_PIN, slow);
    }
}
