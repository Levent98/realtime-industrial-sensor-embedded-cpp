#ifndef SHT3XSENSOR_HPP
#define SHT3XSENSOR_HPP

#include <stdint.h>
#include "I2cBus.hpp"

class Sht3xSensor {
public:
    explicit Sht3xSensor(I2cBus& bus);

    I2cStatus triggerMeasurement();
    I2cStatus readMeasurement(uint16_t& rawTemperature, uint16_t& rawHumidity);

    int32_t temperatureX100(uint16_t raw) const;
    int32_t humidityX100(uint16_t raw) const;

private:
    static const uint8_t Address = 0x44U;

    I2cBus& bus_;
};

#endif