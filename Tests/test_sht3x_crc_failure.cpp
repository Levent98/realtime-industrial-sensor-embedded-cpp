#include "Sht3xSensor.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

class FakeI2cBus : public I2cBus {
public:
    std::uint8_t payload[6] = {0U};
    I2cStatus readResult = I2cStatus::Ok;

    void initialize() override {}

    I2cStatus write(std::uint8_t, const std::uint8_t*, std::size_t) override
    {
        return I2cStatus::Ok;
    }

    I2cStatus read(std::uint8_t, std::uint8_t* data, std::size_t size) override
    {
        if (readResult != I2cStatus::Ok) {
            return readResult;
        }

        for (std::size_t i = 0U; i < size; ++i) {
            data[i] = payload[i];
        }

        return I2cStatus::Ok;
    }

    I2cStatus writeRead(std::uint8_t,
                        const std::uint8_t*,
                        std::size_t,
                        std::uint8_t*,
                        std::size_t) override
    {
        return I2cStatus::InvalidArgument;
    }

    bool recoverBus() override
    {
        return false;
    }
};

int main()
{
    // TC-SHT3X-CRC-001 / BT-SW-SENSOR-005
    // Real Sht3xSensor::readMeasurement rejects payloads with bad SHT3x CRC bytes.
    FakeI2cBus bus;
    bus.payload[0] = 0x66U;
    bus.payload[1] = 0x66U;
    bus.payload[2] = 0x00U; // intentionally wrong CRC
    bus.payload[3] = 0x80U;
    bus.payload[4] = 0x00U;
    bus.payload[5] = 0x00U; // intentionally wrong CRC

    Sht3xSensor sensor(bus);
    std::uint16_t rawTemperature = 0x1234U;
    std::uint16_t rawHumidity = 0x5678U;

    assert(sensor.readMeasurement(rawTemperature, rawHumidity) == I2cStatus::BusError);
    assert(rawTemperature == 0x1234U);
    assert(rawHumidity == 0x5678U);

    return 0;
}
