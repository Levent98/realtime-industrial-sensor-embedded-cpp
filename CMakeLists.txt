cmake_minimum_required(VERSION 3.20)

project(realtime_industrial_sensor C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(CMAKE_ASM_FLAGS "--cpu=Cortex-M4")

set(SOURCES
    App/Src/main.cpp
    App/Src/Application.cpp
    App/Src/ConfigManager.cpp

    Services/Src/AdcService.cpp
    Services/Src/ErrorManager.cpp
    Services/Src/FlashStorage.cpp
    Services/Src/ModbusService.cpp
    Services/Src/SensorService.cpp
    Services/Src/WatchdogService.cpp

    Drivers/Src/DelayTimer.cpp
    Drivers/Src/I2cBus.cpp
    Drivers/Src/LcdDisplay.cpp
    Drivers/Src/PwmOutput.cpp
    Drivers/Src/Sht3xSensor.cpp
    Drivers/Src/SystemClockFacade.cpp

    Config/Src/config_manager.c
    Config/Src/structs.cpp

    Legacy/Src/filter.cpp
    Legacy/Src/flash_write.cpp
    Legacy/Src/i2c.cpp
    Legacy/Src/interrupts.cpp
    Legacy/Src/sysclock.cpp
    Legacy/Src/uart.cpp

    RTE/Device/STM32F410RBTx/system_stm32f4xx.c
    RTE/Device/STM32F410RBTx/startup_stm32f410rx.s
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}

    ${CMAKE_SOURCE_DIR}/App/Inc
    ${CMAKE_SOURCE_DIR}/Services/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/Inc
    ${CMAKE_SOURCE_DIR}/Legacy/Inc
    ${CMAKE_SOURCE_DIR}/Config/Inc
    ${CMAKE_SOURCE_DIR}/Common/Inc

    ${CMAKE_SOURCE_DIR}/RTE
    ${CMAKE_SOURCE_DIR}/RTE/Device/STM32F410RBTx
    ${CMAKE_SOURCE_DIR}/RTE/_Target_1

    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32F410Rx
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:${MCU_FLAGS}>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Oz>
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fdata-sections>
)

target_link_options(${PROJECT_NAME} PRIVATE
    --cpu=Cortex-M4
    --fpu=FPv4-SP
    --scatter=${CMAKE_SOURCE_DIR}/Project/stm32f410.sct
    --remove
    --diag_suppress=L6314W
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "firmware"
    SUFFIX ".axf"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND C:/Keil_v5/ARM/ARMCLANG/bin/fromelf.exe
    --i32combined $<TARGET_FILE:${PROJECT_NAME}>
    --output $<TARGET_FILE_DIR:${PROJECT_NAME}>/firmware.hex
)