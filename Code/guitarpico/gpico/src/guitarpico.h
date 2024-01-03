#ifndef _GUITARPICO_H
#define _GUITARPICO_H

#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/timer.h"

#define DAC_PWM_B3 15
#define DAC_PWM_B2 14
#define DAC_PWM_B1 13
#define DAC_PWM_B0 12

#define DAC_PWM_WRAP_VALUE 0x400

#define ADC_AUDIO_IN 26
#define ADC_CONTROL_IN 27
#define ADC_MAX_VALUE 4096
#define ADC_PREC_VALUE 16384

#define GPIO_ADC_SEL0 16
#define GPIO_ADC_SEL1 17
#define GPIO_ADC_SEL2 18

#define GPIO_BUTTON1 19
#define GPIO_BUTTON2 20
#define GPIO_BUTTON3 21
#define GPIO_BUTTON4 22
#define GPIO_BUTTON5 28

#define GUITARPICO_SAMPLERATE 25000u
#define POT_MAX_VALUE 16384u

#ifndef LED_PIN
#define LED_PIN 25
#endif /* LED_PIN */

#ifdef __cplusplus
extern "C"
{
#endif

uint16_t read_potentiometer_value(uint v);

#define POTENTIOMETER_VALUE_SENSITIVITY 20

#ifdef __cplusplus
}
#endif


#endif /* _GUITARPICO_H */
