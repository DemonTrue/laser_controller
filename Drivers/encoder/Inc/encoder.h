/* encoder.h */
#ifndef ENCODER_H
#define ENCODER_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"

// Направление вращения
typedef enum {
    ENC_DIR_NONE = 0,
    ENC_DIR_CW = 1,      // Clockwise
    ENC_DIR_CCW = -1     // Counter-Clockwise
} encoder_direction_t;

typedef struct {
    TIM_HandleTypeDef* htim;      		// Дескриптор таймера (настроенного в режиме Encoder)

    // Конфигурация
    int32_t step;						// Сколько сырых тиков таймера = 1 единице position_norm
    uint32_t pulses_per_revolution; 	// Количество импульсов на полный оборот (для нормализации)
    int32_t min_limit;              	// Минимальное значение счетчика
    int32_t max_limit;              	// Максимальное значение счетчика
    bool limit;               			// Ограничение счетчика

    // Состояние
    int32_t position_raw;           	// Сырое значение из регистра CNT
    int32_t position_norm;          	// Нормализованная позиция (например, 0-360 или 0-100)
    encoder_direction_t last_direction; // Последнее зафиксированное направление

    int32_t prev_raw_count;

    int32_t remainder;					// Накопитель остатка для плавности, если step > 1 и diff маленький
} encoder_t;


/**
 * @brief Инициализирует структуру энкодера
 * @param enc Указатель на структуру энкодера
 * @param htim Указатель на обработчик таймера (должен быть настроен в CubeMX как Encoder Mode)
 * @param ppr Количество импульсов на оборот (если нужно масштабирование)
 */
void encoder_init(encoder_t* enc);


/**
 * @brief Обновляет состояние энкодера (чтение регистра CNT и вычисление направления)
 * @param enc Указатель на структуру энкодера
 * @note Вызывать периодически в основном цикле или по таймеру
 */
void encoder_update(encoder_t* enc);


/**
 * @brief Сбрасывает текущую позицию в ноль
 */
void encoder_reset(encoder_t* enc);


/**
 * @brief Устанавливает конкретное значение позиции
 */
void encoder_set_position(encoder_t* enc, int32_t value);


/**
 * @brief Получает текущую нормализованную позицию
 */
int32_t encoder_get_position(encoder_t* enc);


/**
 * @brief Получает направление последнего движения
 */
encoder_direction_t encoder_get_direction(encoder_t* enc);

#endif /* ENCODER_H */
