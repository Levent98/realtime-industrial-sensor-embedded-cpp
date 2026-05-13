cmake_minimum_required(VERSION 3.20)

project(realtime_industrial_sensor C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

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
    Drivers/Src/SpiBus.cpp
    Drivers/Src/UartRingBuffer.cpp

    Config/Src/config_manager.c
    Config/Src/structs.cpp

    Legacy/Src/filter.cpp
    Legacy/Src/flash_write.cpp
    Legacy/Src/i2c.cpp
    Legacy/Src/interrupts.cpp
    Legacy/Src/sysclock.cpp
    Legacy/Src/uart.cpp

    Common/Src/syscalls.c

    RTE/Device/STM32F410RBTx/system_stm32f4xx.c
    RTE/Device/STM32F410RBTx/startup_stm32f410rx_gcc.s
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
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

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F410Rx
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}

    -Os
    -ffunction-sections
    -fdata-sections
    -fstack-usage

    -Wall
    -Wextra

    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}

    -T${CMAKE_SOURCE_DIR}/Project/STM32F410RBTx_FLASH.ld

    -specs=nano.specs
    -specs=nosys.specs

    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    OUTPUT_NAME "firmware"
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/firmware.hex
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/firmware.bin
)
set(PCLINT_EXE "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/pclp64.exe")
set(PCLINT_CONFIG_FILES
    "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/config/co-gcc.lnt"
    "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/lnt/au-misra-cpp2.lnt"
)
# LNT (config) dosyalarının olduğu klasör
set(LNT_FILES_DIR "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/config")

# 1. PC-Lint Yollarını Tanımla
set(PCLINT_EXE "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/pclp64.exe")
set(LNT_FILES_DIR "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/config")

add_custom_target(pclint
    COMMAND "${PCLINT_EXE}"

        # Lint config klasörü
        "-i${LNT_FILES_DIR}"

        # GCC compiler config
        "co-gcc.lnt"

        # MISRA C++ profile
        "C:/Users/Arge1/Downloads/pclp.windows.2025-sp1/pclp/lnt/au-misra-cpp2.lnt"

        # Kendi suppress ayarların
        "${CMAKE_SOURCE_DIR}/suppressions.lnt"

        # Çıktı dosyası
        "-os(${CMAKE_SOURCE_DIR}/lint_results.txt)"

        # Macro / compiler compatibility header
        "-header(${CMAKE_SOURCE_DIR}/lint_cmacros.h)"

        # Compiler defines
        "-DSTM32F410Rx"
        "-D__GNUC__"

        # Include yolları
        "-i${CMAKE_SOURCE_DIR}"
        "-i${CMAKE_SOURCE_DIR}/App/Inc"
        "-i${CMAKE_SOURCE_DIR}/Services/Inc"
        "-i${CMAKE_SOURCE_DIR}/Drivers/Inc"
        "-i${CMAKE_SOURCE_DIR}/Legacy/Inc"
        "-i${CMAKE_SOURCE_DIR}/Config/Inc"
        "-i${CMAKE_SOURCE_DIR}/Common/Inc"

        "-i${CMAKE_SOURCE_DIR}/RTE"
        "-i${CMAKE_SOURCE_DIR}/RTE/Device/STM32F410RBTx"
        "-i${CMAKE_SOURCE_DIR}/RTE/_Target_1"

        "-i${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include"
        "-i${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"

        # Source dosyaları
        "${CMAKE_SOURCE_DIR}/App/Src/main.cpp"
        "${CMAKE_SOURCE_DIR}/App/Src/Application.cpp"
        "${CMAKE_SOURCE_DIR}/App/Src/ConfigManager.cpp"

        "${CMAKE_SOURCE_DIR}/Services/Src/ModbusService.cpp"
        "${CMAKE_SOURCE_DIR}/Services/Src/SensorService.cpp"

        "${CMAKE_SOURCE_DIR}/Drivers/Src/DelayTimer.cpp"
        "${CMAKE_SOURCE_DIR}/Drivers/Src/Sht3xSensor.cpp"

        "${CMAKE_SOURCE_DIR}/Legacy/Src/sysclock.cpp"
        "${CMAKE_SOURCE_DIR}/Legacy/Src/filter.cpp"

    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"

    VERBATIM

    COMMENT "PC-Lint Plus MISRA analizi yapiliyor..."
)