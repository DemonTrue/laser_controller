/* one_pulse_mode_TIM1.h */
#ifndef OPM_TIM1_H
#define OPM_TIM1_H

#include "tim.h"
#include "stm32f4xx_hal.h"

void TIM1_OnePulse_Init(uint32_t pulse_width, uint32_t arr, uint32_t psc, uint16_t deadtime);
void TIM1_generate_short_pulse(uint32_t pulse_width, uint32_t arr, uint32_t psc);
void TIM1_generate_long_pulse(uint32_t duration, uint32_t arr, uint32_t psc);

#endif /* OPM_TIM1_H */
