/* peltier.h */
#ifndef PELTIER_H
#define PELTIER_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"


typedef struct {
	TIM_TypeDef* htim;
    uint32_t tim_channel;

	uint8_t power;
} peltier_t;

void peltier_init(peltier_t* peltier);

#endif /* PELTIER_H */
