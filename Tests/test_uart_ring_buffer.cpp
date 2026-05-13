#include "UartRingBuffer.hpp"
#include <cassert>
#include <cstdint>

int main()
{
    UartRingBuffer rb;
    std::uint8_t value = 0U;

    // TC-UART-RB-001 / BT-SW-UART-002
    // Production UartRingBuffer preserves FIFO read order.
    assert(rb.push(0x11U));
    assert(rb.push(0x22U));
    assert(rb.pop(value));
    assert(value == 0x11U);
    assert(rb.pop(value));
    assert(value == 0x22U);
    assert(rb.empty());

    // TC-UART-RB-002 / BT-SW-UART-002
    // Production UartRingBuffer preserves ordering across head/tail wrap-around.
    for (std::size_t i = 0U; i < UartRingBuffer::Capacity - 1U; ++i) {
        assert(rb.push(static_cast<std::uint8_t>(i & 0xFFU)));
    }

    for (std::size_t i = 0U; i < 32U; ++i) {
        assert(rb.pop(value));
        assert(value == static_cast<std::uint8_t>(i & 0xFFU));
    }

    for (std::size_t i = 0U; i < 32U; ++i) {
        assert(rb.push(static_cast<std::uint8_t>((200U + i) & 0xFFU)));
    }

    for (std::size_t i = 32U; i < UartRingBuffer::Capacity - 1U; ++i) {
        assert(rb.pop(value));
        assert(value == static_cast<std::uint8_t>(i & 0xFFU));
    }

    for (std::size_t i = 0U; i < 32U; ++i) {
        assert(rb.pop(value));
        assert(value == static_cast<std::uint8_t>((200U + i) & 0xFFU));
    }

    assert(rb.empty());

    // TC-UART-RB-003 / BT-SW-UART-002
    // Production UartRingBuffer rejects overflow without overwriting unread bytes.
    for (std::size_t i = 0U; i < UartRingBuffer::Capacity; ++i) {
        assert(rb.push(static_cast<std::uint8_t>(i & 0xFFU)));
    }

    assert(rb.full());
    assert(!rb.push(0xEEU));
    assert(rb.pop(value));
    assert(value == 0x00U);

    return 0;
}
