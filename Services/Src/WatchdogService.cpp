#include "WatchdogService.hpp"

extern "C" {
#include "stm32f410rx.h"
}

WatchdogService::WatchdogService()
{
}

void WatchdogService::initialize()
{
    IWDG->KR = 0xCCCCU;
    IWDG->KR = 0x5555U;

    IWDG->PR = Prescaler;
    IWDG->RLR = ReloadValue;

    waitForUpdate();

    IWDG->KR = 0xAAAAU;
}

void WatchdogService::kick()
{
    IWDG->KR = 0xAAAAU;
}

void WatchdogService::waitForUpdate() const
{
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) {
    }
}