
// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#include "main.h"
#include "guitarpico.h"
#include "ssd1306_i2c.h"
#include "buttons.h"
#include "dsp.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

void initialize_video(void);

#define POLLING_FREQUENCY_HZ 50000u
#define POLLING_FREQUENCY_US (1000000u/POLLING_FREQUENCY_HZ)

uint dac_pwm_b3_slice_num, dac_pwm_b2_slice_num, dac_pwm_b1_slice_num, dac_pwm_b0_slice_num;
uint claimed_alarm_num;
absolute_time_t last_alarm_time;

const u8* Fonts[5] = { FontBold8x8, FontGame8x8, FontIbm8x8, FontItalic8x8, FontThin8x8 };

volatile uint32_t counter = 0;
volatile uint16_t p = 0;

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
	
	pwm_set_wrap(dac_pwm_b3_slice_num, DAC_PWM_WRAP_VALUE);
	pwm_set_wrap(dac_pwm_b2_slice_num, DAC_PWM_WRAP_VALUE);
	pwm_set_wrap(dac_pwm_b1_slice_num, DAC_PWM_WRAP_VALUE);
	pwm_set_wrap(dac_pwm_b0_slice_num, DAC_PWM_WRAP_VALUE);
	
	pwm_set_output_polarity(dac_pwm_b3_slice_num, false, false);
	pwm_set_output_polarity(dac_pwm_b2_slice_num, false, false);
	pwm_set_output_polarity(dac_pwm_b1_slice_num, false, false);
	pwm_set_output_polarity(dac_pwm_b0_slice_num, false, false);
	
	pwm_set_enabled(dac_pwm_b3_slice_num, true);
	pwm_set_enabled(dac_pwm_b2_slice_num, true);
	pwm_set_enabled(dac_pwm_b1_slice_num, true);
	pwm_set_enabled(dac_pwm_b0_slice_num, true);
}

uint current_input = 0;
uint control_sample_no = 0;
uint16_t control_samples[8];

volatile uint32_t last1=0,last2=0,last3=0;
volatile uint32_t dly1,dly2,dly3;

uint16_t sine_ctr = 0;
uint16_t sine_ctr_frac = 2000;

//extern "C" void __not_in_flash_func(alarm_func(uint alarm_num))
void alarm_func(uint alarm_num)
{
	uint16_t sample;

	sample = adc_hw->result;
	uint32_t cur_time = timer_hw->timelr;
	absolute_time_t next_alarm_time = make_timeout_time_us(current_input ? 10 : 30);
	hardware_alarm_set_target(claimed_alarm_num, next_alarm_time);
	last_alarm_time = next_alarm_time;
	current_input = (current_input+1) & 0x01;
	adc_select_input(current_input);
	if (current_input == 0)
	{
		dly3 = cur_time-last3;
		control_samples[control_sample_no] = sample;
		control_sample_no = (control_sample_no >= 7) ? 0 : (control_sample_no+1);
		gpio_put(GPIO_ADC_SEL0, (control_sample_no & 0x01) == 0);
		gpio_put(GPIO_ADC_SEL1, (control_sample_no & 0x02) == 0);
		gpio_put(GPIO_ADC_SEL2, (control_sample_no & 0x04) == 0);
		last2 = cur_time;
		return;
	} 

	dly1 = cur_time-last1;
	last1 = cur_time;
	last3 = cur_time;
	dly2 = cur_time-last2;

	int16_t s = (sample << 3) - 16384;
	s = dsp_process_all_units(s);
	insert_sample_circ_buf(s);
	if (s > 16383) p = 255;
	else if (s < -16384) p = 0;
	else p = (s+16384) / 32;
	pwm_set_both_levels(dac_pwm_b3_slice_num, p, p);
	pwm_set_both_levels(dac_pwm_b1_slice_num, p, p);

	counter++;
	/*
	if ((counter & 0x7FFF) == 0x7FFF)
	{
		gpio_put(25,!gpio_get(25));
	} */
	return;
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

void test_all_control(void)
{
	char s1[100];
	char s2[100];
	sprintf(s1,"counter:%d %u\n",counter,(uint32_t)((((uint64_t)counter)*1000000)/time_us_32()));
	sprintf(s2,"dly1: %u dly2: %u dly3: %u\n",dly1,dly2,dly3);
	printf("%s%s",s1,s2);
	for (int i=0;i<8;i++)
		printf("control:%d value:%d\n",i,control_samples[i]);
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

void initialize_periodic_alarm(void)
{
	for (uint alarm_no=0;alarm_no<4;alarm_no++)
	{
		if (!hardware_alarm_is_claimed(alarm_no))
		{
			hardware_alarm_claim_unused(alarm_no);
			claimed_alarm_num = alarm_no;
			hardware_alarm_set_callback(claimed_alarm_num, alarm_func);
			last_alarm_time = get_absolute_time();
			absolute_time_t next_alarm_time = delayed_by_us(last_alarm_time, POLLING_FREQUENCY_US);
			hardware_alarm_set_target(claimed_alarm_num, next_alarm_time);
			last_alarm_time = next_alarm_time;
			
		}
	}
}

char buttonpressed(uint8_t b)
{
	return button_readbutton(b) ? '1' : '0';
}

int main2();

void initialize_sine_counter()
{
	dsp_type_sine_synth dtss;
	dtss.sine_counter_inc = 2000;
	dsp_initialize(DSP_TYPE_SINE_SYNTH, (void *)&dtss, dsp_unit_entry(0));
}

int main()
{
	char str[40];
	
	stdio_init_all();
	initialize_dsp();
	//initialize_sine_counter();
	initialize_gpio();
	buttons_initialize();
	initialize_pwm();
	ssd1306_Initialize();
	initialize_video();
	initialize_adc();
	initialize_periodic_alarm();
	
	for (;;)
	{
		uint32_t last_time = time_us_32();
		while ((time_us_32() - last_time) < 100000)
			buttons_poll();
		ssd1306_Clear_Buffer();
		sprintf(str,"%u %c%c%c%c%c",counter,buttonpressed(0),buttonpressed(1),buttonpressed(2),buttonpressed(3),buttonpressed(4));
		ssd1306_WriteString(0,0,str);
		sprintf(str,"dlys: %u %u %u",dly1,dly2,dly3);
		ssd1306_WriteString(0,8,str);
		sprintf(str,"c1: %u %u",control_samples[0],control_samples[1]);
		ssd1306_WriteString(0,16,str);
		sprintf(str,"c2: %u %u",control_samples[2],control_samples[3]);
		ssd1306_WriteString(0,24,str);
		sprintf(str,"buf: %d",sample_circ_buf_value(0));
		ssd1306_WriteString(0,32,str);
		sprintf(str,"rate %u",(uint32_t)((((uint64_t)counter)*1000000)/time_us_32()));
		ssd1306_WriteString(0,40,str);
		ssd1306_render();
		test_all_control();
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