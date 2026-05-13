#include "UartRingBuffer.hpp"
#include <cassert>
#include <cstdint>

int main()
{
    UartFrameBuffer frameBuffer;

    // TC-UART-001 / BT-SW-UART-001
    // Production UartFrameBuffer keeps frameReady false while bytes are still being received.
    assert(frameBuffer.receiveByte(0x01U));
    assert(frameBuffer.receiveByte(0x03U));
    assert(frameBuffer.receiveByte(0x00U));
    assert(!frameBuffer.frameReady());

    // TC-UART-002 / BT-SW-UART-001
    // Production UartFrameBuffer marks a complete frame ready after IDLE/t3.5 completion event.
    assert(frameBuffer.completeFrame());
    assert(frameBuffer.frameReady());

    // TC-UART-003 / BT-SW-UART-001
    // Production UartFrameBuffer preserves bytes and clears frameReady after fetch.
    std::uint8_t frame[8] = {0U};
    const std::size_t length = frameBuffer.fetchFrame(frame, sizeof(frame));

    assert(length == 3U);
    assert(frame[0] == 0x01U);
    assert(frame[1] == 0x03U);
    assert(frame[2] == 0x00U);
    assert(!frameBuffer.frameReady());

    return 0;
}
