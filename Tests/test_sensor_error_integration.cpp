#include "SensorService.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

class FakeI2cBus : public I2cBus {
public:
    void initialize() override {}
    I2cStatus write(std::uint8_t, const std::uint8_t*, std::size_t) override { return I2cStatus::Ok; }
    I2cStatus read(std::uint8_t, std::uint8_t*, std::size_t) override { return I2cStatus::Ok; }
    I2cStatus writeRead(std::uint8_t, const std::uint8_t*, std::size_t, std::uint8_t*, std::size_t) override { return I2cStatus::Ok; }
    bool recoverBus() override { return true; }
};

class FailingSht3xSensor : public Sht3xSensor {
public:
    explicit FailingSht3xSensor(I2cBus& bus) : Sht3xSensor(bus) {}

    I2cStatus triggerMeasurement() override
    {
        ++triggerCount;
        return I2cStatus::Ok;
    }

    I2cStatus readMeasurement(std::uint16_t&, std::uint16_t&) override
    {
        ++readCount;
        return I2cStatus::Nack;
    }

    int triggerCount = 0;
    int readCount = 0;
};

int main()
{
    // Production SensorService + real ErrorManager integration test.
    FakeI2cBus bus;
    FailingSht3xSensor sensor(bus);
    UartDriver uart;
    ErrorManager errors;
    ModbusService modbus(uart, errors);
    LcdDisplay lcd;
    PwmOutput pwm;
    SensorService service(sensor, modbus, lcd, pwm, errors);

    service.initialize();

    // TC-SENSOR-ERR-001 / BT-SW-SENSOR-008
    // Real SensorService reports one sensor read failure to ErrorManager.
    service.process(0U);
    service.process(20U);
    assert(sensor.readCount == 1);
    assert(errors.sensorReadFailCount() == 1U);
    assert(!errors.recoveryRequested());

    // TC-SENSOR-ERR-002 / BT-SW-SENSOR-008
    // Repeated production SensorService read failures trigger ErrorManager recovery request.
    service.process(120U);
    service.process(140U);
    service.process(240U);
    service.process(260U);

    assert(sensor.readCount == 3);
    assert(errors.sensorReadFailCount() == 3U);
    assert(errors.recoveryRequested());

    return 0;
}
