#include "ModbusService.hpp"
#include "ErrorManager.hpp"
#include "Config_Manager.h"

#include <array>
#include <cassert>
#include <cstdint>

extern std::uint32_t g_fakeTimeUs;

int main()
{
    Config_SetDefaults();
    DeviceConfig.id = 1U;

    UartDriver uart;
    ErrorManager errors;
    ModbusService modbus(uart, errors);

    const std::uint8_t sampleFrame[] = {1U, 3U, 0U, 1U, 0U, 1U, 0xD5U, 0xCAU};

    // TC-MODBUS-019 / BT-SW-MODBUS-019: null frame pointer is rejected safely.
    assert(modbus.queueFrame(nullptr, sizeof(sampleFrame)) == 0U);
    assert(modbus.queueCount() == 0U);

    // TC-MODBUS-018 / BT-SW-MODBUS-018: queue accepts exactly FRAME_QUEUE_SIZE frames.
    for (std::uint8_t i = 0U; i < FRAME_QUEUE_SIZE; ++i) {
        g_fakeTimeUs = static_cast<std::uint32_t>(1000U + i);
        assert(modbus.queueFrame(sampleFrame, sizeof(sampleFrame)) == 1U);
    }

    assert(modbus.queueCount() == FRAME_QUEUE_SIZE);
    assert(modbus.isQueueFull() == 1U);

    // TC-MODBUS-018 / BT-SW-MODBUS-018: pushing one more frame increments overflow and ErrorManager.
    assert(modbus.queueFrame(sampleFrame, sizeof(sampleFrame)) == 0U);
    assert(modbus.overflowCount() == 1U);
    assert(errors.uartOverflowCount() == 1U);

    // TC-MODBUS-018: dequeue order and timestamps are preserved.
    for (std::uint8_t i = 0U; i < FRAME_QUEUE_SIZE; ++i) {
        frame_item_t out{};
        assert(modbus.dequeueFrame(&out) == 1U);
        assert(out.len == sizeof(sampleFrame));
        assert(out.timestamp == static_cast<std::uint32_t>(1000U + i));
        assert(out.data[0] == 1U);
        assert(out.data[1] == 3U);
    }

    assert(modbus.isQueueEmpty() == 1U);

    // TC-MODBUS-020 / BT-SW-MODBUS-020: oversize queued frame is truncated to 256 bytes.
    std::array<std::uint8_t, 300U> oversized{};
    for (std::size_t i = 0U; i < oversized.size(); ++i) {
        oversized[i] = static_cast<std::uint8_t>(i & 0xFFU);
    }

    g_fakeTimeUs = 4242U;
    assert(modbus.queueFrame(oversized.data(), static_cast<std::uint16_t>(oversized.size())) == 1U);

    frame_item_t truncated{};
    assert(modbus.dequeueFrame(&truncated) == 1U);
    assert(truncated.len == 256U);
    assert(truncated.timestamp == 4242U);
    assert(truncated.data[0] == 0U);
    assert(truncated.data[255] == 255U);

    // TC-MODBUS-022: clearQueue resets queued frames and overflow counter.
    assert(modbus.queueFrame(sampleFrame, sizeof(sampleFrame)) == 1U);
    modbus.clearQueue();
    assert(modbus.isQueueEmpty() == 1U);
    assert(modbus.overflowCount() == 0U);

    return 0;
}
