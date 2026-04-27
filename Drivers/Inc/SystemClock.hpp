#ifndef SYSTEM_CLOCK_HPP
#define SYSTEM_CLOCK_HPP

#include <stdint.h>

class SystemClock
{
public:
    SystemClock();

    void configure();
    uint32_t coreFrequencyHz() const;

private:
    uint32_t coreFrequencyHz_;

    static constexpr uint32_t kClockTimeout = 0xFFFFU;

    bool enableHse();
    bool configurePllFromHse24MHz();
    void configureFlashLatency();
    void configureBusPrescalers();
    void switchSystemClockToPll();
};

#endif