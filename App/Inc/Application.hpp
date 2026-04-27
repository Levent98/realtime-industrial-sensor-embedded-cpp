#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "SystemClock.hpp"
#include "DelayTimer.hpp"
#include "UartDriver.hpp"
#include "I2cBus.hpp"
#include "Sht3xSensor.hpp"
#include "LcdDisplay.hpp"
#include "PwmOutput.hpp"
#include "AdcService.hpp"
#include "ModbusService.hpp"
#include "WatchdogService.hpp"
#include "ErrorManager.hpp"
#include "FlashStorage.hpp"
#include "ConfigManager.hpp"
#include "SensorService.hpp"

class Application
{
public:
    Application();

    void initialize();
    void run();

private:
    void showStartupScreen();

    SystemClock systemClock_;
    UartDriver uart_;
    I2cBus i2c_;
    Sht3xSensor sensor_;
    LcdDisplay lcd_;
    PwmOutput pwm_;
    AdcService adc_;
    ModbusService modbus_;
    WatchdogService watchdog_;
    ErrorManager errors_;
    FlashStorage flash_;
    ConfigManager config_;
    SensorService sensorService_;
};

#endif