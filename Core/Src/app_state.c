/**
 * @file app_context.c
 * @brief Реализация менеджера состояний приложения и обработчиков состояний.
 *
 * Реализует конечный автомат приложения с обработчиками для каждого состояния,
 * управление переходами между состояниями и обновление интерфейса.
 */

#include "app_state.h"
#include "stdio.h"
#include "stdbool.h"
#include "common.h"
#include "liquidcrystal_i2c.h"
#include "laser_controller.h"
#include "ui.h"
#include "encoder.h"
#include "cooler.h"
#include "peltier.h"
#include "toggle_switch.h"
#include "main.h"

extern laser_t laser;
extern encoder_t encoder;
extern cooler_t cooler;
extern peltier_t peltier;
extern menu_t menu;
extern toggle_switch_t toggle_switch;

static void check_encoder_limits(encoder_t* encoder, menu_t* menu, laser_t* laser, cooler_t* cooler, peltier_t* peltier);
static void switch_safety(void);


/**
 * @brief Функция, которая сообщает, что МК жив
 */
void blink_led(GPIO_TypeDef* gpio_port, uint16_t gpio_pin, uint32_t delay)
{
	static uint32_t last_led_update = 0;

    if (HAL_GetTick() - last_led_update > delay)
    {
        HAL_GPIO_TogglePin(gpio_port, gpio_pin);
        last_led_update = HAL_GetTick();
    }
}


/**
 * @brief Обработчик состояния интро (INTRO)
 * @param context Указатель на менеджер состояний приложения
 *
 * Отображает заставку и проверяет состояние тумблера, затем переходит в меню.
 */
void handle_intro_state(void* context)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;

    // Заставка
	ui_intro();

	// Проверка, не остался ли включенным тумблер
	switch_safety();

	app_state_transition(manager, APP_STATE_MENU);
	ui_clear();

}


void handle_menu_state(void* context)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;
    laser_t* laser = manager->app_context.laser;
    encoder_t* encoder = manager->app_context.encoder;
    cooler_t* cooler = manager->app_context.cooler;
    peltier_t* peltier = manager->app_context.peltier;
    menu_t* menu = manager->app_context.menu;

    static int32_t last_encoder_value = -1;
    static menu_state_t last_menu_state = 255;
    static uint8_t last_digit_state = 255;

    encoder_update(encoder);

    check_encoder_limits(encoder, menu, laser, cooler, peltier);

    if (manager->app_context.first_entry_state)
    {
        ui_update_menu(laser->pulse_duration, cooler->power, peltier->power,
        		menu->position, menu->digit_state, laser->time_range_config->label);
        manager->app_context.first_entry_state = false;
    }

    // Проверяем все, что может измениться
    if (encoder->position_norm != last_encoder_value ||
        menu->menu_state != last_menu_state ||
        menu->digit_state != last_digit_state)
    {
        menu_update_value(menu);

        ui_update_menu(laser->pulse_duration, cooler->power, peltier->power,
        		menu->position, menu->digit_state, menu->unit);

        last_encoder_value = encoder->position_norm;
        last_menu_state = menu->menu_state;
        last_digit_state = menu->digit_state;
    }
}


/**
 * @brief Обработчик состояния измерений (MEASURING)
 * @param context Указатель на менеджер состояний приложения
 *
 * Управляет активным экспериментом: обновляет измерения, контролирует мотор,
 * проверяет безопасность и обрабатывает состояния экспериментального автомата.
 */
void handle_experiment_state(void* context)
{
    if (context == NULL) return;

    app_state_manager_t* manager = (app_state_manager_t*)context;
    laser_t* laser = manager->app_context.laser;
    cooler_t* cooler = manager->app_context.cooler;
    peltier_t* peltier = manager->app_context.peltier;
    menu_t* menu = manager->app_context.menu;

    static uint32_t last_display_update = 0;
	uint32_t time_now = HAL_GetTick();

    if (manager->app_context.first_entry_state)
    {
    	manager->app_context.first_entry_state = false;
    	laser->is_active = true;

    	// Время для лазера обновляется лишь на старте
    	laser->last_update_time = time_now;
    	last_display_update = time_now;
		laser->elapsed_time = 0;

		HD44780_Clear();
    	// Используем разные функции ui для разных режимов
    	if (laser->switch_used)
    	{
    		ui_update_continuous_experiment(cooler->power, peltier->power,
                    menu->position, menu->digit_state, MAX_CONTINIOUS_PULSE_TIME);
    	}
    	else
    	{
    		ui_update_pulse_experiment(cooler->power, peltier->power,
                    menu->position, menu->digit_state);
    	}
    }

    if (laser->pulse_type == CONTINUOUS_PULSE && time_now - last_display_update > DISPLAY_EXPERIMENT_UPDATE_TIME)
    {
    	laser->elapsed_time = time_now - laser->last_update_time;

		ui_update_continuous_experiment(cooler->power, peltier->power,
                menu->position, menu->digit_state, MAX_CONTINIOUS_PULSE_TIME - laser->elapsed_time);

    	last_display_update = HAL_GetTick();

    	if (laser->elapsed_time > MAX_CONTINIOUS_PULSE_TIME)
    	{
			manager->app_context.laser_is_active = false;
			switch_safety();
    	}
    }

    // Проверяем выключился ли лазер. Если выключился, то гарантируем,
    // что приложение сразу не перейдет в меню и картинка успеет отобразиться
    if (!laser->is_active && (HAL_GetTick() - laser->last_update_time) > MIN_DISPLAY_PULSE_TIME)
    {
    	manager->app_context.laser_is_active = false;
    	manager->app_context.first_entry_state = true;
    	app_state_transition(manager, APP_STATE_MENU);
    }
}


/**
 * @brief Инициализация менеджера состояний приложения
 * @param manager Указатель на структуру менеджера состояний
 *
 * Выполняет начальную настройку менеджера: устанавливает начальное состояние,
 * инициализирует обработчики, связывает все модули системы через контекст.
 */

void app_state_init(app_state_manager_t* manager)
{
    // Инициализация состояний
    manager->current_state = APP_STATE_INTRO;
    manager->previous_state = APP_STATE_INTRO;

    // Инициализация всех обработчиков как NULL
    for (uint8_t i = 0; i < APP_STATE_COUNT; i++)
    {
        manager->state_handlers[i] = NULL;
    }

    // Связывание модулей системы через контекст приложения
    manager->app_context.laser = &laser;
    manager->app_context.encoder = &encoder;
    manager->app_context.cooler = &cooler;
    manager->app_context.peltier = &peltier;
    manager->app_context.menu = &menu;

    // Инициализация переменных управления отображением
    manager->app_context.last_display_update = 0;
    manager->app_context.laser_is_active = false;
    manager->app_context.first_entry_state = true;

    // Регистрация обработчиков для конкретных состояний
    manager->state_handlers[APP_STATE_INTRO] = handle_intro_state;
    manager->state_handlers[APP_STATE_MENU] = handle_menu_state;
    manager->state_handlers[APP_STATE_LASER_EXPERIMENT] = handle_experiment_state;
}


/**
 * @brief Выполнение перехода между состояниями приложения
 * @param manager Указатель на менеджер состояний
 * @param new_state Новое состояние для перехода
 *
 * Обновляет текущее и предыдущее состояния, обеспечивая корректные
 * переходы в конечном автомате приложения.
 */
void app_state_transition(app_state_manager_t* manager, app_state_t new_state)
{
    if (new_state >= APP_STATE_COUNT) return;

    manager->previous_state = manager->current_state;
    manager->current_state = new_state;
}


/**
 * @brief Основная функция обновления приложения
 * @param manager Указатель на менеджер состояний
 *
 * Вызывается в основном цикле программы для выполнения обработки
 * текущего состояния приложения через зарегистрированные обработчики.
 */
void app_update(app_state_manager_t* manager)
{
    if (manager == NULL) return;

    // Вызов обработчика текущего состояния, если он зарегистрирован
    if (manager->state_handlers[manager->current_state])
    {
        manager->state_handlers[manager->current_state](manager);
    }
}


/**
 * @brief Обработчик состояния измерений (MEASURING)
 * @param encoder Указатель на энкодер
 * @param menu Указатель на меню
 * @param laser Указатель на лазер
 * @param cooler Указатель на вентилятор
 * @param peltier Указатель на элемент Пельте
 *
 * Проверка границ значений энкодера в случае изменения айтема меню
 */
static void check_encoder_limits(encoder_t* encoder, menu_t* menu, laser_t* laser, cooler_t* cooler, peltier_t* peltier)
{
    if (menu->digit_state == DIGIT_STATE_UNIT)
    {
        // Если мы выбираем единицы, лимиты должны быть 0..2
        if (encoder->min_limit != 0 || encoder->max_limit != MAX_UNITS_COUNT)
        {
            encoder->min_limit = 0;
            encoder->max_limit = MAX_UNITS_COUNT;
            encoder->position_norm = menu->number_of_unit;
            encoder->remainder = 0;
        }
    }
    else if (menu->position == 0 && menu->digit_state != DIGIT_STATE_UNIT) // TIME
    {
        // Если вернулись к редактированию времени, возвращаем лимиты лазера
    	if (menu->number_of_unit == RANGE_SEC)
    	{
			if (encoder->min_limit != MIN_LONG_PULSE_DURATION || encoder->max_limit != MAX_LONG_PULSE_DURATION)
			{
				encoder->min_limit = MIN_LONG_PULSE_DURATION;
				encoder->max_limit = MAX_LONG_PULSE_DURATION;
				encoder->position_norm = laser->pulse_duration;
				encoder->remainder = 0;
			}
    	}
    	else
    	{
			if (encoder->min_limit != MIN_SHORT_PULSE_DURATION || encoder->max_limit != MAX_SHORT_PULSE_DURATION)
			{
				encoder->min_limit = MIN_SHORT_PULSE_DURATION;
				encoder->max_limit = MAX_SHORT_PULSE_DURATION;
				encoder->position_norm = laser->pulse_duration;
				encoder->remainder = 0;
			}
    	}

    }
    else if (menu->position == 1) // FAN
    {
        if (encoder->min_limit != MIN_COOLER_POWER || encoder->max_limit != MAX_COOLER_POWER)
        {
			 // Лимиты для вентилятора
			 encoder->min_limit = MIN_COOLER_POWER;
			 encoder->max_limit = MAX_COOLER_POWER;
			 encoder->position_norm = cooler->power;
			 encoder->remainder = 0;
        }
    }
    else if (menu->position == 2) // PELTIER
    {
        if (encoder->min_limit != MIN_PELTIER_POWER || encoder->max_limit != MAX_PELTIER_POWER)
        {
			 // Лимиты для Пельтье
			 encoder->min_limit = MIN_PELTIER_POWER;
			 encoder->max_limit = MAX_PELTIER_POWER;
			 encoder->position_norm = peltier->power;
			 encoder->remainder = 0;
        }
    }

}


/**
 * @brief Проверка тумблера на отключение
 * @param None
 *
 * Проверяет, включен ли тумблер. Если включен, то не пускает, пока не выключат
 */
static void switch_safety(void)
{
	uint8_t state = !toggle_switch_get_state(&toggle_switch);
	if (state)
	{
		ui_switch_warning();

		while(state)
		{
			blink_led(LED_GPIO_Port, LED_Pin, ERROR_BLINK_DELAY);
			state = !toggle_switch_get_state(&toggle_switch);
		}
	}
}
