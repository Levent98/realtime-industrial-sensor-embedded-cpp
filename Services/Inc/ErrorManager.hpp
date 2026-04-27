#ifndef ERROR_MANAGER_HPP
#define ERROR_MANAGER_HPP

#include <stdint.h>
#include "I2cBus.hpp"

class ErrorManager {
public:
    ErrorManager();

    void initialize();
    void process();

    void reportI2cSuccess();
    void reportI2cError(I2cStatus status);
    void reportSensorReadFailure();

    bool recoveryRequested() const;
    void clearRecoveryRequest();

    void markRecoveryDone();
    bool fatalFault() const;

private:
    uint8_t timeoutCount_;
    uint8_t nackCount_;
    uint8_t busErrorCount_;
    uint8_t crcCount_;
    uint8_t sensorReadFailCount_;

    bool recoveryRequested_;
    bool recoveryDone_;
    bool fatalFault_;

    static void incrementSaturated(uint8_t& value);
};

#endif