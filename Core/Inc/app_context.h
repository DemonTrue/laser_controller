/**
 * @file app_context.h
 * @brief Определение структуры контекста приложения.
 *
 * Содержит объявление основной структуры приложения, которая объединяет
 * все модули системы и предоставляет единый интерфейс для доступа к ним.
 *
 *
 * Особенности:
 * - Централизованное хранение указателей на все модули системы
 * - Управление состоянием отображения и интерфейса
 * - Поддержка флагов для управления потоком выполнения
 */

#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <stdint.h>
#include "stdbool.h"
#include "laser_controller.h"
#include "encoder.h"
#include "cooler.h"
#include "peltier.h"
#include "menu.h"


/**
 * @brief Основная структура контекста приложения
 *
 * Объединяет все модули системы в единую структуру для упрощения
 * передачи данных между компонентами и управления состоянием приложения.
 */
typedef struct app_context_s {
	laser_t* laser;
	encoder_t* encoder;
	cooler_t* cooler;
	peltier_t* peltier;
	menu_t* menu;

    uint32_t last_display_update;
    bool laser_is_active;
    bool first_entry_state;

} app_context_t;

#endif /* APP_CONTEXT_H */
