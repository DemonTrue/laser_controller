/**
 * @file button.h
 * @brief Модуль обработки кнопок с поддержкой коротких и длинных нажатий.
 *
 * Реализует конечный автомат для обработки состояний кнопки с антидребезгом,
 * обнаружением коротких и длинных нажатий, и callback-функциями для обработки событий.
 *
 * Особенности:
 * - Антидребезг контактов с настраиваемым временем
 * - Обнаружение коротких и длинных нажатий
 * - Поддержка нескольких кнопок с индивидуальными настройками
 * - Callback-функции с передачей контекста и идентификатора кнопки
 * - Ограничение частоты опроса для оптимизации производительности
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include "stm32f4xx_hal.h"


// === Настройки кнопки: можно переопределить извне (например, в common.h или main.h) ===
#ifndef BUTTON_ACTIVE_STATE
#define BUTTON_ACTIVE_STATE 0      // PULLUP → нажатие = 0; для PULLDOWN — 1
#endif

#ifndef DEBOUNCE_TIME
#define DEBOUNCE_TIME       30     // мс
#endif

#ifndef LONG_PRESS_TIME
#define LONG_PRESS_TIME     500    // мс
#endif

#ifndef HOLD_REPEAT_TIME
#define HOLD_REPEAT_TIME    1000   // мс
#endif

#ifndef BUTTON_CHECK_TIME
#define BUTTON_CHECK_TIME   10     // мс — частота опроса в polling-режиме
#endif

/**
 * @brief Состояния кнопки в конечном автомате
 */
typedef enum {
    BUTTON_IDLE,    	/* Кнопка отпущена или ожидает */
    BUTTON_PRESSED, 	/* Кнопка нажата (обрабатывается) */
    BUTTON_HELD,		/* Кнопка удерживается (долгое нажатие) */
	BUTTON_HOLD_REPEAT, /* Кнопка удерживается с повторением */
	BUTTON_COUNT
} button_state_t;

/**
 * @brief События кнопки (результат обработки)
 */
typedef enum {
    EVENT_NONE,         /* Событий нет */
    EVENT_SHORT_PRESS,  /* Обнаружено короткое нажатие */
    EVENT_LONG_PRESS,   /* Обнаружено длинное нажатие */
    EVENT_HOLD_REPEAT,  /* Обнаружено повторяющееся нажатие при удержании */
	EVENT_COUNT
} button_event_t;

/**
 * @brief Структура для устранения дребезга контактов
 */
typedef struct {
    uint8_t stable_state;      /* Текущее стабильное состояние кнопки */
    uint32_t last_stable_time; /* Время последнего стабильного изменения состояния */
    uint32_t debounce_time;    /* Время антидребезга в миллисекундах */
} button_debounce_t;

/**
 * @brief Идентификаторы кнопок для различения в обработчиках
 */
typedef enum {
    BUTTON_ID_MAIN, /* Основная кнопка */
    BUTTON_ID_UP,   /* Кнопка "вверх" */
    BUTTON_ID_DOWN  /* Кнопка "вниз" */
} button_id_t;


/**
 * @brief Режим работы кнопки
 */
typedef enum {
    BUTTON_MODE_POLLING,    /* Опрос в основном цикле */
    BUTTON_MODE_INTERRUPT   /* Обработка через прерывания */
} button_mode_t;


/**
 * @brief Основная структура управления кнопкой
 */
typedef struct {
    GPIO_TypeDef* port;        /* Порт GPIO кнопки */
    uint16_t pin;              /* Номер пина кнопки */

    button_id_t id;            /* Идентификатор кнопки */
    button_mode_t mode;        /* Режим работы кнопки */

    uint8_t active_state;      /* Активный уровень (0 для PULLUP, 1 для PULLDOWN) */

    button_state_t state;      /* Текущее состояние конечного автомата */
    button_event_t event;      /* Текущее событие для обработки */
    button_debounce_t debounce;/* Данные для антидребезга */

    uint32_t button_check_time;/* Интервал проверки кнопки в миллисекундах */
    uint32_t last_button_check;/* Время последней проверки кнопки */

    // Для обнаружения долгого нажатия
    uint32_t first_press_time; /* Время первого нажатия кнопки */
    uint32_t long_press_time;  /* Время для определения длинного нажатия */

    // Для режима повторения при удержании
    uint32_t hold_repeat_time; /* Интервал повторения при удержании */
    uint32_t last_repeat_time; /* Время последнего повторения */

    // Callback-функции для обработки событий
    void (*on_short_press)(void* context, button_id_t id); /* Обработчик короткого нажатия */
    void (*on_long_press)(void* context, button_id_t id);  /* Обработчик длинного нажатия */
    void (*on_hold_repeat)(void* context, button_id_t id); /* Обработчик повторяющегося нажатия */

    void* context;  /* Указатель на пользовательские данные (контекст) */

    volatile uint8_t interrupt_flag; /* Флаг прерывания для обработки в основном цикле */
} button_t;

/**
 * @brief Инициализирует структуру кнопки
 * @param button Указатель на структуру кнопки
 * @param port GPIO порт кнопки
 * @param pin Номер пина кнопки
 * @param id Идентификатор кнопки
 * @param short_press Callback для короткого нажатия (может быть NULL)
 * @param long_press Callback для длинного нажатия (может быть NULL)
 * @param context Пользовательский контекст для callback-функций
 */
void button_init(button_t* button,
        GPIO_TypeDef* port,
        uint16_t pin,
        button_id_t id,
	    button_mode_t mode,
        void (*short_press)(void*, button_id_t id),
        void (*long_press)(void*, button_id_t id),
		void (*hold_repeat)(void*, button_id_t id),
        void* context);

/**
 * @brief Читает состояние кнопки с антидребезгом
 * @param button Указатель на инициализированную структуру кнопки
 * @return Стабильное состояние кнопки (1 - нажата, 0 - отпущена)
 */
uint8_t read_button(button_t* button);

/**
 * @brief Обновляет состояние конечного автомата кнопки
 * @param button Указатель на структуру кнопки
 * @note Должен вызываться периодически для обработки состояний
 */
void button_update(button_t* button);

/**
 * @brief Обрабатывает события кнопки и вызывает соответствующие callback-функции
 * @param button Указатель на структуру кнопки
 * @note Автоматически ограничивает частоту опроса и сбрасывает события после обработки
 */
void handle_button_events(button_t* button);

/**
 * @brief Обработчик прерывания кнопки (вызывать из HAL_GPIO_EXTI_Callback)
 * @param button Указатель на структуру кнопки
 * @note Только для кнопок в режиме INTERRUPT
 */
void button_interrupt_handler(button_t* button);

#endif /* BUTTON_H */
