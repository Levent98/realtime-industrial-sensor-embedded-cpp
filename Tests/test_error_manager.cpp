#include "ErrorManager.hpp"
#include <cassert>

int main()
{
    ErrorManager errors;

    // TC-ERROR-001: Initial system state should be healthy.
    assert(errors.systemHealthy());
    assert(!errors.recoveryRequested());
    assert(!errors.fatalFault());

    // TC-ERROR-002: CRC error reporting should increment CRC error counter.
    errors.reportCrcError();
    assert(errors.crcErrorCount() == 1U);

    // TC-ERROR-002: A single CRC error should not immediately make the system unhealthy.
    assert(errors.systemHealthy());

    // TC-ERROR-003: Repeated sensor read failures should request recovery.
    errors.reportSensorReadFailure();
    errors.reportSensorReadFailure();
    errors.reportSensorReadFailure();

    assert(errors.sensorReadFailCount() == 3U);
    assert(errors.recoveryRequested());

    // TC-ERROR-004: Recovery request should be clearable.
    errors.clearRecoveryRequest();
    assert(!errors.recoveryRequested());

    // TC-ERROR-005: Repeated UART overflow should escalate to fatal fault.
    errors.reportUartOverflow();
    errors.reportUartOverflow();
    errors.reportUartOverflow();
    errors.reportUartOverflow();
    errors.reportUartOverflow();

    assert(errors.uartOverflowCount() == 5U);
    assert(errors.fatalFault());

    // TC-ERROR-006: Fatal fault should mark system unhealthy and block watchdog feeding.
    assert(!errors.systemHealthy());
    assert(!errors.shouldFeedWatchdog());

    return 0;
}
