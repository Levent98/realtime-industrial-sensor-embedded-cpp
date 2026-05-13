#include "SensorService.hpp"

extern "C" {
#include "appcons.h"
#include "apptdef.h"
}

SensorService::SensorService(Sht3xSensor& sensor,
                             ModbusService& modbus,
                             LcdDisplay& lcd,
                             PwmOutput& pwm,
                             ErrorManager& errors)
    : sensor_(sensor),
      modbus_(modbus),
      lcd_(lcd),
      pwm_(pwm),
      errors_(errors),
      state_(State::Trigger),
      lastSensorTimeMs_(0U),
      nextTriggerTimeMs_(0U),
      measurementStartMs_(0U),
      rawTemperature_(0U),
      rawHumidity_(0U),
      temperatureX100_(0),
      humidityX100_(0),
      temperatureFilter_(),
      humidityFilter_()
{
}

void SensorService::initialize()
{
    state_ = State::Trigger;
    lastSensorTimeMs_ = 0U;
    nextTriggerTimeMs_ = 0U;
    measurementStartMs_ = 0U;

    rawTemperature_ = 0U;
    rawHumidity_ = 0U;
    temperatureX100_ = 0;
    humidityX100_ = 0;

    temperatureFilter_.reset();
    humidityFilter_.reset();
}

void SensorService::process(uint32_t nowMs)
{
    switch (state_) {
    case State::Trigger:
        handleTriggerState(nowMs);
        break;

    case State::Read:
        handleReadState(nowMs);
        break;

    default:
        state_ = State::Trigger;
        nextTriggerTimeMs_ = 0U;
        measurementStartMs_ = 0U;
        break;
    }
}

void SensorService::handleTriggerState(uint32_t nowMs)
{
    if (static_cast<int32_t>(nowMs - nextTriggerTimeMs_) < 0) {
        return;
    }

    const I2cStatus status = sensor_.triggerMeasurement();

    if (status == I2cStatus::Ok) {
        errors_.reportI2cSuccess();

        measurementStartMs_ = nowMs;
        state_ = State::Read;
        return;
    }

    errors_.reportI2cError(status);

    nextTriggerTimeMs_ =
        (status == I2cStatus::BusError) ? (nowMs + 50U) : (nowMs + 100U);

    state_ = State::Trigger;
}

void SensorService::handleReadState(uint32_t nowMs)
{
    if ((nowMs - measurementStartMs_) < MeasureDelayMs) {
        return;
    }

    const I2cStatus status =
        sensor_.readMeasurement(rawTemperature_, rawHumidity_);

    if (status != I2cStatus::Ok) {
        errors_.reportI2cError(status);
        errors_.reportSensorReadFailure();

        nextTriggerTimeMs_ = nowMs + 100U;
        state_ = State::Trigger;
        return;
    }

    errors_.reportI2cSuccess();

    if ((rawHumidity_ == 0xFFFFU) || (rawHumidity_ == 0x0000U) ||
        (rawTemperature_ == 0xFFFFU) || (rawTemperature_ == 0x0000U)) {
        errors_.reportSensorReadFailure();

        nextTriggerTimeMs_ = nowMs + 100U;
        state_ = State::Trigger;
        return;
    }

    errors_.reportSensorReadSuccess();

    const int32_t tempInput = sensor_.temperatureX100(rawTemperature_);
    int32_t humidityInput = sensor_.humidityX100(rawHumidity_);

    if (humidityInput < 0) {
        humidityInput = 0;
    }

    if (humidityInput > 10000) {
        humidityInput = 10000;
    }

    if ((!temperatureFilter_.initialized()) || (!humidityFilter_.initialized())) {
        temperatureFilter_.seed(tempInput);
        humidityFilter_.seed(humidityInput);

        temperatureX100_ = tempInput;
        humidityX100_ = humidityInput;
    } else {
        temperatureX100_ = temperatureFilter_.apply(tempInput);
        humidityX100_ = humidityFilter_.apply(humidityInput);
    }

    updateModbus(static_cast<uint16_t>(temperatureX100_ / 10),
                 static_cast<uint16_t>(humidityX100_ / 10));

    updateLcd();

    nextTriggerTimeMs_ = nowMs + MeasurePeriodMs;
    state_ = State::Trigger;
}

void SensorService::updateModbus(uint16_t currentTemp, uint16_t currentHumidity)
{
    modbus_.setInputRegister(REG_TEMP_VALUE, currentTemp);
    modbus_.setInputRegister(REG_GAS_CONCT_VALUE, currentHumidity);

    uint16_t status = 0U;
    uint16_t alarms = 0U;

    const uint16_t tempHigh = modbus_.holdingRegister(REG_TEMP_HIGH_ALARM);
    const uint16_t tempLow  = modbus_.holdingRegister(REG_TEMP_LOW_ALARM);

    if ((currentTemp > tempHigh) || (currentTemp < tempLow)) {
        status |= 0x01U;

        if (currentTemp > tempHigh) {
            alarms |= 0x01U;
        }

        if (currentTemp < tempLow) {
            alarms |= 0x02U;
        }
    }

    modbus_.setInputRegister(REG_SENSOR_STATUS, status);
    modbus_.setInputRegister(REG_ALARM_FLAGS, alarms);
}

void SensorService::updateLcd()
{
    lcd_.setCursor(0U, 0U);
    lcd_.writeString("T:");
    lcd_.writeFixed(temperatureX100_, 2U);
    lcd_.writeString("C   ");

    lcd_.setCursor(1U, 0U);
    lcd_.writeString("H:");
    lcd_.writeFixed(humidityX100_, 2U);
    lcd_.writeString("%   ");
}