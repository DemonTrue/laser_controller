/* encoder.c */
#include "encoder.h"
#include <stdlib.h>


void encoder_init(encoder_t* enc) {
    if (!enc) return;

//    enc->min_limit = 0;
//    enc->max_limit = __HAL_TIM_GET_AUTORELOAD(enc->htim);

    if (enc->step < 0) enc->step = 1;

    enc->remainder = 0; // Сброс накопителя остатка
    enc->last_direction = ENC_DIR_NONE;
    enc->prev_raw_count = 0;

    // Начальное чтение
    enc->position_raw = (int32_t)__HAL_TIM_GET_COUNTER(enc->htim);
    enc->position_norm = enc->position_raw;
}


void encoder_update(encoder_t* enc) {
    if (!enc || !enc->htim) return;

    int32_t current_raw = (int32_t)__HAL_TIM_GET_COUNTER(enc->htim);

    if (current_raw != enc->prev_raw_count) {
        uint32_t arr = __HAL_TIM_GET_AUTORELOAD(enc->htim);
        int32_t diff = current_raw - enc->prev_raw_count;

        // Коррекция переполнения таймера
        if (diff > (int32_t)(arr / 2)) {
            diff -= (arr + 1);
        } else if (diff < -(int32_t)(arr / 2)) {
            diff += (arr + 1);
        }

        // Определение направления по сырой дельте
        if (diff > 0)
        {
            enc->last_direction = ENC_DIR_CW;
        }
        else if (diff < 0)
        {
            enc->last_direction = ENC_DIR_CCW;
        }
        else
        {
        	enc->last_direction = ENC_DIR_NONE;
        }

        enc->prev_raw_count = current_raw;

        // логика масштабирования
        // Добавляем текущую дельту к накопленному остатку
        int32_t total_diff = enc->remainder + diff;

        // Вычисляем изменение в нормализованных единицах
        int32_t norm_diff = total_diff / enc->step;

        // Сохраняем то, что не вошло в целый шаг (остаток от деления)
        enc->remainder = total_diff % enc->step;

        // Если изменений в нормализованных единицах нет, выходим
        if (norm_diff == 0)
        {
            return;
        }

        // Вычисляем новую позицию
        int32_t new_pos = enc->position_norm + norm_diff;

        // логика ограничения
        if (enc->limit)
        {
			if (new_pos < enc->min_limit)
			{
				new_pos = enc->min_limit;

				// Синхронизация счетчика при упоре в минимум
				if (diff < 0)
				{
					// Сбрасываем остаток, чтобы не было задержки при обратном вращении
					enc->remainder = 0;
				}
			}
			else if (new_pos > enc->max_limit)
			{
				new_pos = enc->max_limit;

				if (diff > 0)
				{
					enc->remainder = 0;
				}
			}
        }

        // Применяем изменения
        enc->position_norm = new_pos;
        enc->position_raw = new_pos;
    }
}


void encoder_reset(encoder_t* enc) {
    if (!enc || !enc->htim) return;
    // Сбрасываем в 0 нормализованных единиц -> 0 сырых тиков (если step=1) или 0 (если база 0)
    __HAL_TIM_SET_COUNTER(enc->htim, 0);
    enc->prev_raw_count = 0;
    enc->position_raw = 0;
    enc->position_norm = 0;
    enc->remainder = 0;
    enc->last_direction = ENC_DIR_NONE;
}


void encoder_set_position(encoder_t* enc, int32_t value)
{
    if (!enc || !enc->htim) return;
    // Устанавливаем нормализованное значение
    enc->position_norm = value;
    // Сырое значение устанавливаем пропорционально
    int32_t raw_val = value * enc->step;
    __HAL_TIM_SET_COUNTER(enc->htim, (uint32_t)raw_val);
    enc->prev_raw_count = raw_val;
    enc->position_raw = raw_val;
    enc->remainder = 0;
}


int32_t encoder_get_position(encoder_t* enc)
{
    if (!enc) return 0;
    return enc->position_norm;
}


encoder_direction_t encoder_get_direction(encoder_t* enc)
{
    if (!enc) return ENC_DIR_NONE;
    return enc->last_direction;
}
