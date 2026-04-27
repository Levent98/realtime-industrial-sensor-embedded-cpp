#include "DelayTimer.hpp"

extern "C" {
#include "delay.h"
#include "stm32f410rx.h"
#include "uart.h"
}

extern "C" {
volatile uint32_t g_ms_ticks = 0U;
volatile uint16_t modbus_counter = 0U;
}

volatile uint32_t DelayTimer::msTicks_ = 0U;
static DelayTimer g_delayTimer;

extern "C" void TimConfig(void)
{
    g_delayTimer.initialize();
}

extern "C" void SysTick_Init(void)
{
    g_delayTimer.initialize();
}

extern "C" void Delay_us(uint16_t us)
{
    g_delayTimer.delayUs(us);
}

extern "C" void Delay_ms(uint16_t ms)
{
    g_delayTimer.delayMs(ms);
}

extern "C" uint32_t GetTick(void)
{
    return g_delayTimer.tickMs();
}

extern "C" uint32_t GetTime_us(void)
{
    uint32_t ms1;
    uint32_t ms2;
    uint16_t us;

    do {
        ms1 = g_ms_ticks;
        us = static_cast<uint16_t>(TIM6->CNT);
        ms2 = g_ms_ticks;
    } while (ms1 != ms2);

    return (ms1 * 1000U) + static_cast<uint32_t>(us % 1000U);
}

DelayTimer::DelayTimer()
{
}

void DelayTimer::initialize()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    (void)RCC->APB1ENR;

    TIM6->CR1 = 0U;
    TIM6->PSC = 24U - 1U;
    TIM6->ARR = 0xFFFFU;
    TIM6->CNT = 0U;
    TIM6->CR1 |= TIM_CR1_CEN;

    SysTick->LOAD = 2400U - 1U;   /* 100 us @ 24 MHz */
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;

    g_ms_ticks = 0U;
    msTicks_ = 0U;
    modbus_counter = 0U;
}

void DelayTimer::delayUs(uint16_t us) const
{
    const uint16_t start = static_cast<uint16_t>(TIM6->CNT);
    while (static_cast<uint16_t>(TIM6->CNT - start) < us) {
    }
}

void DelayTimer::delayMs(uint16_t ms) const
{
    const uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
    }
}

uint32_t DelayTimer::tickMs() const
{
    return g_ms_ticks;
}

void DelayTimer::onSysTickInterrupt()
{
    if (modbus_timer_running != 0U) {
        ++modbus_counter;
        ++modbus_timer_counter;

        if (modbus_counter >= modbus_t35_steps) {
            modbus_timer_running = 0U;
            modbus_counter = 0U;
            modbus_timer_counter = 0U;
            rtu_frame_ready = 1U;
        }
    }

    static uint8_t msDivider = 0U;
    ++msDivider;

    if (msDivider >= 10U) {     /* 50 us x 20 = 1 ms */
        ++g_ms_ticks;
        msTicks_ = g_ms_ticks;
        msDivider = 0U;
    }
}