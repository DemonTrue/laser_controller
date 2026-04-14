/* ui.h */
#ifndef UI_H
#define UI_H

#include "stdint.h"

typedef enum {
	UI_STATE_INTRO,
    UI_STATE_MENU,
    UI_STATE_LASER_PULSE,
    UI_STATE_LASER_CONTINIOUS
} ui_state_t;


void ui_init(void);
void ui_intro(void);
void ui_switch_warning(void);
void ui_clear(void);
void ui_update_menu(int32_t time_value, int32_t cooler_value, int32_t peltier_value,
		uint8_t position, uint8_t digit_state, const char* time_scale);
void ui_update_pulse_experiment(int32_t cooler_value, int32_t peltier_value,
        uint8_t position, uint8_t digit_state);
void ui_update_continuous_experiment(int32_t cooler_value, int32_t peltier_value,
        uint8_t position, uint8_t digit_state, uint32_t elapsed_time);

#endif /* UI_H */
