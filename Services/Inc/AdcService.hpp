#ifndef ADC_SERVICE_HPP
#define ADC_SERVICE_HPP

#include <stdint.h>
#include "ModbusService.hpp"

class AdcService {
public:
    explicit AdcService(ModbusService& modbus);

    void initialize();
    void process();

    uint16_t rawChannel3() const;
    uint16_t rawChannel4() const;

private:
    static constexpr float VoltageScale = 0.008864468f;

    ModbusService& modbus_;

    static volatile uint16_t adcBuffer_[2];

    void configureGpio();
    void configureDma();
    void configureAdc();
};

#endif