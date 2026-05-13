#include "ErrorManager.hpp"

ErrorManager::ErrorManager()
    : timeoutCount_(0U),
      nackCount_(0U),
      busErrorCount_(0U),
      crcCount_(0U),
      sensorReadFailCount_(0U),
      uartOverflowCount_(0U),
      i2cRecoveryCount_(0U),
      watchdogResetCount_(0U),
      flashSaveFailCount_(0U),
      consecutiveSensorReadFailCount_(0U),
      consecutiveUartOverflowCount_(0U),
      consecutiveI2cRecoveryFailCount_(0U),
      consecutiveFlashSaveFailCount_(0U),
      recoveryRequested_(false),
      recoveryDone_(false),
      fatalFault_(false)
{
}

void ErrorManager::initialize()
{
    timeoutCount_ = 0U;
    nackCount_ = 0U;
    busErrorCount_ = 0U;

    crcCount_ = 0U;
    sensorReadFailCount_ = 0U;
    uartOverflowCount_ = 0U;
    i2cRecoveryCount_ = 0U;
    watchdogResetCount_ = 0U;
    flashSaveFailCount_ = 0U;

    consecutiveSensorReadFailCount_ = 0U;
    consecutiveUartOverflowCount_ = 0U;
    consecutiveI2cRecoveryFailCount_ = 0U;
    consecutiveFlashSaveFailCount_ = 0U;

    recoveryRequested_ = false;
    recoveryDone_ = false;
    fatalFault_ = false;
}

void ErrorManager::process()
{
    const uint8_t severeI2cCount =
        static_cast<uint8_t>(timeoutCount_ + busErrorCount_);

    if (severeI2cCount >= I2cRecoveryThreshold) {
        recoveryRequested_ = true;
    }

    if (consecutiveSensorReadFailCount_ >= SensorFailThreshold) {
        recoveryRequested_ = true;
    }

    if (consecutiveI2cRecoveryFailCount_ >= I2cRecoveryFailThreshold) {
        fatalFault_ = true;
    }

    if (consecutiveUartOverflowCount_ >= UartOverflowThreshold) {
        fatalFault_ = true;
    }

    if (consecutiveFlashSaveFailCount_ >= FlashSaveFailThreshold) {
        fatalFault_ = true;
    }
}

void ErrorManager::reportI2cSuccess()
{
    timeoutCount_ = 0U;
    nackCount_ = 0U;
    busErrorCount_ = 0U;
    consecutiveI2cRecoveryFailCount_ = 0U;
}

void ErrorManager::reportI2cError(I2cStatus status)
{
    switch (status) {
    case I2cStatus::Timeout:
        incrementSaturated(timeoutCount_);
        break;

    case I2cStatus::Nack:
        incrementSaturated(nackCount_);
        break;

    case I2cStatus::BusError:
        incrementSaturated(busErrorCount_);
        break;

    default:
        break;
    }

    process();
}

void ErrorManager::reportSensorReadFailure()
{
    incrementSaturated(sensorReadFailCount_);
    incrementSaturated(consecutiveSensorReadFailCount_);
    process();
}

void ErrorManager::reportSensorReadSuccess()
{
    consecutiveSensorReadFailCount_ = 0U;
}

void ErrorManager::reportCrcError()
{
    incrementSaturated(crcCount_);
    /* No watchdog penalty: noisy RS485 frames should be diagnosable, not fatal. */
}

void ErrorManager::reportValidModbusFrame()
{
    consecutiveUartOverflowCount_ = 0U;
}

void ErrorManager::reportUartOverflow()
{
    incrementSaturated(uartOverflowCount_);
    incrementSaturated(consecutiveUartOverflowCount_);
    process();
}

void ErrorManager::reportI2cRecovery(bool success)
{
    incrementSaturated(i2cRecoveryCount_);

    if (success) {
        timeoutCount_ = 0U;
        busErrorCount_ = 0U;
        consecutiveI2cRecoveryFailCount_ = 0U;
        recoveryRequested_ = false;
        recoveryDone_ = true;
    } else {
        incrementSaturated(consecutiveI2cRecoveryFailCount_);
    }

    process();
}

void ErrorManager::reportWatchdogReset()
{
    incrementSaturated(watchdogResetCount_);
}

void ErrorManager::reportFlashSaveFailure()
{
    incrementSaturated(flashSaveFailCount_);
    incrementSaturated(consecutiveFlashSaveFailCount_);
    process();
}

void ErrorManager::reportFlashSaveSuccess()
{
    consecutiveFlashSaveFailCount_ = 0U;
}

bool ErrorManager::recoveryRequested() const
{
    return recoveryRequested_;
}

void ErrorManager::clearRecoveryRequest()
{
    recoveryRequested_ = false;
}

void ErrorManager::markRecoveryDone()
{
    recoveryDone_ = true;
    recoveryRequested_ = false;
}

bool ErrorManager::fatalFault() const
{
    return fatalFault_;
}

bool ErrorManager::systemHealthy() const
{
    return !fatalFault_;
}

bool ErrorManager::shouldFeedWatchdog() const
{
    return systemHealthy();
}

uint8_t ErrorManager::crcErrorCount() const { return crcCount_; }
uint8_t ErrorManager::uartOverflowCount() const { return uartOverflowCount_; }
uint8_t ErrorManager::sensorReadFailCount() const { return sensorReadFailCount_; }
uint8_t ErrorManager::i2cRecoveryCount() const { return i2cRecoveryCount_; }
uint8_t ErrorManager::watchdogResetCount() const { return watchdogResetCount_; }
uint8_t ErrorManager::flashSaveFailCount() const { return flashSaveFailCount_; }

uint8_t ErrorManager::consecutiveSensorReadFailCount() const
{
    return consecutiveSensorReadFailCount_;
}

uint8_t ErrorManager::consecutiveUartOverflowCount() const
{
    return consecutiveUartOverflowCount_;
}

uint8_t ErrorManager::consecutiveI2cRecoveryFailCount() const
{
    return consecutiveI2cRecoveryFailCount_;
}

uint8_t ErrorManager::consecutiveFlashSaveFailCount() const
{
    return consecutiveFlashSaveFailCount_;
}

void ErrorManager::incrementSaturated(uint8_t& value)
{
    if (value < 255U) {
        ++value;
    }
}
