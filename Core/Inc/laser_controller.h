/* laser_experiment.h */
#ifndef LASER_EXPERIMENT_H
#define LASER_EXPERIMENT_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"


typedef enum {
	RANGE_US,
	RANGE_MS,
	RANGE_SEC,
	RANGE_CONTINUOUS,
    RANGE_COUNT
} time_range_t;


typedef struct {
    uint32_t psc;                // Значение для регистра PSC
    uint32_t ticks_per_unit;     // Коэффициент: сколько тиков в 1 единице ввода (1мкс, 1мс, 1с)
    const char* label;           // Название для меню
} time_range_config_t;


typedef enum {
	SHORT_PULSE,
	LONG_PULSE,
	CONTINUOUS_PULSE
} laser_pulse_type_t;


typedef struct
{
	laser_pulse_type_t pulse_type;

	uint32_t deadtime;
	uint32_t pulse_duration;
	bool is_active;

	time_range_config_t* time_range_config;

	bool switch_used;
	bool switch_state;

	uint32_t last_update_time;
	uint32_t elapsed_time;
	uint32_t max_pulse_time;
	bool experiment_aborted;
} laser_t;


void init_laser_controller(laser_t* laser);
void laser_generate_short_pulse(laser_t* laser);
void laser_generate_long_pulse(laser_t* laser);
void laser_generate_continuous_pulse(laser_t* laser);

#endif /* LASER_EXPERIMENT_H */
