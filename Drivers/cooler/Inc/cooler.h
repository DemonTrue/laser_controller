/* cooler.h */
#ifndef COOLER_H
#define COOLER_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"


typedef struct {
	TIM_TypeDef* htim;
    uint32_t tim_channel;
	uint8_t power;
} cooler_t;


void cooler_init(cooler_t* cooler);

#endif /* COOLER_H */
