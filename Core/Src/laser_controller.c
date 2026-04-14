#include "laser_controller.h"
#include "common.h"
#include "stm32f4xx_hal.h"
#include "one_pulse_mode_TIM1.h"


// Массив конфигураций
time_range_config_t time_range_table[RANGE_COUNT] = {
    { .psc = 10,		.ticks_per_unit = 10,		.label = "us"},		// OPM_MODE
    { .psc = 10000,		.ticks_per_unit = 10,		.label = "ms"},		// OPM_MODE
    { .psc = 1000,		.ticks_per_unit = 10,		.label = "sec"},	// LONG MODE - останавливаем через прерывания
    { .psc = 1000,		.ticks_per_unit = 10,		.label = "cont"}	// LONG MODE - останавливаем через прерывания
};


void init_laser_controller(laser_t* laser)
{
	laser->pulse_type = SHORT_PULSE;

	laser->deadtime = DEADTIME_TICKS;
	laser->pulse_duration = START_PULSE_DURATION;
	laser->is_active = false;

	laser->time_range_config = &time_range_table[RANGE_US];

	laser->switch_used = false;
	laser->switch_state = false;
	laser->last_update_time = 0;
	laser->elapsed_time = 0;
	laser->max_pulse_time = MAX_CONTINIOUS_PULSE_TIME;
	laser->experiment_aborted = false;

	uint32_t pulses = laser->pulse_duration * laser->time_range_config->ticks_per_unit;
	uint32_t arr = 10010; // из рассчета, что длительность не превышает 1000 единиц

	TIM1_OnePulse_Init(pulses, arr, laser->time_range_config->psc, laser->deadtime);
}


// Генерация импульса длительностью до от 1 мс до 1 сек с точностью duration - deadtime
void laser_generate_short_pulse(laser_t* laser)
{
	uint32_t arr = 0;
	if (laser->pulse_duration <= 0 || laser->pulse_duration > 1001)
	{
		laser->is_active = false;
		return;
	}

	uint32_t pulses = laser->pulse_duration * laser->time_range_config->ticks_per_unit;

	if (pulses <= 1000)
	{
		arr = 1010;
	}
	else
	{
		arr = 10010; // из рассчета, что длительность не превышает 1000 единиц
	}

	TIM1_generate_short_pulse(pulses, arr, laser->time_range_config->psc);
}


// Генерация импульса длительностью от 1 сек с точностью в несколько мс
void laser_generate_long_pulse(laser_t* laser)
{
	uint32_t arr = 10000;
	if (laser->pulse_duration <= 0 || laser->pulse_duration > 101)
	{
		laser->is_active = false;
		return;
	}

	uint32_t pulses = laser->pulse_duration * laser->time_range_config->ticks_per_unit;

	TIM1_generate_long_pulse(pulses, arr, laser->time_range_config->psc);
}


// Генерация непрерывного импульса. Используется функция для долгого импульса.
// Вся логика обработки в прерывании таймера
void laser_generate_continuous_pulse(laser_t* laser)
{
	uint32_t arr = 10000;
	TIM1_generate_long_pulse(1, arr, laser->time_range_config->psc);
}
