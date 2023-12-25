
// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#include "main.h"
#include "guitarpico.h"
#include "ssd1306_i2c.h"
#include "buttons.h"
#include <string.h>
#include <stdio.h>


#define POLLING_FREQUENCY_HZ 20000u
#define POLLING_FREQUENCY_US (1000000u/POLLING_FREQUENCY_HZ)

const u8* Fonts[5] = { FontBold8x8, FontGame8x8, FontIbm8x8, FontItalic8x8, FontThin8x8 };

#define NUMBER_OF_DMA_SAMPLES 16
uint16_t sample_capture_buf[NUMBER_OF_DMA_SAMPLES];

uint adc_dma_chan;
volatile uint32_t counter = 0;
volatile uint16_t p = 0;

uint dac_pwm_b3_slice_num, dac_pwm_b2_slice_num, dac_pwm_b1_slice_num, dac_pwm_b0_slice_num;
repeating_timer_t sampling_port_func_repeating_timer_t;

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

static inline void adc_start(void) {
	hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
}

void adc_start_fifo_sample_channels(void)
{
	dma_channel_config cfg = dma_channel_get_default_config(adc_dma_chan);
	channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
	channel_config_set_read_increment(&cfg, false);
	channel_config_set_write_increment(&cfg, true);
	channel_config_set_dreq(&cfg, DREQ_ADC);
	dma_channel_configure(adc_dma_chan, &cfg, sample_capture_buf, &adc_hw->fifo, NUMBER_OF_DMA_SAMPLES, true);
	adc_select_input(0);
	adc_run(true);
}

void adc_stop_fifo_sample_channels()
{

	dma_channel_wait_for_finish_blocking(adc_dma_chan);
	adc_run(false);
	adc_fifo_drain();
}


void initialize_adc(void)
{
	adc_init();
	adc_set_clkdiv(0);
	adc_gpio_init(ADC_AUDIO_IN);
	adc_gpio_init(ADC_CONTROL_IN);
	adc_irq_set_enabled(false);
	adc_fifo_setup(true, true, 1, true, false);
	adc_set_round_robin(0x03);
	
	adc_dma_chan = dma_claim_unused_channel(true);

	adc_start_fifo_sample_channels();
}

uint control_sample = 0;
uint16_t control_samples[8];


//bool sampling_port_func(repeating_timer *rt)
extern "C" bool __not_in_flash_func(sampling_port_func(repeating_timer *rt))
{
	//adc_stop_fifo_sample_channels();
	control_samples[control_sample] = sample_capture_buf[1];
	control_sample = (control_sample >= 7) ? 0 : (control_sample+1);
	gpio_put(GPIO_ADC_SEL0, (control_sample & 0x01) == 0);
	gpio_put(GPIO_ADC_SEL1, (control_sample & 0x02) == 0);
	gpio_put(GPIO_ADC_SEL2, (control_sample & 0x04) == 0);
	//adc_start_fifo_sample_channels();

	counter++;
	if ((counter & 0x3FFF) == 0x3FFF)
	{
		p++;
		if (p > DAC_PWM_WRAP_VALUE)
			p = 0;
		pwm_set_both_levels(dac_pwm_b3_slice_num, p, p);
		pwm_set_both_levels(dac_pwm_b1_slice_num, p, p);
		gpio_put(25,!gpio_get(25));
	}
	
	return true;
}

void initialize_periodic_alarm(void)
{
	alarm_pool_t *alarmpool = alarm_pool_create_with_unused_hardware_alarm(1);
	alarm_pool_add_repeating_timer_us(alarmpool, POLLING_FREQUENCY_US, sampling_port_func, NULL, &sampling_port_func_repeating_timer_t); 
}


void test_all_control(void)
{
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

}

char buttonpressed(uint8_t b)
{
	return button_readbutton(b) ? '1' : '0';
}

int main2();

int main()
{
	char str[20];
	
	stdio_init_all();
	initialize_gpio();
	buttons_initialize();
	initialize_pwm();
//	initialize_adc();
	ssd1306_Initialize();
	//initialize_periodic_alarm();

	
	main2();
	for (;;)
	{
		buttons_poll();
		if ((counter & 0xFFF) == 0xFFF)
		{
			ssd1306_Clear_Buffer();
			sprintf(str,"%u %c%c%c%c%c",counter,buttonpressed(0),buttonpressed(1),buttonpressed(2),buttonpressed(3),buttonpressed(4));
			ssd1306_WriteString(0,0,str);
			ssd1306_render();
			printf("test1\n");
			test_all_control();
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

int main2()
{
	Bool slow = True;
	u32 t, t2;

	// run VGA core
	StartVgaCore();

	// initialize videomode
	VideoInit();

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
