#include "ModbusService.hpp"
#include "ErrorManager.hpp"
#include "Config_Manager.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

extern std::vector<std::uint8_t> g_lastUartTx;
extern std::uint32_t g_fakeTimeUs;

// Test helper: calculates Modbus RTU CRC16 so test frames match real wire format.
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

// Test helper: creates a valid Modbus RTU frame by appending CRC automatically.
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

// Test helper: validates the CRC of the response captured by the fake UART driver.
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

    // TC-MODBUS-001: Queue valid frame.
    assert(modbus.queueFrame(rawFrame, sizeof(rawFrame)) == 1U);
    assert(modbus.queueCount() == 1U);

    frame_item_t dequeued{};

    // TC-MODBUS-002: Dequeue frame and preserve data/timestamp.
    assert(modbus.dequeueFrame(&dequeued) == 1U);
    assert(dequeued.len == sizeof(rawFrame));
    assert(dequeued.timestamp == 777U);
    assert(std::memcmp(dequeued.data, rawFrame, sizeof(rawFrame)) == 0);
    assert(modbus.isQueueEmpty() == 1U);

    modbus.setHoldingRegister(REG_TEMP_HIGH_ALARM, 650U);

    auto readHolding = makeFrame({
        1U,                                                // slave ID
        FC_READ_HOLDING,                                   // function code
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),    // register address
        0x00U,
        0x01U                                             // quantity: 1 register
    });

    g_lastUartTx.clear();

    // TC-MODBUS-003: Valid read holding register request should be accepted.
    assert(modbus.processFrame(&readHolding) == 1U);

    // TC-MODBUS-003: Response should contain slave ID, function code, byte count and register value.
    assert(g_lastUartTx.size() == 7U);
    assert(g_lastUartTx[0] == 1U);
    assert(g_lastUartTx[1] == FC_READ_HOLDING);
    assert(g_lastUartTx[2] == 2U);
    assert(g_lastUartTx[3] == 0x02U);
    assert(g_lastUartTx[4] == 0x8AU); // 0x028A = 650

    // TC-MODBUS-004: Generated read response CRC should be valid.
    assert(txCrcIsValid());

    // Corrupt the CRC byte intentionally to simulate noisy RS485 line / invalid frame.
    readHolding.data[readHolding.len - 1U] ^= 0xFFU;

    g_lastUartTx.clear();

    // TC-MODBUS-005: Invalid CRC request should be rejected.
    assert(modbus.processFrame(&readHolding) == 0U);

    // TC-MODBUS-006: Invalid CRC request should not generate a UART response.
    assert(g_lastUartTx.empty());

    // TC-MODBUS-007: Invalid CRC request should increment CRC error counter.
    assert(errors.crcErrorCount() == 1U);

    auto wrongSlave = makeFrame({
        2U,                                                // wrong slave ID
        FC_READ_HOLDING,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x00U,
        0x01U
    });

    g_lastUartTx.clear();

    // TC-MODBUS-008: Frame addressed to another slave should be ignored.
    assert(modbus.processFrame(&wrongSlave) == 0U);

    // TC-MODBUS-009: Wrong slave request should not generate a UART response.
    assert(g_lastUartTx.empty());

    auto writeSingle = makeFrame({
        1U,
        FC_WRITE_SINGLE_REG,
        0x00U,
        static_cast<std::uint8_t>(REG_TEMP_HIGH_ALARM),
        0x01U,
        0x2CU                                             // 0x012C = 300
    });

    g_lastUartTx.clear();

    // TC-MODBUS-010: Valid write single register request should update holding register.
    assert(modbus.processFrame(&writeSingle) == 1U);
    assert(modbus.holdingRegister(REG_TEMP_HIGH_ALARM) == 300U);

    // TC-MODBUS-011: Write single register should generate Modbus echo response.
    assert(g_lastUartTx.size() == 8U);
    assert(g_lastUartTx[1] == FC_WRITE_SINGLE_REG);

    // TC-MODBUS-012: Generated write response CRC should be valid.
    assert(txCrcIsValid());

    return 0;
}
