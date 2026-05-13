#include "ModbusService.hpp"
#include "ErrorManager.hpp"
#include "Config_Manager.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

extern std::vector<std::uint8_t> g_lastUartTx;

static std::uint16_t crc16(const std::uint8_t* buffer, std::size_t len)
{
    std::uint16_t crc = 0xFFFFU;

    for (std::size_t i = 0U; i < len; ++i) {
        crc ^= buffer[i];

        for (std::uint8_t j = 0U; j < 8U; ++j) {
            if ((crc & 1U) != 0U) {
                crc = static_cast<std::uint16_t>((crc >> 1U) ^ 0xA001U);
            } else {
                crc = static_cast<std::uint16_t>(crc >> 1U);
            }
        }
    }

    return crc;
}

static frame_item_t makeFrame(std::initializer_list<std::uint8_t> bytesWithoutCrc)
{
    frame_item_t frame{};
    frame.len = static_cast<std::uint16_t>(bytesWithoutCrc.size() + 2U);

    std::size_t idx = 0U;
    for (std::uint8_t b : bytesWithoutCrc) {
        frame.data[idx++] = b;
    }

    const std::uint16_t crc = crc16(frame.data, bytesWithoutCrc.size());
    frame.data[idx++] = static_cast<std::uint8_t>(crc & 0xFFU);
    frame.data[idx++] = static_cast<std::uint8_t>(crc >> 8U);

    return frame;
}

static bool txCrcIsValid()
{
    if (g_lastUartTx.size() < 4U) {
        return false;
    }

    const std::uint16_t rxCrc =
        static_cast<std::uint16_t>(
            g_lastUartTx[g_lastUartTx.size() - 2U] |
            (static_cast<std::uint16_t>(g_lastUartTx[g_lastUartTx.size() - 1U]) << 8U)
        );

    return crc16(g_lastUartTx.data(), g_lastUartTx.size() - 2U) == rxCrc;
}

static bool isExceptionResponse(std::uint8_t function, std::uint8_t code)
{
    return (g_lastUartTx.size() == 5U) &&
           (g_lastUartTx[0] == DeviceConfig.id) &&
           (g_lastUartTx[1] == static_cast<std::uint8_t>(function | 0x80U)) &&
           (g_lastUartTx[2] == code) &&
           txCrcIsValid();
}

int main()
{
    Config_SetDefaults();
    DeviceConfig.id = 1U;

    UartDriver uart;
    ErrorManager errors;
    ModbusService modbus(uart, errors);

    // TC-MODBUS-013 / BT-SW-MODBUS-013: valid read input register request.
    modbus.setInputRegister(REG_TEMP_VALUE, 253U);
    auto readInput = makeFrame({
        1U,
        FC_READ_INPUT,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_VALUE),
        0x00U,
        0x01U
    });

    g_lastUartTx.clear();
    assert(modbus.processFrame(&readInput) == 1U);
    assert(g_lastUartTx.size() == 7U);
    assert(g_lastUartTx[0] == 1U);
    assert(g_lastUartTx[1] == FC_READ_INPUT);
    assert(g_lastUartTx[2] == 2U);
    assert(g_lastUartTx[3] == 0x00U);
    assert(g_lastUartTx[4] == 0xFDU);
    assert(txCrcIsValid());

    // TC-MODBUS-014 / BT-SW-MODBUS-014: unsupported function returns exception.
    auto unsupportedFunction = makeFrame({1U, 0x45U, 0x00U, 0x00U, 0x00U, 0x01U});
    g_lastUartTx.clear();
    assert(modbus.processFrame(&unsupportedFunction) == 0U);
    assert(isExceptionResponse(0x45U, MODBUS_ERR_ILLEGAL_FUNCTION));

    // TC-MODBUS-015 / BT-SW-MODBUS-015: out-of-range holding address is rejected.
    auto illegalAddress = makeFrame({
        1U,
        FC_READ_HOLDING,
        0x00U,
        static_cast<std::uint8_t>(HOLDING_REG_SIZE),
        0x00U,
        0x01U
    });
    g_lastUartTx.clear();
    assert(modbus.processFrame(&illegalAddress) == 0U);
    assert(isExceptionResponse(FC_READ_HOLDING, MODBUS_ERR_ILLEGAL_DATA_ADDR));

    // TC-MODBUS-016 / BT-SW-MODBUS-016: illegal quantity zero is rejected.
    auto zeroQuantity = makeFrame({
        1U,
        FC_READ_HOLDING,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x00U,
        0x00U
    });
    g_lastUartTx.clear();
    assert(modbus.processFrame(&zeroQuantity) == 0U);
    assert(isExceptionResponse(FC_READ_HOLDING, MODBUS_ERR_ILLEGAL_DATA_ADDR));

    // TC-MODBUS-017 / BT-SW-MODBUS-017: write multiple registers updates consecutive registers.
    auto writeMultiple = makeFrame({
        1U,
        FC_WRITE_MULTIPLE_REGS,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x00U,
        0x02U,
        0x04U,
        0x01U,
        0x11U,
        0x02U,
        0x22U
    });
    g_lastUartTx.clear();
    assert(modbus.processFrame(&writeMultiple) == 1U);
    assert(modbus.holdingRegister(REG_TEMP_HIGH_ALARM) == 0x0111U);
    assert(modbus.holdingRegister(REG_TEMP_LOW_ALARM) == 0x0222U);
    assert(g_lastUartTx.size() == 8U);
    assert(g_lastUartTx[1] == FC_WRITE_MULTIPLE_REGS);
    assert(txCrcIsValid());

    // TC-MODBUS-021: broadcast write is accepted but must not generate response.
    auto broadcastWrite = makeFrame({
        0U,
        FC_WRITE_SINGLE_REG,
        0x00U,
        static_cast<std::uint8_t>(REG_HUM_HIGH_ALARM),
        0x03U,
        0x21U
    });
    g_lastUartTx.clear();
    assert(modbus.processFrame(&broadcastWrite) == 1U);
    assert(modbus.holdingRegister(REG_HUM_HIGH_ALARM) == 0x0321U);
    assert(g_lastUartTx.empty());

    return 0;
}
