/* guitarpico.h

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

#ifndef _GUITARPICO_H
#define _GUITARPICO_H

#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "pico/multicore.h"

#define DMB() __dmb()

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
#define POTENTIOMETER_MAX 6

#ifdef __cplusplus
}
#endif


#endif /* _GUITARPICO_H */
