#include "one_pulse_mode_TIM1.h"
// TODO: Избавиться от циклической зависимости...

// Переменные для управления длительным импульсом
volatile uint32_t pulse_tick_count = 0;
volatile uint32_t pulse_target_ticks = 0;
volatile uint8_t is_pulse_active = 0;

extern time_range_config_t time_range_table[RANGE_COUNT];

void TIM1_OnePulse_Init(uint32_t pulse_width, uint32_t arr, uint32_t psc, uint16_t deadtime)
{
   // Включаем тактирование
   RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
   RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

   // Небольшая задержка для стабилизации тактирования
   __DSB();

   // Полный сброс таймера
   TIM1->CR1 = 0;
   TIM1->CCER = 0;
   TIM1->BDTR = 0;
   TIM1->CR2 = 0;
   TIM1->CCMR1 = 0;

   // ========== 1. Настраиваем GPIO как выходы и фиксируем уровни ==========
   // Сначала сбрасываем в дефолт
   GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8);
   GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER13);

   // Настраиваем как выходы
   GPIOA->MODER |= GPIO_MODER_MODER8_0;  // Output
   GPIOB->MODER |= GPIO_MODER_MODER13_0; // Output

   // Устанавливаем безопасные уровни
   GPIOA->BSRR = GPIO_BSRR_BR_8;     // CH1 = LOW
   GPIOB->BSRR = GPIO_BSRR_BS_13;    // CH1N = HIGH

   // Небольшая задержка для установки уровней
   __NOP(); __NOP(); __NOP();

   // ========== 2. Настраиваем таймер в режим FORCE INACTIVE ==========
   // Базовая настройка времени
   TIM1->PSC = psc - 1;
   TIM1->ARR = arr - 1;
   TIM1->CCR1 = arr - pulse_width;

   // FORCE INACTIVE mode (OC1M = 000)
   TIM1->CCMR1 = 0;  // OC1M = 000 (Force inactive)
   TIM1->CCMR1 |= TIM_CCMR1_OC1PE;  // Preload enable

   // Настройка выходов - включаем каналы
   TIM1->CCER = 0;
   TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE;
   TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);  // Active high

   // Настройка idle state (на всякий случай)
   TIM1->CR2 &= ~TIM_CR2_OIS1;   // OIS1 = 0
   TIM1->CR2 |= TIM_CR2_OIS1N;   // OIS1N = 1

   // Deadtime
   TIM1->BDTR &= ~0xFF;
   TIM1->BDTR |= (deadtime & 0xFF);
   TIM1->BDTR |= TIM_BDTR_AOE;
   TIM1->BDTR |= TIM_BDTR_OSSI;  // Включаем OSSI для надежности

   // Загружаем настройки
   TIM1->EGR |= TIM_EGR_UG;

   // Ждем завершения загрузки
   TIM1->SR &= ~TIM_SR_UIF;
   TIM1->EGR |= TIM_EGR_UG;
   while(!(TIM1->SR & TIM_SR_UIF));
   TIM1->SR &= ~TIM_SR_UIF;

   // Включаем MOE и запускаем таймер в FORCE INACTIVE режиме
   TIM1->BDTR |= TIM_BDTR_MOE;
   TIM1->CR1 |= TIM_CR1_CEN;

   // Даем время установиться выходным сигналам
   for(volatile int i = 0; i < 50; i++);

   // ========== 3. Переключаем GPIO в альтернативный режим ==========
   // Критический момент: переключаем оба пина максимально быстро,
   // чтобы минимизировать разрыв во времени между ними

   // Сначала настраиваем AFR (это не влияет на состояние пина, пока MODE не переключен)
   GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL8) | (1 << 0);
   GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL13) | (1 << (4 * (13 - 8)));

   // используем барьер памяти для гарантии порядка операций
   __DSB();

   // Переключаем MODE атомарно - сначала оба пина в INPUT (безопасное состояние)
   GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8);  // Input mode
   GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER13); // Input mode

   // Сразу же переключаем в AF mode (минимальный разрыв)
   GPIOA->MODER |= GPIO_MODER_MODER8_1;
   GPIOB->MODER |= GPIO_MODER_MODER13_1;

   __DSB();

   // Даем время переключиться
   for(volatile int i = 0; i < 10; i++);

   // ========== 4. Критический момент: переключение с FORCE на PWM ==========
   // Чтобы избежать глитча, делаем это в правильной последовательности

   // Останавливаем счетчик (но MOE оставляем)
   TIM1->CR1 &= ~TIM_CR1_CEN;
   while(TIM1->CR1 & TIM_CR1_CEN);

   // Сбрасываем счетчик
   TIM1->CNT = 0;

   // Меняем режим с FORCE на PWM
   TIM1->CCMR1 = 0;
   TIM1->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0); // PWM mode 2
   TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

   // Загружаем новые настройки
   TIM1->SR &= ~TIM_SR_UIF;
   TIM1->EGR |= TIM_EGR_UG;
   while(!(TIM1->SR & TIM_SR_UIF));
   TIM1->SR &= ~TIM_SR_UIF;

   // Даем время стабилизироваться
   for(volatile int i = 0; i < 10; i++);
}


// Функция для генерации одного импульса
void TIM1_generate_short_pulse(uint32_t pulse_width, uint32_t arr, uint32_t psc)
{
   // Останавливаем таймер
   TIM1->CR1 &= ~TIM_CR1_CEN;
   while(TIM1->CR1 & TIM_CR1_CEN);

   // Обновляем параметры
   TIM1->PSC = psc - 1;
   TIM1->ARR = arr - 1;
   TIM1->CCR1 = arr - pulse_width;

   // Сбрасываем счетчик
   TIM1->CNT = 0;

   // Генерируем событие обновления
   TIM1->SR &= ~TIM_SR_UIF;
   TIM1->EGR |= TIM_EGR_UG;
   while(!(TIM1->SR & TIM_SR_UIF));
   TIM1->SR &= ~TIM_SR_UIF;

   // Включаем одноимпульсный режим
   TIM1->CR1 |= TIM_CR1_OPM;

   // Включение прерываний
   TIM1->DIER |= TIM_DIER_UIE;
   NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0);
   NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

   // Запускаем таймер
   TIM1->CR1 |= TIM_CR1_CEN;
}


void TIM1_generate_long_pulse(uint32_t duration, uint32_t arr, uint32_t psc)
{
    // Полная остановка и сброс
    TIM1->CR1 &= ~TIM_CR1_CEN;
    while(TIM1->CR1 & TIM_CR1_CEN);

    pulse_target_ticks = duration;
    pulse_tick_count = 0;
    is_pulse_active = 1;

    // Настройка параметров
    TIM1->PSC = psc - 1;
    TIM1->ARR = arr - 1;
    TIM1->CNT = 0;
    TIM1->CCR1 = 0;

    TIM1->CCMR1 &= ~TIM_CCMR1_OC1PE;
    TIM1->CR1 &= ~TIM_CR1_OPM;

    // Обновление регистров
    TIM1->SR &= ~TIM_SR_UIF;
    TIM1->EGR |= TIM_EGR_UG;
    while(!(TIM1->SR & TIM_SR_UIF));
    TIM1->SR &= ~TIM_SR_UIF;

    // Включение прерываний
    TIM1->DIER |= TIM_DIER_UIE;
	NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0);
    NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    // Запуск
    TIM1->CR1 |= TIM_CR1_CEN;
}


void TIM1_handle_up_interrupt(laser_t* laser)
{
    // Проверяем, наше ли это прерывание (флаг UIF)
    if (TIM1->SR & TIM_SR_UIF)
    {
        // Сразу сбрасываем флаг, иначе прерывание сработает бесконечно
        TIM1->SR &= ~TIM_SR_UIF;

        switch (laser->pulse_type)
        {
        	case SHORT_PULSE:
				// Выключаем прерывания
				TIM1->DIER &= ~TIM_DIER_UIE;

				laser->is_active = false;

				is_pulse_active = 0;
        		break;

        	case LONG_PULSE:
				if (is_pulse_active)
				{
					pulse_tick_count++;

					if (pulse_tick_count >= pulse_target_ticks || laser->experiment_aborted)
					{
						// Выключаем импульс (PWM Mode 2: Low when CNT <= CCR)
						TIM1->CCR1 = TIM1->ARR;

						is_pulse_active = 0;

						// Выключаем прерывания
						TIM1->DIER &= ~TIM_DIER_UIE;

						// Останавливаем таймер
						TIM1->CR1 &= ~TIM_CR1_CEN;
						while(TIM1->CR1 & TIM_CR1_CEN);

						laser->is_active = false;
						laser->experiment_aborted = false;
					}
				}
				break;

        	case CONTINUOUS_PULSE:
        		if (!laser->switch_state || laser->elapsed_time > laser->max_pulse_time)
        		{
					// Выключаем импульс (PWM Mode 2: Low when CNT <= CCR)
					TIM1->CCR1 = TIM1->ARR;

					// Выключаем прерывания
					TIM1->DIER &= ~TIM_DIER_UIE;
					is_pulse_active = 0;

					// Останавливаем таймер
					TIM1->CR1 &= ~TIM_CR1_CEN;
					while(TIM1->CR1 & TIM_CR1_CEN);

					laser->is_active = false;
					laser->switch_used = false;
					laser->switch_state = false; // Сброс на всякий случай

					//TODO: сделать это элегнатнее? Сохранять прошлое значение типа, например
					laser->pulse_type = SHORT_PULSE;
					laser->time_range_config = &time_range_table[RANGE_US];
        		}
        		break;
        }
    }
}
