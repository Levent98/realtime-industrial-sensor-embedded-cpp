#include "SystemClock.hpp"

extern "C" {
#include "stm32f410rx.h"
#include "system_stm32f4xx.h"
}

SystemClock::SystemClock()
    : coreFrequencyHz_(16000000U)   // reset sonrasi güvenli baslangiç: HSI
{
}

bool SystemClock::enableHse()
{
    uint32_t timeout = kClockTimeout;

    RCC->CR |= RCC_CR_HSEON;

    while (((RCC->CR & RCC_CR_HSERDY) == 0U) && (timeout > 0U)) {
        --timeout;
    }

    return (timeout > 0U);
}

void SystemClock::configureFlashLatency()
{
    /* 24 MHz için 0WS çogu durumda yeterli olabilir ama senin C kodunda 1WS seçilmis.
       Ayni davranisi koruyoruz. */
    FLASH->ACR = FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN   |
                 FLASH_ACR_DCEN   |
                 FLASH_ACR_LATENCY_1WS;
}

void SystemClock::configureBusPrescalers()
{
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |=  (RCC_CFGR_HPRE_DIV1 |
                   RCC_CFGR_PPRE1_DIV1 |
                   RCC_CFGR_PPRE2_DIV1);
}

bool SystemClock::configurePllFromHse24MHz()
{
    uint32_t timeout = kClockTimeout;

    /* PLL kapat */
    RCC->CR &= ~RCC_CR_PLLON;
    while (((RCC->CR & RCC_CR_PLLRDY) != 0U) && (timeout > 0U)) {
        --timeout;
    }

    if (timeout == 0U) {
        return false;
    }

    /* PLL_M = 8
       PLL_N = 192
       PLL_P = 8  -> encoded as 3
       PLL_Q = 4
       Source = HSE

       Formül:
       VCO_in  = 8 MHz / 8   = 1 MHz
       VCO_out = 1 MHz * 192 = 192 MHz
       PLLCLK  = 192 / 8     = 24 MHz
    */
    RCC->PLLCFGR =
        (8U   << 0U)  |
        (192U << 6U)  |
        (3U   << 16U) |
        RCC_PLLCFGR_PLLSRC_HSE |
        (4U   << 24U);

    timeout = kClockTimeout;

    RCC->CR |= RCC_CR_PLLON;

    while (((RCC->CR & RCC_CR_PLLRDY) == 0U) && (timeout > 0U)) {
        --timeout;
    }

    return (timeout > 0U);
}

void SystemClock::switchSystemClockToPll()
{
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }
}

void SystemClock::configure()
{
    /* Clock interrupt flag temizleme / disable */
    RCC->CIR = 0x00000000U;

    /* Varsayilan: HSI ile kaldigini varsay */
    coreFrequencyHz_ = 16000000U;

    if (!enableHse()) {
        SystemCoreClockUpdate();
        coreFrequencyHz_ = SystemCoreClock;
        return;
    }

    configureFlashLatency();
    configureBusPrescalers();

    if (!configurePllFromHse24MHz()) {
        SystemCoreClockUpdate();
        coreFrequencyHz_ = SystemCoreClock;
        return;
    }

    switchSystemClockToPll();

    SystemCoreClockUpdate();
    coreFrequencyHz_ = SystemCoreClock;
}

uint32_t SystemClock::coreFrequencyHz() const
{
    return coreFrequencyHz_;
}