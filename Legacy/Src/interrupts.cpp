#include "UartDriver.hpp"
#include "DelayTimer.hpp"



extern "C" void SysTick_Handler(void)
{
    DelayTimer::onSysTickInterrupt();
}