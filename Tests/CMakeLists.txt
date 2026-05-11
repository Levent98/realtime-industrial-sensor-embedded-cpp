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
    ../Drivers/Src/Sht3xSensor.cpp
)

target_include_directories(test_sht3x_conversion PRIVATE
    ../Drivers/Inc
)

add_test(NAME test_sht3x_conversion COMMAND test_sht3x_conversion)

add_executable(test_modbus_service
    test_modbus_service.cpp
    HostUartDriverStub.cpp
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