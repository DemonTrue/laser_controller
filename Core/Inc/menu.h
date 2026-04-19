#ifndef MENU_H
#define MENU_H

#include "stdint.h"

typedef enum {
    MENU_STATE_SET_TIME,
	MENU_STATE_SET_PELTIER,
	MENU_STATE_SET_COOLER,
} menu_state_t;


typedef enum {
	DIGIT_STATE_UNIT,	// единицы измерения
	DIGIT_STATE_1,		// +1
	DIGIT_STATE_2,		// +10
	DIGIT_STATE_3,		// +100
	DIGIT_STATE_4,		// +1000
} digit_state_t;


typedef struct {
	menu_state_t menu_state;
	uint8_t position;
	digit_state_t digit_state;
	uint16_t digit;
	int8_t number_of_unit; //TODO: подумать как согласовать с лазером на старте...
	const char* unit;
} menu_t;


void menu_init(menu_t* menu);
void menu_update_state(menu_t* menu);
void menu_update_value(menu_t* menu);
void menu_update_digit(menu_t* menu);

#endif /* MENU_H */
