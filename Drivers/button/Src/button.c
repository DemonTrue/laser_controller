/**
 * @file button.c
 * @brief Реализация модуля обработки кнопок.
 *
 * Реализует конечный автомат для обработки нажатий кнопок с поддержкой
 * антидребезга, коротких и длинных нажатий через callback-функции.
 */

#include "button.h"

/**
 * @brief Инициализация структуры кнопки с указанными параметрами
 *
 * Устанавливает начальные значения всех полей структуры, включая
 * аппаратные параметры, временные интервалы и callback-функции.
 */
void button_init(button_t* button,
        GPIO_TypeDef* port,
        uint16_t pin,
        button_id_t id,
	    button_mode_t mode,
        void (*short_press)(void*, button_id_t id),
        void (*long_press)(void*, button_id_t id),
		void (*hold_repeat)(void*, button_id_t id),
        void* context)
{
    button->port = port;
    button->pin = pin;

    button->id = id;
    button->mode = mode;

    button->active_state = BUTTON_ACTIVE_STATE;

    button->state = BUTTON_IDLE;
    button->event = EVENT_NONE;

    button->debounce.stable_state = !BUTTON_ACTIVE_STATE;
    button->debounce.last_stable_time = 0;
    button->debounce.debounce_time = DEBOUNCE_TIME;

    button->button_check_time = BUTTON_CHECK_TIME;
    button->last_button_check = 0;

    button->first_press_time = 0;
    button->long_press_time = LONG_PRESS_TIME;

    button->hold_repeat_time = HOLD_REPEAT_TIME;
    button->last_repeat_time = 0;


    // Инициализируем callback-функции и данные
    button->on_short_press = short_press;
    button->on_long_press = long_press;
    button->on_hold_repeat = hold_repeat;
    button->context = context;

    // Инициализация для режима прерываний
    button->interrupt_flag = 0;
}

/**
 * @brief Чтение состояния кнопки с алгоритмом антидребезга
 *
 * Использует временные задержки для фильтрации механических дребезгов контактов.
 * Возвращает стабильное состояние после заданного времени дебаунса.
 *
 * @return Стабильное состояние кнопки (1 - нажата, 0 - отпущена)
 */
uint8_t read_button(button_t* button)
{
    uint8_t raw_state = HAL_GPIO_ReadPin(button->port, button->pin);
    uint32_t time_now = HAL_GetTick();

    uint8_t is_pressed = (raw_state == button->active_state);

    if (is_pressed != button->debounce.stable_state)
    {
        if (time_now - button->debounce.last_stable_time > button->debounce.debounce_time)
        {
            button->debounce.stable_state = is_pressed;
            button->debounce.last_stable_time = time_now;
        }
    }
    else
    {
        button->debounce.last_stable_time = time_now;
    }

    return button->debounce.stable_state;
}

/**
 * @brief Обновление конечного автомата состояний кнопки
 *
 * Обрабатывает переходы между состояниями IDLE, PRESSED и HELD,
 * определяет события коротких и длинных нажатий на основе временных меток.
 */
void button_update(button_t* button) {
    uint32_t time_now = HAL_GetTick();

    uint8_t is_pressed = read_button(button);

    switch (button->state)
    {
        case BUTTON_IDLE:
            if (is_pressed)
            {
                button->first_press_time = time_now;
                button->state = BUTTON_PRESSED;
            }
            break;

        case BUTTON_PRESSED:
            if (!is_pressed)
            {
                // Кнопку отпустили быстро - короткое нажатие
                button->state = BUTTON_IDLE;
                button->event = EVENT_SHORT_PRESS;
            }
            else if ((time_now - button->first_press_time) >= button->long_press_time)
            {
                // Долгое нажатие
                button->state = BUTTON_HELD;
                button->event = EVENT_LONG_PRESS;
            }
            break;

        case BUTTON_HELD:
            if (!is_pressed)
            {
                button->state = BUTTON_IDLE;
            }
            else
            {
                button->state = BUTTON_HOLD_REPEAT;
            }
            break;

        case BUTTON_HOLD_REPEAT:
            if (!is_pressed)
            {
                button->state = BUTTON_IDLE;
            }
            else if ((time_now - button->last_repeat_time) >= button->hold_repeat_time)
            {
                // Генерируем повторяющееся событие
                button->last_repeat_time = time_now;
                button->event = EVENT_HOLD_REPEAT;
            }
            break;

        default:
            // Неожиданное состояние - возвращаем в idle
            button->state = BUTTON_IDLE;
            break;
    }
}

/**
 * @brief Обработка событий кнопки с вызовом callback-функций
 *
 * Ограничивает частоту опроса кнопки, обновляет состояние конечного автомата,
 * обрабатывает накопившиеся события и вызывает соответствующие callback-функции.
 * Автоматически сбрасывает события после обработки.
 */
void handle_button_events(button_t* button)
{
    uint32_t time_now = HAL_GetTick();

    // Для режима прерываний - обрабатываем только если есть флаг
    if (button->mode == BUTTON_MODE_INTERRUPT && !button->interrupt_flag) {
        return;
    }

    // Ограничиваем частоту опроса кнопки (для polling mode)
    if (button->mode == BUTTON_MODE_POLLING &&
        time_now - button->last_button_check < button->button_check_time)
    {
        return;
    }

    button->last_button_check = time_now;

    button_update(button);

    // Обрабатываем события и вызываем callback-функции
    if (button->event == EVENT_SHORT_PRESS && button->on_short_press)
    {
        button->on_short_press(button->context, button->id);
    }
    else if (button->event == EVENT_LONG_PRESS && button->on_long_press)
    {
        button->on_long_press(button->context, button->id);
    }
    else if (button->event == EVENT_HOLD_REPEAT && button->on_hold_repeat)
    {
    	button->on_hold_repeat(button->context, button->id);
    }

    button->event = EVENT_NONE;
}


/**
 * @brief Обработчик прерывания кнопки
 * @param button Указатель на структуру кнопки
 */
void button_interrupt_handler(button_t* button)
{
    if (button->mode == BUTTON_MODE_INTERRUPT) {
        button->interrupt_flag = 1;
    }
}
