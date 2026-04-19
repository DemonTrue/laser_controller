#include "laser_controller.h"
#include "common.h"
#include "stm32f4xx_hal.h"
#include "one_pulse_mode_TIM1.h"


extern volatile uint32_t pulse_tick_count;
extern volatile uint32_t pulse_target_ticks;
extern volatile uint8_t is_pulse_active;

// Массив конфигураций
time_range_config_t time_range_table[RANGE_COUNT] = {
    { .psc = 10,		.ticks_per_unit = 10,		.label = "us",		.type = RANGE_US},			// OPM_MODE
    { .psc = 10000,		.ticks_per_unit = 10,		.label = "ms",		.type = RANGE_MS},			// OPM_MODE
    { .psc = 1000,		.ticks_per_unit = 10,		.label = "sec",		.type = RANGE_SEC},			// LONG MODE - останавливаем через прерывания
    { .psc = 1000,		.ticks_per_unit = 10,		.label = "cont",	.type = RANGE_CONTINUOUS}	// LONG MODE - останавливаем через прерывания
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


void laser_handle_up_interrupt(laser_t* laser)
{
    // Проверяем, наше ли это прерывание (флаг UIF)
    if (TIM1->SR & TIM_SR_UIF)
    {
        // Сразу сбрасываем флаг, иначе прерывание сработает бесконечно
        TIM1->SR &= ~TIM_SR_UIF;

        switch (laser->pulse_type)
        {
        	case SHORT_PULSE:
				// Выключаем прерывания
				TIM1->DIER &= ~TIM_DIER_UIE;

				laser->is_active = false;

				is_pulse_active = 0;
        		break;

        	case LONG_PULSE:
				if (is_pulse_active)
				{
					pulse_tick_count++;

					if (pulse_tick_count >= pulse_target_ticks || laser->experiment_aborted)
					{
						// Выключаем импульс (PWM Mode 2: Low when CNT <= CCR)
						TIM1->CCR1 = TIM1->ARR;

						is_pulse_active = 0;

						// Выключаем прерывания
						TIM1->DIER &= ~TIM_DIER_UIE;

						// Останавливаем таймер
						TIM1->CR1 &= ~TIM_CR1_CEN;
						while(TIM1->CR1 & TIM_CR1_CEN);

						laser->is_active = false;
						laser->experiment_aborted = false;
					}
				}
				break;

        	case CONTINUOUS_PULSE:
        		if (!laser->switch_state || laser->elapsed_time > laser->max_pulse_time)
        		{
					// Выключаем импульс (PWM Mode 2: Low when CNT <= CCR)
					TIM1->CCR1 = TIM1->ARR;

					// Выключаем прерывания
					TIM1->DIER &= ~TIM_DIER_UIE;
					is_pulse_active = 0;

					// Останавливаем таймер
					TIM1->CR1 &= ~TIM_CR1_CEN;
					while(TIM1->CR1 & TIM_CR1_CEN);

					laser->is_active = false;
					laser->switch_used = false;
					laser->switch_state = false; // Сброс на всякий случай

					//TODO: сделать это элегнатнее? Сохранять прошлое значение типа, например
					laser->pulse_type = SHORT_PULSE;
					laser->time_range_config = &time_range_table[RANGE_US];
        		}
        		break;
        }
    }
}


// Генерация импульса длительностью до от 1 мс до 1 сек с точностью duration - deadtime
void laser_generate_short_pulse(laser_t* laser)
{
	uint32_t arr = 0;
	uint32_t pulses = 0;
	if (laser->pulse_duration <= 0 || laser->pulse_duration > 1001)
	{
		laser->is_active = false;
		return;
	}

	// Компенсируем аппаратный дэдтайм на диапазоне мкс
	// В случае мс из установленной длины импульса вычитается длительность дэдтайм
	if (laser->time_range_config->type == RANGE_US)
	{
		pulses = laser->pulse_duration * laser->time_range_config->ticks_per_unit +
				(uint32_t) (DEADTIME_TICKS / laser->time_range_config->ticks_per_unit);
	}
	else
	{
		pulses = laser->pulse_duration * laser->time_range_config->ticks_per_unit;
	}

	if (pulses <= 1000)
	{
		arr = 1010;
	}
	else
	{
		arr = 10100; // из рассчета, что длительность не превышает 1000 единиц
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
