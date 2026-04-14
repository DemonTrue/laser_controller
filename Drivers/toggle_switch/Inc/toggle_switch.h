/**
 * @file toggle_switch.h
 * @brief Модуль обработки переключателей (кнопки с фиксацией и тумблеры)
 */

#ifndef TOGGLE_SWITCH_H
#define TOGGLE_SWITCH_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Конфигурация по умолчанию */
#ifndef TOGGLE_SWITCH_ACTIVE_STATE
    #define TOGGLE_SWITCH_ACTIVE_STATE 0		/* 1 - активный высокий уровень, 0 - активный низкий */
#endif

#ifndef TOGGLE_SWITCH_DEBOUNCE_TIME
    #define TOGGLE_SWITCH_DEBOUNCE_TIME 50		/* мс */
#endif

#ifndef TOGGLE_SWITCH_CHECK_TIME
    #define TOGGLE_SWITCH_CHECK_TIME 10			/* мс — частота опроса в polling-режиме */
#endif


typedef struct toggle_switch_s toggle_switch_t;

/**
 * @brief Состояния переключателя в конечном автомате
 */
typedef enum {
    TOGGLE_SWITCH_STATE_OFF,    /* Выключен */
    TOGGLE_SWITCH_STATE_ON,     /* Включен */
    TOGGLE_SWITCH_STATE_COUNT
} toggle_switch_state_t;


/**
 * @brief События переключателя
 */
typedef enum {
    TOGGLE_SWITCH_EVENT_NONE,       /* Нет событий */
    TOGGLE_SWITCH_EVENT_CHANGED,    /* Состояние изменилось (вкл/выкл) */
    TOGGLE_SWITCH_EVENT_COUNT
} toggle_switch_event_t;


/**
 * @brief Идентификаторы переключателей
 */
typedef enum {
    TOGGLE_SWITCH_ID_MAIN,
    TOGGLE_SWITCH_ID_COUNT
} toggle_switch_id_t;


/**
 * @brief Режим работы
 */
typedef enum {
    TOGGLE_SWITCH_MODE_POLLING,     /* Опрос в цикле */
    TOGGLE_SWITCH_MODE_INTERRUPT    /* Прерывания */
} toggle_switch_mode_t;


/**
 * @brief Структура данных антидребезга
 */
typedef struct {
    uint8_t raw_state;          /* Текущее "сырое" состояние пина */
    uint8_t stable_state;       /* Отфильтрованное стабильное состояние */
    uint32_t last_change_time;  /* Время последнего изменения сырого состояния */
    uint32_t debounce_time;       /* Время задержки антидребезга */
} toggle_switch_debounce_t;


/**
 * @brief Основная структура переключателя
 */
struct toggle_switch_s {
    GPIO_TypeDef* port;                 /* Порт GPIO */
    uint16_t pin;                       /* Пин GPIO */

    toggle_switch_id_t id;              /* ID переключателя */
    toggle_switch_mode_t mode;          /* Режим работы */
    uint8_t active_level;               /* Активный уровень (0 или 1) */

    toggle_switch_state_t state;        /* Текущее состояние FSM */
    toggle_switch_event_t event;        /* Флаг события */

    toggle_switch_debounce_t debounce;  /* Данные антидребезга */

    uint32_t poll_interval_ms;          /* Интервал опроса для POLLING */
    uint32_t last_poll_time;            /* Время последнего опроса */

    void (*callback)(void* context, toggle_switch_t* toggle_switch); /* Callback при изменении */
    void* context;                      /* Контекст для callback */

    volatile uint8_t irq_flag;          /* Флаг прерывания */
};


/**
 * @brief Инициализация структуры переключателя
 */
void toggle_switch_init(toggle_switch_t* toggle_switch,
		GPIO_TypeDef* port,
		uint16_t pin,
		toggle_switch_id_t id,
		toggle_switch_mode_t mode,
		void (*callback)(void*, toggle_switch_t*),
		void* context);


/**
 * @brief Чтение стабильного состояния с антидребезгом
 * @return 1 если активно, 0 если нет
 */
uint8_t toggle_switch_get_state(toggle_switch_t* toggle_switch);


/**
 * @brief Обновление логики (FSM + Debounce). Вызывать периодически.
 */
void toggle_switch_update(toggle_switch_t* toggle_switch);


/**
 * @brief Обработчик событий. Вызывает callback при изменении состояния.
 */
void handle_toggle_switch_events(toggle_switch_t* toggle_switch);


/**
 * @brief Обработчик прерывания (вызывать из HAL_GPIO_EXTI_Callback)
 */
void toggle_switch_irq_handler(toggle_switch_t* toggle_switch);

#endif /* TOGGLE_SWITCH_H */
