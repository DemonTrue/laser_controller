/**
 * @file button_handlers.c
 * @brief Реализация обработчиков событий кнопок.
 *
 * Реализует контекстно-зависимую логику обработки нажатий кнопок
 * для управления состояниями приложения, меню и экспериментов.
 */

#include "button_handlers.h"
#include <stdint.h>
#include "app_state.h"
#include "menu.h"
#include "common.h"
#include "main.h"

extern time_range_config_t time_range_table[RANGE_COUNT];

/**
 * @brief Обработчик короткого нажатия основной кнопки
 */
void on_main_button_short_press(void* context, button_id_t id)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;
    laser_t* laser = manager->app_context.laser;

    switch (manager->current_state)
    {
        case APP_STATE_INTRO:
            break;

        case APP_STATE_MENU:
        	if (!manager->app_context.laser_is_active)
        	{
        		manager->app_context.first_entry_state = true;

//        		printf("duration = %lu %s psc:%lu\r\n", laser->pulse_duration, laser->time_range_config->label, laser->time_range_config->psc);
        		if (laser->pulse_type == SHORT_PULSE)
        		{
		        	laser_generate_short_pulse(laser);
        		}
        		else if (laser->pulse_type == LONG_PULSE)
        		{
        			laser_generate_long_pulse(laser);
        		}

                app_state_transition(manager, APP_STATE_LASER_EXPERIMENT);
        	}
            break;

        case APP_STATE_LASER_EXPERIMENT:
        	//TODO: добавить возможность вырубать импульс
        	if (laser->pulse_type == LONG_PULSE && HAL_GetTick() - laser->last_update_time > ABORT_COOLDOWN_TIME)
        	{
        		laser->experiment_aborted = true;
        	}

            break;

        default:
            break;
    }
}


/**
 * @brief Обработчик длинного нажатия основной кнопки
 */
void on_main_button_long_press(void* context, button_id_t id)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;

    switch (manager->current_state)
    {
        case APP_STATE_INTRO:
            break;

        case APP_STATE_MENU:
        	// TODO: здесь переход между диапазонами времени??? мс\мкс\с
//        	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            break;

        default:
            break;
    }
}


/**
 * @brief Обработчик короткого нажатия кнопки энкодера
 */
void on_encoder_button_short_press(void* context, button_id_t id)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;

    menu_t* menu = manager->app_context.menu;

    switch (manager->current_state)
    {
    	case APP_STATE_INTRO:
    		break;

    	case APP_STATE_MENU:
    		// Обновляем разрядность
        	if (!manager->app_context.laser_is_active)
        	{
        		menu_update_digit(menu);
        	}
    		break;

        case APP_STATE_LASER_EXPERIMENT:
            break;

        default:
            break;
    }
}


/**
 * @brief Обработчик длинного нажатия кнопки энкодера
 */
void on_encoder_button_long_press(void* context, button_id_t id)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;
    menu_t* menu = manager->app_context.menu;

	switch (manager->current_state)
	{
		case APP_STATE_INTRO:
			break;

		case APP_STATE_MENU:
			// Переходим на регулировку следующего параметра
        	if (!manager->app_context.laser_is_active)
        	{
    			menu_update_state(menu);
        	}
			break;

	    default:
	        break;
	}
}


/**
 * @brief Обработчик переключателя
 */
void on_toggle_switch_state_change(void* context, toggle_switch_t* toggle_switch)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;
    laser_t* laser = manager->app_context.laser;
    uint8_t state = !toggle_switch_get_state(toggle_switch);

    switch (manager->current_state)
    {
        case APP_STATE_INTRO:
            break;

        case APP_STATE_MENU:
			if (state)
			{
				laser->pulse_type = CONTINUOUS_PULSE;

				laser->time_range_config = &time_range_table[RANGE_CONTINUOUS];
				laser_generate_continuous_pulse(laser);

        		manager->app_context.laser_is_active = true;
                manager->app_context.first_entry_state = true;
                laser->switch_used = true;
                laser->switch_state = true;
                app_state_transition(manager, APP_STATE_LASER_EXPERIMENT);
			}
			else
			{
				//TODO: ШТО???
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
				manager->app_context.laser_is_active = false;
			}
            break;

        case APP_STATE_LASER_EXPERIMENT:
            laser->switch_state = false;
            break;

        default:
            break;
    }
}
