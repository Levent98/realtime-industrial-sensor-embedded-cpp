#include "Application.hpp"

extern "C" {
#include "delay.h"
}

Application::Application()
    : systemClock_(),
      uart_(),
      i2c_(),
      sensor_(i2c_),
      lcd_(),
      pwm_(),
      modbus_(uart_),
      adc_(modbus_),
      watchdog_(),
      errors_(),
      flash_(),
      config_(flash_),
      sensorService_(sensor_, modbus_, lcd_, pwm_, errors_)
{
}

void Application::initialize()
{
    systemClock_.configure();

    /* Tek delay altyapisi */
    TimConfig();

    lcd_.initialize();
    lcd_.clear();

    uart_.initialize(57600U);
    i2c_.initialize();
    pwm_.initialize();
    adc_.initialize();
    modbus_.initialize(DeviceConfig.id);
    watchdog_.initialize();
    errors_.initialize();
    sensorService_.initialize();

    showStartupScreen();
}

void Application::showStartupScreen()
{
    lcd_.setCursor(0U, 0U);
    lcd_.writeString("OOP Firmware");
    lcd_.setCursor(1U, 0U);
    lcd_.writeString("STM32F410   ");
    Delay_ms(300U);
    lcd_.clear();
}

void Application::run()
{
    while (1) {
        const uint32_t nowMs = GetTick();

        modbus_.process();
        adc_.process();
        sensorService_.process(nowMs);
        errors_.process();

       if (errors_.recoveryRequested()) {
        i2c_.recoverBus();
        errors_.markRecoveryDone();
       }

        watchdog_.kick();
    }
}