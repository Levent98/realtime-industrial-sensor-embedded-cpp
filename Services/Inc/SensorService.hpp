#ifndef SENSOR_SERVICE_HPP
#define SENSOR_SERVICE_HPP

#include <stdint.h>

#include "Sht3xSensor.hpp"
#include "ModbusService.hpp"
#include "LcdDisplay.hpp"
#include "PwmOutput.hpp"
#include "ErrorManager.hpp"
#include "MedianFilter.hpp"

class SensorService {
public:
    SensorService(Sht3xSensor& sensor,
                  ModbusService& modbus,
                  LcdDisplay& lcd,
                  PwmOutput& pwm,
                  ErrorManager& errors);

    void initialize();
    void process(uint32_t nowMs);

private:
    enum class State : uint8_t {
        Trigger = 0U,
        Read
    };

    static constexpr uint16_t MeasureMediumCommand = 0x240BU;
    static constexpr uint32_t MeasurePeriodMs = 1000U;
    static constexpr uint32_t MeasureDelayMs = 20U;

    Sht3xSensor& sensor_;
    ModbusService& modbus_;
    LcdDisplay& lcd_;
    PwmOutput& pwm_;
    ErrorManager& errors_;

    State state_;
    uint32_t lastSensorTimeMs_;
    uint32_t nextTriggerTimeMs_;
    uint32_t measurementStartMs_;
    uint16_t rawTemperature_;
    uint16_t rawHumidity_;

    int32_t temperatureX100_;
    int32_t humidityX100_;

    MedianFilter temperatureFilter_;
    MedianFilter humidityFilter_;

    void handleTriggerState(uint32_t nowMs);
    void handleReadState(uint32_t nowMs);
    void updateModbus(uint16_t currentTemp, uint16_t currentHumidity);
    void updateLcd();
};

#endif