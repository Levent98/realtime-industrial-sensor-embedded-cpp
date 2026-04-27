Realtime Industrial Sensor (STM32 Bare-Metal)

This project implements a bare-metal embedded system on an STM32 microcontroller for real-time temperature and humidity monitoring, designed with industrial-grade considerations such as robustness, modularity, and communication reliability.

The system integrates sensor acquisition, Modbus RTU communication over RS485, LCD display output, ADC measurements, watchdog supervision, and flash-based configuration storage.

Features
Layered architecture (C and C++ hybrid)
Modbus RTU communication over RS485
SHT3x sensor integration over I2C
LCD driver (HD44780 compatible, 4-bit mode)
ADC sampling and processing
Watchdog supervision
Flash-based persistent configuration
Error detection and handling
Project Structure

The project is organized into a layered architecture to improve maintainability and scalability:

App/        Application entry point and orchestration
Services/   Application services (Modbus, Sensor, ADC, Watchdog, Flash, Error handling)
Drivers/    C++ hardware abstraction layer (peripheral interfaces)
Legacy/     Low-level C drivers preserved for compatibility
Config/     Configuration structures and legacy configuration API
Common/     Shared utilities and headers
Project/    Keil-specific files (scatter file, Event Recorder stubs)
RTE/        CMSIS and Keil auto-generated startup/system files
Architecture Overview

The system follows a layered embedded design:

Application Layer (App/)

Entry point of the system
Coordinates system behavior

Service Layer (Services/)

Contains core logic
Includes Modbus, sensor processing, ADC handling, watchdog management

Driver Layer (Drivers/)

Hardware abstraction using C++
Encapsulates peripheral access

Legacy Layer (Legacy/)

Existing low-level C drivers
Preserved for compatibility and gradual migration

Configuration Layer (Config/)

System configuration and persistent parameters

Common (Common/)

Shared utilities and helper structures
Hardware Details
MCU: STM32F4 series (bare-metal)
Communication:
RS485 (Modbus RTU)
I2C (SHT3x sensor)
Peripherals:
UART (DMA-based)
ADC
Timers (microsecond precision)
Display:
HD44780-compatible LCD (4-bit mode)
Communication

Modbus RTU

Implemented over RS485
Frame-based communication with CRC validation
Supports register read and write operations

SHT3x Sensor

I2C-based communication
Periodic measurement with filtering
Build and Development
IDE: Keil MDK (ARMCLANG)
Language: C and C++
Architecture: bare-metal (no RTOS)

Notes:

Include paths and project configuration are aligned with the layered structure
CMSIS and startup files are located under RTE/
Reliability Features
Watchdog-based supervision
Error tracking (I2C, communication, etc.)
Timeout handling
Bus recovery mechanisms (I2C)
Migration Strategy

The project follows a gradual migration from C to modern C++:

Legacy drivers remain in Legacy/
New modules are implemented in Drivers/ using C++
Services interact through abstraction layers

Benefits:

Improved modularity
Safer refactoring
Better testability
Easier transition to RTOS-based systems
Future Improvements
FreeRTOS integration
Advanced fault logging (Flash or SD card)
SCADA or MQTT integration
Configurable Modbus register mapping
Bootloader support
Notes
Some files are not included due to confidentiality reasons
The project reflects an industrial-oriented embedded system design approach
Author

Levent Keskin
Embedded Software Engineer