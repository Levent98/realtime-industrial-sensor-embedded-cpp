#ifndef DELAY_TIMER_HPP
#define DELAY_TIMER_HPP

#include <stdint.h>

class DelayTimer
{
public:
    DelayTimer();

    void initialize();
    void delayUs(uint16_t us) const;
    void delayMs(uint16_t ms) const;
    uint32_t tickMs() const;

    static void onSysTickInterrupt();

private:
    static volatile uint32_t msTicks_;
};

#endif