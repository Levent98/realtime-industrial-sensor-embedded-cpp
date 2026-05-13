#ifndef SHT3XSENSOR_HPP
#define SHT3XSENSOR_HPP

#include <stdint.h>
#include "I2cBus.hpp"
#include "Testability.hpp"

class Sht3xSensor {
public:
    explicit Sht3xSensor(I2cBus& bus);
    Sht3xSensor(const Sht3xSensor&) = delete;
    Sht3xSensor& operator=(const Sht3xSensor&) = delete;
    Sht3xSensor(Sht3xSensor&&) = delete;
    Sht3xSensor& operator=(Sht3xSensor&&) = delete;

    TEST_VIRTUAL ~Sht3xSensor() = default;

    TEST_VIRTUAL I2cStatus triggerMeasurement();
    TEST_VIRTUAL I2cStatus readMeasurement(uint16_t& rawTemperature, uint16_t& rawHumidity);

    static int32_t temperatureX100(uint16_t raw);
    static int32_t humidityX100(uint16_t raw);

private:
    static const uint8_t Address = 0x44U;

    I2cBus& bus_;
};

#endif