#ifndef __COMMON_H__
#define __COMMON_H__

// Константы
#define HEARTBEAT_DELAY					1000 	// Частота мигания светодиода сердцебиения
#define ERROR_BLINK_DELAY				100 	// Частота мигания светодиода при ошибке

#define DISPLAY_MENU_UPDATE_TIME		100
#define DISPLAY_EXPERIMENT_UPDATE_TIME  1000
#define MIN_DISPLAY_PULSE_TIME			1000 	// Время, после которого приложение может вернуться в состояние меню. Нужно, чтобы избежать мерцаний дисплея
#define ABORT_COOLDOWN_TIME				2000 	// Время, после которого можно отключать импульс нажатием кнопки
#define MAX_CONTINIOUS_PULSE_TIME		10000 	// Максимальная длительность эксперимента 10 минут TODO: исправить на 600000

#define DEADTIME_TICKS					10		// Дэдтайм: число тиков относительно частоты тактирования APB2. При 100МГц 1 тик = 10нс
#define MIN_SHORT_PULSE_DURATION		1
#define MAX_SHORT_PULSE_DURATION		1000
#define MIN_LONG_PULSE_DURATION			1
#define MAX_LONG_PULSE_DURATION			100

#define START_PULSE_DURATION			500 	// мкс или мс

#define MIN_COOLER_POWER				0
#define MAX_COOLER_POWER				100
#define START_COOLER_POWER				30

#define MIN_PELTIER_POWER				0
#define MAX_PELTIER_POWER				100
#define START_PELTIER_POWER				0

#define MAX_UNITS_COUNT 2

#endif // __COMMON_H__
