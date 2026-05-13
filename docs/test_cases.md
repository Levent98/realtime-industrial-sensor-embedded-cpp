\# Host-Side Unit Test Cases



This document summarizes the host-side unit tests used to validate firmware logic without requiring STM32 hardware.



\## Test Case Matrix



| Test ID | Test File | Scenario | Expected Result |

|---|---|---|---|

| TC-FILTER-001 | test\_median\_filter.cpp | Filter starts uninitialized | `initialized()` returns false |

| TC-FILTER-002 | test\_median\_filter.cpp | First sample is applied | First value is returned |

| TC-FILTER-003 | test\_median\_filter.cpp | Filter is seeded | `initialized()` returns true |

| TC-FILTER-004 | test\_median\_filter.cpp | Outlier sample is added | Median remains stable |

| TC-FILTER-005 | test\_median\_filter.cpp | Filter is reset | `initialized()` returns false |



| TC-CONFIG-001 | test\_config\_manager.cpp | Invalid/default config state | Defaults are loaded |

| TC-CONFIG-002 | test\_config\_manager.cpp | Base settings update | Device ID is updated |

| TC-CONFIG-003 | test\_config\_manager.cpp | Null base settings pointer | Function rejects input |

| TC-CONFIG-004 | test\_config\_manager.cpp | Sensor settings update | Alarm threshold and save flags update |

| TC-CONFIG-005 | test\_config\_manager.cpp | Null sensor settings pointer | Function rejects input |

| TC-CONFIG-006 | test\_config\_manager.cpp | Valid sensor config check | Function accepts config |

| TC-CONFIG-007 | test\_config\_manager.cpp | Null sensor config pointer | Function rejects config |



| TC-ERROR-001 | test\_error\_manager.cpp | Initial system state | System is healthy |

| TC-ERROR-002 | test\_error\_manager.cpp | CRC error reported | CRC counter increments |

| TC-ERROR-003 | test\_error\_manager.cpp | Repeated sensor read failures | Recovery is requested |

| TC-ERROR-004 | test\_error\_manager.cpp | Recovery request cleared | Recovery flag clears |

| TC-ERROR-005 | test\_error\_manager.cpp | Repeated UART overflow | Fatal fault is triggered |

| TC-ERROR-006 | test\_error\_manager.cpp | Fatal fault state | Watchdog feed is blocked |



| TC-SENSOR-001 | test\_sht3x\_conversion.cpp | Raw temperature = 0 | Converted value is -45.00°C |

| TC-SENSOR-002 | test\_sht3x\_conversion.cpp | Raw temperature = 65535 | Converted value is 130.00°C |

| TC-SENSOR-003 | test\_sht3x\_conversion.cpp | Raw humidity = 0 | Converted value is 0.00%RH |

| TC-SENSOR-004 | test\_sht3x\_conversion.cpp | Raw humidity = 65535 | Converted value is 100.00%RH |



| TC-MODBUS-001 | test\_modbus\_service.cpp | Queue valid frame | Frame is queued successfully |

| TC-MODBUS-002 | test\_modbus\_service.cpp | Dequeue frame | Frame data and timestamp are preserved |

| TC-MODBUS-003 | test\_modbus\_service.cpp | Read holding register request | Valid Modbus response is generated |

| TC-MODBUS-004 | test\_modbus\_service.cpp | Response CRC validation | Generated response CRC is valid |

| TC-MODBUS-005 | test\_modbus\_service.cpp | Invalid CRC request | Frame is rejected |

| TC-MODBUS-006 | test\_modbus\_service.cpp | Invalid CRC request | No UART response is generated |

| TC-MODBUS-007 | test\_modbus\_service.cpp | Invalid CRC request | CRC error counter increments |

| TC-MODBUS-008 | test\_modbus\_service.cpp | Wrong slave ID request | Frame is ignored |

| TC-MODBUS-009 | test\_modbus\_service.cpp | Wrong slave ID request | No UART response is generated |

| TC-MODBUS-010 | test\_modbus\_service.cpp | Write single register request | Holding register is updated |

| TC-MODBUS-011 | test\_modbus\_service.cpp | Write single register request | Correct Modbus echo response is generated |

| TC-MODBUS-012 | test\_modbus\_service.cpp | Write response CRC validation | Generated response CRC is valid |



\## Execution



Run all host-side tests locally:



```bash

cmake -S Tests -B build-tests -G Ninja

cmake --build build-tests

ctest --test-dir build-tests --output-on-failure



## Production Test Refactor Status

The following previously model-based tests now execute production code paths:

| Backlog ID | Test | Production Code Covered | Status |
|---|---|---|---|
| BT-SW-UART-001 | test_uart_frame_ready | UartFrameBuffer / UartDriver support component | Automated - Production Test Added |
| BT-SW-UART-002 | test_uart_ring_buffer | UartRingBuffer | Automated - Production Test Added |
| BT-SW-SENSOR-005 | test_sht3x_crc_failure | Sht3xSensor::readMeasurement | Automated - Production Test Added |
| BT-SW-SENSOR-006 | test_sht3x_i2c_failure | Sht3xSensor::triggerMeasurement | Automated - Production Test Added |
| BT-SW-SENSOR-007 | test_sensor_service_timing | SensorService.cpp timing path | Automated - Production Test Added |
| BT-SW-SENSOR-008 | test_sensor_error_integration | SensorService.cpp + ErrorManager.cpp integration | Automated - Production Test Added |
