#include "Sht3xSensor.hpp"

namespace {
    uint8_t crc8(const uint8_t* data, uint8_t len)
    {
        uint8_t crc = 0xFFU;

        for (uint8_t i = 0U; i < len; ++i) {
            crc ^= data[i];

            for (uint8_t bit = 0U; bit < 8U; ++bit) {
                if ((crc & 0x80U) != 0U) {
                    crc = static_cast<uint8_t>((crc << 1U) ^ 0x31U);
                } else {
                    crc = static_cast<uint8_t>(crc << 1U);
                }
            }
        }

        return crc;
    }
}

Sht3xSensor::Sht3xSensor(I2cBus& bus)
    : bus_(bus)
{
}

I2cStatus Sht3xSensor::triggerMeasurement()
{
    const uint8_t command[2] = { 0x24U, 0x0BU };
    return bus_.write(Address, command, 2U);
}

I2cStatus Sht3xSensor::readMeasurement(uint16_t& rawTemperature, uint16_t& rawHumidity)
{
    uint8_t frame[6] = {0U};

    const I2cStatus status = bus_.read(Address, frame, 6U);
    if (status != I2cStatus::Ok) {
        return status;
    }

    if ((crc8(&frame[0], 2U) != frame[2]) ||
        (crc8(&frame[3], 2U) != frame[5])) {
        return I2cStatus::BusError;
    }

    rawTemperature = static_cast<uint16_t>((static_cast<uint16_t>(frame[0]) << 8U) | frame[1]);
    rawHumidity    = static_cast<uint16_t>((static_cast<uint16_t>(frame[3]) << 8U) | frame[4]);

    return I2cStatus::Ok;
}

int32_t Sht3xSensor::temperatureX100(uint16_t raw) const
{
    return ((17500L * static_cast<int32_t>(raw)) / 65535L) - 4500L;
}

int32_t Sht3xSensor::humidityX100(uint16_t raw) const
{
    return static_cast<int32_t>((10000L * static_cast<uint32_t>(raw)) / 65535UL);
}