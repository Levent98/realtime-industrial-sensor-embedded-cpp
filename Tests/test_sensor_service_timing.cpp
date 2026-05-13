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

class FakeSht3xSensor : public Sht3xSensor {
public:
    explicit FakeSht3xSensor(I2cBus& bus) : Sht3xSensor(bus) {}

    I2cStatus triggerMeasurement() override
    {
        ++triggerCount;
        return triggerResult;
    }

    I2cStatus readMeasurement(std::uint16_t& rawTemperature, std::uint16_t& rawHumidity) override
    {
        ++readCount;
        rawTemperature = rawTemperatureValue;
        rawHumidity = rawHumidityValue;
        return readResult;
    }

    int triggerCount = 0;
    int readCount = 0;
    I2cStatus triggerResult = I2cStatus::Ok;
    I2cStatus readResult = I2cStatus::Ok;
    std::uint16_t rawTemperatureValue = 0x6666U;
    std::uint16_t rawHumidityValue = 0x8000U;
};

int main()
{
    // Production SensorService timing test with fake sensor and real ErrorManager/ModbusService.
    FakeI2cBus bus;
    FakeSht3xSensor sensor(bus);
    UartDriver uart;
    ErrorManager errors;
    ModbusService modbus(uart, errors);
    LcdDisplay lcd;
    PwmOutput pwm;
    SensorService service(sensor, modbus, lcd, pwm, errors);

    service.initialize();

    // TC-SENSOR-SVC-001 / BT-SW-SENSOR-007
    // Real SensorService calls trigger before attempting any read.
    service.process(0U);
    assert(sensor.triggerCount == 1);
    assert(sensor.readCount == 0);

    // TC-SENSOR-SVC-002 / BT-SW-SENSOR-007
    // Real SensorService does not read before required measurement delay.
    service.process(5U);
    assert(sensor.readCount == 0);

    // TC-SENSOR-SVC-003 / BT-SW-SENSOR-007
    // Real SensorService reads after required measurement delay and updates Modbus input registers.
    service.process(20U);
    assert(sensor.readCount == 1);
    assert(modbus.inputRegister(REG_TEMP_VALUE) > 0U);
    assert(modbus.inputRegister(REG_GAS_CONCT_VALUE) > 0U);

    return 0;
}
