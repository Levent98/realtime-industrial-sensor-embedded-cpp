#ifndef UART_RING_BUFFER_HPP
#define UART_RING_BUFFER_HPP

#include <array>
#include <cstddef>
#include <cstdint>

class UartRingBuffer {
public:
    static constexpr std::size_t Capacity = 256U;

    bool push(std::uint8_t value);
    bool pop(std::uint8_t& value);
    bool empty() const;
    bool full() const;
    std::size_t size() const;
    void clear();

private:
    std::array<std::uint8_t, Capacity> buffer_{};
    std::size_t head_ = 0U;
    std::size_t tail_ = 0U;
    std::size_t count_ = 0U;
};

class UartFrameBuffer {
public:
    bool receiveByte(std::uint8_t value);
    bool completeFrame();
    bool frameReady() const;
    std::size_t fetchFrame(std::uint8_t* destination, std::size_t maxLength);
    void clear();

private:
    UartRingBuffer rx_{};
    std::array<std::uint8_t, UartRingBuffer::Capacity> frame_{};
    std::size_t frameLength_ = 0U;
    bool frameReady_ = false;
};

#endif
