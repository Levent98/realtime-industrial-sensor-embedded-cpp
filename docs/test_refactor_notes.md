# Production Test Refactor Notes

The UART and SensorService backlog tests were converted from standalone model/specification tests into production-code based tests.

## Completed Refactors

### UART

Added production components:

- `Drivers/Inc/UartRingBuffer.hpp`
- `Drivers/Src/UartRingBuffer.cpp`

These components are used by `UartDriver` and are tested directly by:

- `test_uart_frame_ready`
- `test_uart_ring_buffer`

Backlog items now covered as production tests:

- `BT-SW-UART-001`
- `BT-SW-UART-002`

### SHT3x Sensor

`I2cBus` and `Sht3xSensor` were made mockable through virtual methods, allowing real `Sht3xSensor` production code to be tested with fake I2C implementations.

Production tests:

- `test_sht3x_crc_failure`
- `test_sht3x_i2c_failure`

Backlog items now covered as production tests:

- `BT-SW-SENSOR-005`
- `BT-SW-SENSOR-006`

### SensorService

`SensorService.cpp` is now compiled and executed in host-side tests with fake `Sht3xSensor` implementations and real `ErrorManager` / `ModbusService` logic.

Production tests:

- `test_sensor_service_timing`
- `test_sensor_error_integration`

Backlog items now covered as production tests:

- `BT-SW-SENSOR-007`
- `BT-SW-SENSOR-008`

## Behavioral Fix

`SensorService::handleTriggerState()` no longer calls `ErrorManager::reportI2cSuccess()` immediately after a successful trigger command. A successful trigger only proves the measurement command was sent; it should not clear consecutive sensor read failures before the actual read result is known.

A successful read still calls `reportI2cSuccess()` after measurement data is accepted.

## Current Test Result

Host-side validation result:

```text
15/15 tests passed
```
