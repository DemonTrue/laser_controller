#include "toggle_switch.h"

/**
 * @brief Инициализация
 */
void toggle_switch_init(toggle_switch_t* toggle_switch,
		GPIO_TypeDef* port,
		uint16_t pin,
		toggle_switch_id_t id,
		toggle_switch_mode_t mode,
		void (*callback)(void*, toggle_switch_t*),
		void* context)
{
    if (!toggle_switch) return;

    toggle_switch->port = port;
    toggle_switch->pin = pin;
    toggle_switch->id = id;
    toggle_switch->mode = mode;
    toggle_switch->active_level = TOGGLE_SWITCH_ACTIVE_STATE;
    toggle_switch->callback = callback;
    toggle_switch->context = context;

    toggle_switch->state = TOGGLE_SWITCH_STATE_OFF;
    toggle_switch->event = TOGGLE_SWITCH_EVENT_NONE;
    toggle_switch->irq_flag = 0;

    // Начальное чтение пина
    uint8_t initial_raw = (uint8_t)HAL_GPIO_ReadPin(port, pin);
    // Инвертируем логику сравнения, если активный уровень 0 (pull-up)
    uint8_t is_active = (initial_raw == toggle_switch->active_level) ? 1 : 0;

    toggle_switch->debounce.raw_state = is_active;
    toggle_switch->debounce.stable_state = is_active;
    toggle_switch->debounce.last_change_time = HAL_GetTick();
    toggle_switch->debounce.debounce_time = TOGGLE_SWITCH_DEBOUNCE_TIME;

    toggle_switch->poll_interval_ms = TOGGLE_SWITCH_CHECK_TIME;
    toggle_switch->last_poll_time = HAL_GetTick();
}


/**
 * @brief Внутренняя функция чтения с дебаунсом
 */
static uint8_t toggle_switch_read(toggle_switch_t* toggle_switch)
{
    uint8_t raw_state = HAL_GPIO_ReadPin(toggle_switch->port, toggle_switch->pin);
    uint32_t time_now = HAL_GetTick();

    uint8_t is_active_now = (raw_state == toggle_switch->active_level);

    if (is_active_now != toggle_switch->debounce.stable_state)
    {
        if (time_now - toggle_switch->debounce.last_change_time > toggle_switch->debounce.debounce_time)
        {
            toggle_switch->debounce.stable_state = is_active_now;
            toggle_switch->debounce.last_change_time = time_now;
        }
    }
    else
    {
        toggle_switch->debounce.last_change_time = time_now;
    }

    return toggle_switch->debounce.stable_state;
}


/**
 * @brief Публичный геттер состояния
 */
uint8_t toggle_switch_get_state(toggle_switch_t* toggle_switch)
{
    if (!toggle_switch) return !toggle_switch->active_level;
    return toggle_switch_read(toggle_switch);
}


/**
 * @brief Обновление FSM
 */
void toggle_switch_update(toggle_switch_t* toggle_switch)
{
    if (!toggle_switch) return;

    uint8_t is_on = toggle_switch_read(toggle_switch);

    switch (toggle_switch->state)
    {
        case TOGGLE_SWITCH_STATE_OFF:
            if (is_on)
            {
                toggle_switch->state = TOGGLE_SWITCH_STATE_ON;
                toggle_switch->event = TOGGLE_SWITCH_EVENT_CHANGED;
            }
            break;

        case TOGGLE_SWITCH_STATE_ON:
            if (!is_on)
            {
                toggle_switch->state = TOGGLE_SWITCH_STATE_OFF;
                toggle_switch->event = TOGGLE_SWITCH_EVENT_CHANGED;
            }
            break;

        default:
            toggle_switch->state = TOGGLE_SWITCH_STATE_OFF;
            break;
    }
}


/**
 * @brief Обработка событий
 */
void handle_toggle_switch_events(toggle_switch_t* toggle_switch)
{
    if (!toggle_switch) return;

    uint32_t now = HAL_GetTick();

    // Режим прерываний: выходим, если нет флага
    if (toggle_switch->mode == TOGGLE_SWITCH_MODE_INTERRUPT && !toggle_switch->irq_flag)
    {
        return;
    }

    // Режим опроса: проверяем интервал
    if (toggle_switch->mode == TOGGLE_SWITCH_MODE_POLLING)
    {
        if (now - toggle_switch->last_poll_time < toggle_switch->poll_interval_ms)
        {
            return;
        }
        toggle_switch->last_poll_time = now;
    }

    // Сброс флага прерывания перед обработкой
    toggle_switch->irq_flag = 0;

    // Обновляем FSM
    toggle_switch_update(toggle_switch);

    // Вызов callback, если есть событие
    if (toggle_switch->event == TOGGLE_SWITCH_EVENT_CHANGED && toggle_switch->callback)
    {
        toggle_switch->callback(toggle_switch->context, toggle_switch);
        toggle_switch->event = TOGGLE_SWITCH_EVENT_NONE; // Сброс события
    }
}


/**
 * @brief IRQ Handler
 */
void toggle_switch_irq_handler(toggle_switch_t* toggle_switch)
{
    if (toggle_switch && toggle_switch->mode == TOGGLE_SWITCH_MODE_INTERRUPT)
    {
        toggle_switch->irq_flag = 1;
    }
}
