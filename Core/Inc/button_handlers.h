/**
 * @file button_handlers.h
 * @brief Обработчики событий кнопок для управления состоянием приложения.
 *
 * Содержит callback-функции для обработки коротких и длинных нажатий
 * всех кнопок интерфейса. Функции реализуют логику переходов между
 * состояниями приложения и взаимодействия с меню.
 *
 * Особенности:
 * - Единая точка обработки всех пользовательских действий
 * - Контекстно-зависимое поведение кнопок в разных состояниях приложения
 * - Поддержка навигации по меню и управления экспериментом
 * - Обработка как коротких, так и длинных нажатий
 */

#ifndef BUTTON_HANDLERS_H
#define BUTTON_HANDLERS_H

#include "button.h"
#include "toggle_switch.h"

/**
 * @brief Обработчик короткого нажатия основной кнопки
 * @param context Указатель на менеджер состояний приложения
 * @param id Идентификатор кнопки (должен быть BUTTON_ID_MAIN)
 */
void on_main_button_short_press(void* context, button_id_t id);


/**
 * @brief Обработчик длинного нажатия основной кнопки
 * @param context Указатель на менеджер состояний приложения
 * @param id Идентификатор кнопки (должен быть BUTTON_ID_MAIN)
 */
void on_main_button_long_press(void* context, button_id_t id);


/**
 * @brief Обработчик короткого нажатия кнопки энкодера
 * @param context Указатель на менеджер состояний приложения
 * @param id Идентификатор кнопки
 */
void on_encoder_button_short_press(void* context, button_id_t id);

/**
 * @brief Обработчик длинного нажатия кнопки энкодера
 * @param context Указатель на менеджер состояний приложения
 * @param id Идентификатор кнопки
 */
void on_encoder_button_long_press(void* context, button_id_t id);


/**
 * @brief Обработчик переключателя
 * @param context Указатель на менеджер состояний приложения
 * @param id Идентификатор переключателя
 */
void on_toggle_switch_state_change(void* context, toggle_switch_t* toggle_switch);


#endif /* BUTTON_HANDLERS_H */
