#include "ErrorManager.hpp"
#include <cassert>

int main()
{
    ErrorManager errors;

    // TC-ERROR-007: repeated severe I2C timeouts request recovery.
    errors.reportI2cError(I2cStatus::Timeout);
    errors.reportI2cError(I2cStatus::Timeout);
    assert(!errors.recoveryRequested());
    errors.reportI2cError(I2cStatus::Timeout);
    assert(errors.recoveryRequested());
    assert(errors.systemHealthy());

    // TC-ERROR-008: successful recovery clears recovery request and consecutive failure count.
    errors.reportI2cRecovery(true);
    assert(!errors.recoveryRequested());
    assert(errors.consecutiveI2cRecoveryFailCount() == 0U);
    assert(errors.systemHealthy());

    // TC-ERROR-009: repeated failed I2C recoveries escalate to fatal fault.
    errors.reportI2cRecovery(false);
    assert(!errors.fatalFault());
    errors.reportI2cRecovery(false);
    assert(errors.fatalFault());
    assert(!errors.shouldFeedWatchdog());

    // TC-ERROR-010: initialize resets all counters and fatal/recovery flags.
    errors.initialize();
    assert(errors.systemHealthy());
    assert(!errors.fatalFault());
    assert(!errors.recoveryRequested());
    assert(errors.i2cRecoveryCount() == 0U);

    // TC-ERROR-011: repeated flash-save failures escalate to fatal fault.
    errors.reportFlashSaveFailure();
    errors.reportFlashSaveFailure();
    assert(!errors.fatalFault());
    errors.reportFlashSaveFailure();
    assert(errors.flashSaveFailCount() == 3U);
    assert(errors.fatalFault());

    // TC-ERROR-012: flash-save success clears consecutive flash failure count.
    errors.initialize();
    errors.reportFlashSaveFailure();
    assert(errors.consecutiveFlashSaveFailCount() == 1U);
    errors.reportFlashSaveSuccess();
    assert(errors.consecutiveFlashSaveFailCount() == 0U);

    return 0;
}
