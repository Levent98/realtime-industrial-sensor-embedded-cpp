#include "Sht3xSensor.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

class FakeI2cBus : public I2cBus {
public:
    I2cStatus writeResult = I2cStatus::Nack;

    void initialize() override {}

    I2cStatus write(std::uint8_t, const std::uint8_t*, std::size_t) override
    {
        return writeResult;
    }

    I2cStatus read(std::uint8_t, std::uint8_t*, std::size_t) override
    {
        return I2cStatus::InvalidArgument;
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
    // TC-SHT3X-I2C-001 / BT-SW-SENSOR-006
    // Real Sht3xSensor::triggerMeasurement propagates I2C write failures.
    FakeI2cBus bus;
    Sht3xSensor sensor(bus);

    bus.writeResult = I2cStatus::Nack;
    assert(sensor.triggerMeasurement() == I2cStatus::Nack);

    bus.writeResult = I2cStatus::Timeout;
    assert(sensor.triggerMeasurement() == I2cStatus::Timeout);

    return 0;
}
