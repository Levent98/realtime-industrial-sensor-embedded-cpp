#include "PwmOutput.hpp"

extern "C" {
#include "stm32f410rx.h"
}

/* ================= INIT ================= */

PwmOutput::PwmOutput()
{
}

void PwmOutput::initialize()
{
    /* CLOCK */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN;

    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;

    /* PA1 -> TIM5_CH2 */
    GPIOA->MODER &= ~(3U << (1 * 2));
    GPIOA->MODER |=  (2U << (1 * 2));
    GPIOA->AFR[0] |= (2U << (1 * 4));

    /* PB9 -> TIM11_CH1 */
    GPIOB->MODER &= ~(3U << (9 * 2));
    GPIOB->MODER |=  (2U << (9 * 2));
    GPIOB->AFR[1] |= (3U << ((9 - 8) * 4));

    /* TIM5 */
    TIM5->PSC = PWM_PSC;
    TIM5->ARR = PWM_ARR;

    TIM5->CCMR1 |= (6U << TIM_CCMR1_OC2M_Pos);
    TIM5->CCMR1 |= TIM_CCMR1_OC2PE;

    TIM5->CCER |= TIM_CCER_CC2E;
    TIM5->CR1 |= TIM_CR1_ARPE;
    TIM5->EGR  |= TIM_EGR_UG;
    TIM5->CR1 |= TIM_CR1_CEN;

    /* TIM11 */
    TIM11->PSC = PWM_PSC;
    TIM11->ARR = PWM_ARR;

    TIM11->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);
    TIM11->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM11->CCER |= TIM_CCER_CC1E;
    TIM11->CR1 |= TIM_CR1_ARPE;
    TIM11->EGR  |= TIM_EGR_UG;
    TIM11->CR1 |= TIM_CR1_CEN;
}

/* ================= PUBLIC API ================= */

void PwmOutput::setTemperature(int32_t tempX100)
{
    uint32_t current = temperatureToCurrent(tempX100);
    uint16_t duty = currentToPwm(current);
    setTim5(duty);
}

void PwmOutput::setHumidity(uint32_t humX100)
{
    uint32_t current = humidityToCurrent(humX100);
    uint16_t duty = currentToPwm(current);
    setTim11(duty);
}

/* ================= INTERNAL ================= */

void PwmOutput::setTim5(uint16_t duty)
{
    if (duty > 100) duty = 100;
    TIM5->CCR2 = (PWM_ARR + 1U) * duty / 100U;
}

void PwmOutput::setTim11(uint16_t duty)
{
    if (duty > 100) duty = 100;
    TIM11->CCR1 = (PWM_ARR + 1U) * duty / 100U;
}

/* ================= CONVERSIONS ================= */

uint32_t PwmOutput::temperatureToCurrent(int32_t temp)
{
    if (temp <= TEMP_MIN_X100) return CURRENT_MIN;
    if (temp >= TEMP_MAX_X100) return CURRENT_MAX;

    return CURRENT_MIN +
        ((temp - TEMP_MIN_X100) *
        (CURRENT_MAX - CURRENT_MIN)) /
        (TEMP_MAX_X100 - TEMP_MIN_X100);
}

uint32_t PwmOutput::humidityToCurrent(uint32_t hum)
{
    if (hum <= HUM_MIN_X100) return CURRENT_MIN;
    if (hum >= HUM_MAX_X100) return CURRENT_MAX;

    return CURRENT_MIN +
        ((hum - HUM_MIN_X100) *
        (CURRENT_MAX - CURRENT_MIN)) /
        (HUM_MAX_X100 - HUM_MIN_X100);
}

uint16_t PwmOutput::currentToPwm(uint32_t current)
{
    if (current <= CURRENT_MIN) return DUTY_MIN;
    if (current >= CURRENT_MAX) return DUTY_MAX;

    return DUTY_MIN +
        ((current - CURRENT_MIN) *
        (DUTY_MAX - DUTY_MIN)) /
        (CURRENT_MAX - CURRENT_MIN);
}