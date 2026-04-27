#ifndef WATCHDOG_SERVICE_HPP
#define WATCHDOG_SERVICE_HPP

#include <stdint.h>

class WatchdogService
{
public:
    WatchdogService();

    void initialize();
    void kick();

private:
    static constexpr uint16_t ReloadValue = 999U;
    static constexpr uint8_t Prescaler = 0x04U; // /64

    void waitForUpdate() const;
};

#endif