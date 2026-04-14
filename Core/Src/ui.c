#include "ui.h"
#include "liquidcrystal_i2c.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "menu.h"

void ui_init(void)
{
	HD44780_Init(2);
	HD44780_NoDisplay();
}


void ui_intro(void)
{
    HD44780_Clear();  // Очищаем дисплей
    HD44780_NoCursor();

    HD44780_SetCursor(0,0);
    HD44780_PrintStr("DEVELOPED BY");

    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("OLEG'S TEAM VER 0.1");

    HD44780_Display();  // Включаем дисплей

    HAL_Delay(1000);

    HD44780_Clear();  // Очищаем дисплей
    HD44780_NoCursor();

    HD44780_SetCursor(0,0);
    // TODO: ввести константу в common для рисования версии, а не текстом
    HD44780_PrintStr("VERSION 0.1");

    HAL_Delay(1000);
}


void ui_switch_warning(void)
{
    HD44780_Clear();  // Очищаем дисплей
    HD44780_NoCursor();

    HD44780_SetCursor(0,0);
    HD44780_PrintStr("TURN OFF SWITCH!");

    HD44780_Display();  // Включаем дисплей
}


void ui_clear(void)
{
	HD44780_Clear();
}


void ui_update_menu(int32_t time_value, int32_t cooler_value, int32_t peltier_value,
		uint8_t position, uint8_t digit_state, const char* time_scale)
{
    char buf[20];
    uint8_t cursor_col = 0;
    uint8_t cursor_row = 0;
    bool cursor_active = false;

    // 1. Гарантированно выключаем курсор
    HD44780_NoCursor();

    // Строка 0: TIME
//    snprintf(buf, sizeof(buf), "%sTime:%4ld ms",
//             (position == 0) ? ">" : " ", time_value);

    snprintf(buf, sizeof(buf), "%sTime:%4ld %s",
    		(position == 0) ? ">" : " ",
    		time_value,
			time_scale);

    // Очистка хвоста
    int len = strlen(buf);
    for (int i = len; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    HD44780_SetCursor(0, 0);
    HD44780_PrintStr(buf);

    if (position == 0)
    {
        cursor_row = 0;
        if (digit_state == DIGIT_STATE_UNIT)
        {
            cursor_col = 11;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 9;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 8;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_3)
        {
            cursor_col = 7;
            cursor_active = true;
        }
//        else if (digit_state == DIGIT_STATE_4)
//        {
//            cursor_col = 7;
//            cursor_active = true;
//        }
    }

    // Строка 1: FAN / PELTIER-
    snprintf(buf, sizeof(buf), "%sFAN:%3ld %sPL:%3ld",
             (position == 1) ? ">" : " ", cooler_value,
             (position == 2) ? ">" : " ", peltier_value);

    len = strlen(buf);
    for (int i = len; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    HD44780_SetCursor(0, 1);
    HD44780_PrintStr(buf);

    if (position == 1) // FAN
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 7;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 6;
            cursor_active = true;
        }
    }
    else if (position == 2) // PELTIER
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 15;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 14;
            cursor_active = true;
        }
    }

    // 2. Включаем только если позиция валидна
    if (cursor_active) {
        HD44780_SetCursor(cursor_col, cursor_row);
        HD44780_Cursor();
    }
}

void ui_update_pulse_experiment(int32_t cooler_value, int32_t peltier_value,
        uint8_t position, uint8_t digit_state)
{
	HD44780_Clear();
    char buf[17];
    uint8_t cursor_col = 0;
    uint8_t cursor_row = 0;
    bool cursor_active = false;

    HD44780_NoCursor();

    HD44780_SetCursor(1,0);
    HD44780_PrintStr("PULSE");

    // Строка 1: FAN / PELTIER-
    snprintf(buf, sizeof(buf), "%sFAN:%3ld %sPL:%3ld",
             (position == 1) ? ">" : " ", cooler_value,
             (position == 2) ? ">" : " ", peltier_value);

    int len = strlen(buf);
    for (int i = len; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    HD44780_SetCursor(0, 1);
    HD44780_PrintStr(buf);

    if (position == 1) // FAN
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 7;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 6;
            cursor_active = true;
        }
    }
    else if (position == 2) // PELTIER
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 15;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 14;
            cursor_active = true;
        }
    }

    // 2. Включаем только если позиция валидна
    if (cursor_active) {
        HD44780_SetCursor(cursor_col, cursor_row);
        HD44780_Cursor();
    }
}


void ui_update_continuous_experiment(int32_t cooler_value, int32_t peltier_value,
        uint8_t position, uint8_t digit_state, uint32_t elapsed_time)
{
//	HD44780_Clear();
    char buf[17];
    uint8_t cursor_col = 0;
    uint8_t cursor_row = 0;
    bool cursor_active = false;

    HD44780_NoCursor();

    HD44780_SetCursor(1,0);
//    HD44780_PrintStr("CONTINUOUS");

    uint32_t total_seconds = elapsed_time / 1000;
    uint8_t minutes = total_seconds / 60;
    uint8_t seconds = total_seconds % 60; // Остаток от деления
    snprintf(buf, sizeof(buf), "CONT %02u:%02u", minutes, seconds);

    HD44780_PrintStr(buf);

    // Строка 1: FAN / PELTIER-
    snprintf(buf, sizeof(buf), "%sFAN:%3ld %sPL:%3ld",
             (position == 1) ? ">" : " ", cooler_value,
             (position == 2) ? ">" : " ", peltier_value);

    int len = strlen(buf);
    for (int i = len; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    HD44780_SetCursor(0, 1);
    HD44780_PrintStr(buf);

    if (position == 1) // FAN
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 7;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 6;
            cursor_active = true;
        }
    }
    else if (position == 2) // PELTIER
    {
        cursor_row = 1;
        if (digit_state == DIGIT_STATE_1)
        {
            cursor_col = 15;
            cursor_active = true;
        }
        else if (digit_state == DIGIT_STATE_2)
        {
            cursor_col = 14;
            cursor_active = true;
        }
    }

    // 2. Включаем только если позиция валидна
    if (cursor_active) {
        HD44780_SetCursor(cursor_col, cursor_row);
        HD44780_Cursor();
    }
}
