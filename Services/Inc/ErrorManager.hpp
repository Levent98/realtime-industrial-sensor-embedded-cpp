#ifndef ERROR_MANAGER_HPP
#define ERROR_MANAGER_HPP

#include <stdint.h>
#include "I2cBus.hpp"

class ErrorManager {
public:
    ErrorManager();

    ~ErrorManager() = default;

    ErrorManager(const ErrorManager&) = delete;
    ErrorManager& operator=(const ErrorManager&) = delete;

    ErrorManager(ErrorManager&&) = delete;
    ErrorManager& operator=(ErrorManager&&) = delete;

    void initialize();
    void process();

    void reportI2cSuccess();
    void reportI2cError(I2cStatus status);
    void reportSensorReadFailure();
    void reportSensorReadSuccess();
    void reportCrcError();
    void reportValidModbusFrame();
    void reportUartOverflow();
    void reportI2cRecovery(bool success);
    void reportWatchdogReset();
    void reportFlashSaveFailure();
    void reportFlashSaveSuccess();

    [[nodiscard]] uint8_t crcErrorCount() const;
    [[nodiscard]] uint8_t uartOverflowCount() const;
    [[nodiscard]] uint8_t sensorReadFailCount() const;
    [[nodiscard]] uint8_t i2cRecoveryCount() const;
    [[nodiscard]] uint8_t watchdogResetCount() const;
    [[nodiscard]] uint8_t flashSaveFailCount() const;

    [[nodiscard]] uint8_t consecutiveSensorReadFailCount() const;
    [[nodiscard]] uint8_t consecutiveUartOverflowCount() const;
    [[nodiscard]] uint8_t consecutiveI2cRecoveryFailCount() const;
    [[nodiscard]] uint8_t consecutiveFlashSaveFailCount() const;

    [[nodiscard]] bool recoveryRequested() const;
    void clearRecoveryRequest();

    void markRecoveryDone();
    [[nodiscard]] bool fatalFault() const;
    [[nodiscard]] bool systemHealthy() const;
    [[nodiscard]] bool shouldFeedWatchdog() const;

private:
    static constexpr uint8_t I2cRecoveryThreshold = 3U;
    static constexpr uint8_t SensorFailThreshold = 3U;
    static constexpr uint8_t UartOverflowThreshold = 5U;
    static constexpr uint8_t I2cRecoveryFailThreshold = 2U;
    static constexpr uint8_t FlashSaveFailThreshold = 3U;

    uint8_t timeoutCount_;
    uint8_t nackCount_;
    uint8_t busErrorCount_;

    uint8_t crcCount_;
    uint8_t sensorReadFailCount_;
    uint8_t uartOverflowCount_;
    uint8_t i2cRecoveryCount_;
    uint8_t watchdogResetCount_;
    uint8_t flashSaveFailCount_;

    uint8_t consecutiveSensorReadFailCount_;
    uint8_t consecutiveUartOverflowCount_;
    uint8_t consecutiveI2cRecoveryFailCount_;
    uint8_t consecutiveFlashSaveFailCount_;

    bool recoveryRequested_;
    bool recoveryDone_;
    bool fatalFault_;

    static void incrementSaturated(uint8_t& value);
};

#endif
