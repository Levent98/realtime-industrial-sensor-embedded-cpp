cmake_minimum_required(VERSION 3.20)

project(firmware_host_tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(test_median_filter
    test_median_filter.cpp
    ../Legacy/Src/filter.cpp
)

target_include_directories(test_median_filter PRIVATE
    ../Common/Inc
    ../Legacy/Inc
)

add_test(NAME test_median_filter COMMAND test_median_filter)


add_executable(test_config_manager
    test_config_manager.cpp
    ../Config/Src/config_manager.c
)

target_include_directories(test_config_manager PRIVATE
    ../Config/Inc
)

add_test(NAME test_config_manager COMMAND test_config_manager)


add_executable(test_error_manager
    test_error_manager.cpp
    ../Services/Src/ErrorManager.cpp
)

target_include_directories(test_error_manager PRIVATE
    ../Services/Inc
    ../Drivers/Inc
)

add_test(NAME test_error_manager COMMAND test_error_manager)


add_executable(test_sht3x_conversion
    test_sht3x_conversion.cpp
    HostI2cBusBaseStub.cpp
    ../Drivers/Src/Sht3xSensor.cpp
)

target_include_directories(test_sht3x_conversion PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_sht3x_conversion COMMAND test_sht3x_conversion)

add_executable(test_modbus_service
    test_modbus_service.cpp
    HostUartDriverStub.cpp
    ../Drivers/Src/UartRingBuffer.cpp
    ../Services/Src/ModbusService.cpp
    ../Services/Src/ErrorManager.cpp
    ../Config/Src/config_manager.c
)

target_compile_definitions(test_modbus_service PRIVATE
    UNIT_TEST
)

target_include_directories(test_modbus_service PRIVATE
    ../Services/Inc
    ../Drivers/Inc
    ../Legacy/Inc
    ../Config/Inc
    ../App/Inc
)

add_test(NAME test_modbus_service COMMAND test_modbus_service)

add_executable(test_modbus_edge_cases
    test_modbus_edge_cases.cpp
    HostUartDriverStub.cpp
    ../Drivers/Src/UartRingBuffer.cpp
    ../Services/Src/ModbusService.cpp
    ../Services/Src/ErrorManager.cpp
    ../Config/Src/config_manager.c
)

target_compile_definitions(test_modbus_edge_cases PRIVATE
    UNIT_TEST
)

target_include_directories(test_modbus_edge_cases PRIVATE
    ../Services/Inc
    ../Drivers/Inc
    ../Legacy/Inc
    ../Config/Inc
    ../App/Inc
)

add_test(NAME test_modbus_edge_cases COMMAND test_modbus_edge_cases)


add_executable(test_modbus_queue_boundaries
    test_modbus_queue_boundaries.cpp
    HostUartDriverStub.cpp
    ../Drivers/Src/UartRingBuffer.cpp
    ../Services/Src/ModbusService.cpp
    ../Services/Src/ErrorManager.cpp
    ../Config/Src/config_manager.c
)

target_compile_definitions(test_modbus_queue_boundaries PRIVATE
    UNIT_TEST
)

target_include_directories(test_modbus_queue_boundaries PRIVATE
    ../Services/Inc
    ../Drivers/Inc
    ../Legacy/Inc
    ../Config/Inc
    ../App/Inc
)

add_test(NAME test_modbus_queue_boundaries COMMAND test_modbus_queue_boundaries)


add_executable(test_error_manager_extended
    test_error_manager_extended.cpp
    ../Services/Src/ErrorManager.cpp
)

target_include_directories(test_error_manager_extended PRIVATE
    ../Services/Inc
    ../Drivers/Inc
)

add_test(NAME test_error_manager_extended COMMAND test_error_manager_extended)


add_executable(test_config_edge_cases
    test_config_edge_cases.cpp
    ../Config/Src/config_manager.c
)

target_include_directories(test_config_edge_cases PRIVATE
    ../Config/Inc
)

add_test(NAME test_config_edge_cases COMMAND test_config_edge_cases)

add_executable(test_uart_frame_ready
    test_uart_frame_ready.cpp
    ../Drivers/Src/UartRingBuffer.cpp
)

target_include_directories(test_uart_frame_ready PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_uart_frame_ready COMMAND test_uart_frame_ready)


add_executable(test_uart_ring_buffer
    test_uart_ring_buffer.cpp
    ../Drivers/Src/UartRingBuffer.cpp
)

target_include_directories(test_uart_ring_buffer PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_uart_ring_buffer COMMAND test_uart_ring_buffer)

add_executable(test_sht3x_crc_failure
    test_sht3x_crc_failure.cpp
    HostI2cBusBaseStub.cpp
    ../Drivers/Src/Sht3xSensor.cpp
)

target_compile_definitions(test_sht3x_crc_failure PRIVATE
    UNIT_TEST
)

target_include_directories(test_sht3x_crc_failure PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_sht3x_crc_failure COMMAND test_sht3x_crc_failure)


add_executable(test_sht3x_i2c_failure
    test_sht3x_i2c_failure.cpp
    HostI2cBusBaseStub.cpp
    ../Drivers/Src/Sht3xSensor.cpp
)

target_compile_definitions(test_sht3x_i2c_failure PRIVATE
    UNIT_TEST
)

target_include_directories(test_sht3x_i2c_failure PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_sht3x_i2c_failure COMMAND test_sht3x_i2c_failure)


add_executable(test_sensor_service_timing
    test_sensor_service_timing.cpp
    HostUartDriverStub.cpp
    HostDisplayPwmStub.cpp
    HostI2cBusBaseStub.cpp
    ../Drivers/Src/UartRingBuffer.cpp
    ../Drivers/Src/Sht3xSensor.cpp
    ../Legacy/Src/filter.cpp
    ../Services/Src/SensorService.cpp
    ../Services/Src/ModbusService.cpp
    ../Services/Src/ErrorManager.cpp
    ../Config/Src/config_manager.c
)

target_compile_definitions(test_sensor_service_timing PRIVATE
    UNIT_TEST
)

target_include_directories(test_sensor_service_timing PRIVATE
    ../Services/Inc
    ../Drivers/Inc
    ../Legacy/Inc
    ../Config/Inc
    ../App/Inc
    ../Common/Inc
)

add_test(NAME test_sensor_service_timing COMMAND test_sensor_service_timing)

add_executable(test_sensor_error_integration
    test_sensor_error_integration.cpp
    HostUartDriverStub.cpp
    HostDisplayPwmStub.cpp
    HostI2cBusBaseStub.cpp
    ../Drivers/Src/UartRingBuffer.cpp
    ../Drivers/Src/Sht3xSensor.cpp
    ../Legacy/Src/filter.cpp
    ../Services/Src/SensorService.cpp
    ../Services/Src/ModbusService.cpp
    ../Services/Src/ErrorManager.cpp
    ../Config/Src/config_manager.c
)

target_compile_definitions(test_sensor_error_integration PRIVATE
    UNIT_TEST
)

target_include_directories(test_sensor_error_integration PRIVATE
    ../Services/Inc
    ../Drivers/Inc
    ../Legacy/Inc
    ../Config/Inc
    ../App/Inc
    ../Common/Inc
)

add_test(NAME test_sensor_error_integration COMMAND test_sensor_error_integration)