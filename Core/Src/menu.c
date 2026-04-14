#include "menu.h"
#include "common.h"

#include "laser_controller.h"
#include "encoder.h"
#include "cooler.h"
#include "peltier.h"

extern laser_t laser;
extern encoder_t encoder;
extern cooler_t cooler;
extern peltier_t peltier;
extern time_range_config_t time_range_table[RANGE_COUNT];

void menu_init(menu_t* menu)
{
	menu->menu_state = MENU_STATE_SET_TIME;
	menu->position = 0;
	menu->digit_state = DIGIT_STATE_1;
	menu->digit = 1;
	menu->number_of_unit = (uint8_t)RANGE_US;
	menu->unit = time_range_table[RANGE_US].label;

    encoder.position_norm = laser.pulse_duration;
    encoder.min_limit = MIN_SHORT_PULSE_DURATION;
    encoder.max_limit = MAX_SHORT_PULSE_DURATION;
}


void menu_update_state(menu_t* menu)
{
    switch (menu->menu_state)
    {
        case MENU_STATE_SET_TIME:
            menu->menu_state = MENU_STATE_SET_COOLER;
            menu->position = 1;

            encoder.position_norm = cooler.power;
            encoder.min_limit = MIN_COOLER_POWER;
            encoder.max_limit = MAX_COOLER_POWER;
            break;

        case MENU_STATE_SET_COOLER:
            menu->menu_state = MENU_STATE_SET_PELTIER;
            menu->position = 2;

            encoder.position_norm = peltier.power;
            encoder.min_limit = MIN_PELTIER_POWER;
            encoder.max_limit = MAX_PELTIER_POWER;
            break;

        case MENU_STATE_SET_PELTIER:
            menu->menu_state = MENU_STATE_SET_TIME;
            menu->position = 0;

            encoder.position_norm = laser.pulse_duration;

        	if (menu->number_of_unit == RANGE_SEC)
        	{
                encoder.min_limit = MIN_LONG_PULSE_DURATION;
                encoder.max_limit = MAX_LONG_PULSE_DURATION;
        	}
        	else
        	{
                encoder.min_limit = MIN_SHORT_PULSE_DURATION;
                encoder.max_limit = MAX_SHORT_PULSE_DURATION;
        	}
            break;
    }

    menu->digit_state = DIGIT_STATE_1;
    menu->digit = 1;

    encoder.last_direction = 0;
}


void menu_update_value(menu_t* menu)
{
	int32_t new_value = 0;

    // Если движения не было, ничего не делаем
    if (encoder.last_direction == ENC_DIR_NONE) return;

    switch (menu->menu_state)
    {
        case MENU_STATE_SET_TIME:
        	// Регулируем единицы измерения
        	if (menu->digit_state == DIGIT_STATE_UNIT)
        	{
        		new_value = menu->number_of_unit + (int8_t)encoder.last_direction;

            	menu->number_of_unit = (new_value > MAX_UNITS_COUNT) ? MAX_UNITS_COUNT:
            			(new_value < 0) ? 0 : new_value;
        		menu->unit = time_range_table[(time_range_t)menu->number_of_unit].label;

        		laser.time_range_config = &time_range_table[(time_range_t)menu->number_of_unit];

        		encoder.position_norm = menu->number_of_unit;

        		// если мкс или мс, то short_pulse, иначе long_pulse
        		if (menu->number_of_unit == RANGE_SEC)
        		{
        			laser.pulse_type = LONG_PULSE;
        			// Проверяем границы импульса, поскольку не может быть импульса дольше 100сек
                	laser.pulse_duration = (laser.pulse_duration > MAX_LONG_PULSE_DURATION) ? MAX_LONG_PULSE_DURATION :
                			(laser.pulse_duration < MIN_LONG_PULSE_DURATION) ? MIN_LONG_PULSE_DURATION : laser.pulse_duration;
                	encoder.position_norm = laser.pulse_duration;
        		}
        		else
        		{
        			laser.pulse_type = SHORT_PULSE;
        		}
        	}
        	// Регулируем само время в зависимости от единиц времени
        	else
        	{
            	new_value = laser.pulse_duration + encoder.last_direction * menu->digit;

            	if (menu->number_of_unit == RANGE_SEC)
            	{
                	laser.pulse_duration = (new_value > MAX_LONG_PULSE_DURATION) ? MAX_LONG_PULSE_DURATION :
                			(new_value < MIN_LONG_PULSE_DURATION) ? MIN_LONG_PULSE_DURATION : new_value;
            	}
            	else
            	{
                	laser.pulse_duration = (new_value > MAX_SHORT_PULSE_DURATION) ? MAX_SHORT_PULSE_DURATION :
                			(new_value < MIN_SHORT_PULSE_DURATION) ? MIN_SHORT_PULSE_DURATION : new_value;
            	}

            	encoder.position_norm = laser.pulse_duration;
        	}
            break;

        case MENU_STATE_SET_COOLER:
        	new_value = cooler.power + encoder.last_direction * menu->digit;

        	cooler.power = (new_value > MAX_COOLER_POWER) ? MAX_COOLER_POWER :
        	                      (new_value < MIN_COOLER_POWER) ? MIN_COOLER_POWER : new_value;

        	encoder.position_norm = cooler.power;
            break;

        case MENU_STATE_SET_PELTIER:
        	new_value = peltier.power + encoder.last_direction * menu->digit;

        	peltier.power = (new_value > MAX_PELTIER_POWER) ? MAX_PELTIER_POWER :
        	                      (new_value < MIN_PELTIER_POWER) ? MIN_PELTIER_POWER : new_value;

        	encoder.position_norm = peltier.power;
            break;

        default:
            break;
    }
}


void menu_update_digit(menu_t* menu)
{
    switch (menu->digit_state)
    {
    	case DIGIT_STATE_UNIT:
    		menu->digit_state = DIGIT_STATE_1;
    		break;

        case DIGIT_STATE_1:
            menu->digit = 10;
            menu->digit_state = DIGIT_STATE_2;
            break;

        case DIGIT_STATE_2:
            if (menu->menu_state == MENU_STATE_SET_TIME)
            {
                menu->digit = 100;
                menu->digit_state = DIGIT_STATE_3;
            }
            else
            {
                menu->digit = 1;
                menu->digit_state = DIGIT_STATE_1;
            }
            break;

        case DIGIT_STATE_3:
            if (menu->menu_state == MENU_STATE_SET_TIME)
            {
                menu->digit = 1;
                menu->digit_state = DIGIT_STATE_UNIT;
            }
            else
            {
                menu->digit = 1;
                menu->digit_state = DIGIT_STATE_1;
            }

            break;

        default:
            break;
    }

    // Сбрасываем направление, чтобы старый импульс не применился с новым шагом
    encoder.last_direction = ENC_DIR_NONE;
}
