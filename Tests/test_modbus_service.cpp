#include "ModbusService.hpp"
#include "ErrorManager.hpp"
#include "Config_Manager.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

extern std::vector<std::uint8_t> g_lastUartTx;
extern std::uint32_t g_fakeTimeUs;

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

int main()
{
    Config_SetDefaults();
    DeviceConfig.id = 1U;

    UartDriver uart;
    ErrorManager errors;
    ModbusService modbus(uart, errors);

    const std::uint8_t rawFrame[] = {
        1U, 3U, 0U, 1U, 0U, 1U, 0xD5U, 0xCAU
    };

    g_fakeTimeUs = 777U;

    assert(modbus.queueFrame(rawFrame, sizeof(rawFrame)) == 1U);
    assert(modbus.queueCount() == 1U);

    frame_item_t dequeued{};

    assert(modbus.dequeueFrame(&dequeued) == 1U);
    assert(dequeued.len == sizeof(rawFrame));
    assert(dequeued.timestamp == 777U);
    assert(std::memcmp(dequeued.data, rawFrame, sizeof(rawFrame)) == 0);
    assert(modbus.isQueueEmpty() == 1U);

    modbus.setHoldingRegister(REG_TEMP_HIGH_ALARM, 650U);

    auto readHolding = makeFrame({
        1U,
        FC_READ_HOLDING,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x00U,
        0x01U
    });

    g_lastUartTx.clear();

    assert(modbus.processFrame(&readHolding) == 1U);
    assert(g_lastUartTx.size() == 7U);
    assert(g_lastUartTx[0] == 1U);
    assert(g_lastUartTx[1] == FC_READ_HOLDING);
    assert(g_lastUartTx[2] == 2U);
    assert(g_lastUartTx[3] == 0x02U);
    assert(g_lastUartTx[4] == 0x8AU);
    assert(txCrcIsValid());

    readHolding.data[readHolding.len - 1U] ^= 0xFFU;

    g_lastUartTx.clear();

    assert(modbus.processFrame(&readHolding) == 0U);
    assert(g_lastUartTx.empty());
    assert(errors.crcErrorCount() == 1U);

    auto wrongSlave = makeFrame({
        2U,
        FC_READ_HOLDING,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x00U,
        0x01U
    });

    g_lastUartTx.clear();

    assert(modbus.processFrame(&wrongSlave) == 0U);
    assert(g_lastUartTx.empty());

    auto writeSingle = makeFrame({
        1U,
        FC_WRITE_SINGLE_REG,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x01U,
        0x2CU
    });

    g_lastUartTx.clear();

    assert(modbus.processFrame(&writeSingle) == 1U);
    assert(modbus.holdingRegister(REG_TEMP_HIGH_ALARM) == 300U);
    assert(g_lastUartTx.size() == 8U);
    assert(g_lastUartTx[1] == FC_WRITE_SINGLE_REG);
    assert(txCrcIsValid());

    return 0;
}