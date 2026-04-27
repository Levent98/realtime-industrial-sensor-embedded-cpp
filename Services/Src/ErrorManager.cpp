#include "ErrorManager.hpp"

ErrorManager::ErrorManager()
    : timeoutCount_(0U),
      nackCount_(0U),
      busErrorCount_(0U),
      crcCount_(0U),
      sensorReadFailCount_(0U),
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

    recoveryRequested_ = false;
    recoveryDone_ = false;
    fatalFault_ = false;
}

void ErrorManager::process()
{
    const uint8_t severeCount =
        static_cast<uint8_t>(busErrorCount_ + timeoutCount_);

    if (severeCount >= 3U) {
        if (!recoveryDone_) {
            recoveryRequested_ = true;
        } else {
            fatalFault_ = true;
        }
    }

    if (sensorReadFailCount_ >= 3U) {
        if (!recoveryDone_) {
            recoveryRequested_ = true;
        } else {
            fatalFault_ = true;
        }
    }
}

void ErrorManager::reportI2cSuccess()
{
    timeoutCount_ = 0U;
    nackCount_ = 0U;
    busErrorCount_ = 0U;
    crcCount_ = 0U;
    sensorReadFailCount_ = 0U;

    recoveryRequested_ = false;
    recoveryDone_ = false;
    fatalFault_ = false;
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
    process();
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

void ErrorManager::incrementSaturated(uint8_t& value)
{
    if (value < 255U) {
        ++value;
    }
}