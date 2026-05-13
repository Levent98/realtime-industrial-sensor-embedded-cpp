#include "UartRingBuffer.hpp"

bool UartRingBuffer::push(std::uint8_t value)
{
    if (full()) {
        return false;
    }

    buffer_[head_] = value;
    head_ = (head_ + 1U) % Capacity;
    ++count_;
    return true;
}

bool UartRingBuffer::pop(std::uint8_t& value)
{
    if (empty()) {
        return false;
    }

    value = buffer_[tail_];
    tail_ = (tail_ + 1U) % Capacity;
    --count_;
    return true;
}

bool UartRingBuffer::empty() const
{
    return count_ == 0U;
}

bool UartRingBuffer::full() const
{
    return count_ == Capacity;
}

std::size_t UartRingBuffer::size() const
{
    return count_;
}

void UartRingBuffer::clear()
{
    head_ = 0U;
    tail_ = 0U;
    count_ = 0U;
}

bool UartFrameBuffer::receiveByte(std::uint8_t value)
{
    frameReady_ = false;
    return rx_.push(value);
}

bool UartFrameBuffer::completeFrame()
{
    if (rx_.empty()) {
        return false;
    }

    frameLength_ = 0U;
    std::uint8_t value = 0U;

    while ((!rx_.empty()) && (frameLength_ < frame_.size())) {
        static_cast<void>(rx_.pop(value));
        frame_[frameLength_] = value;
        ++frameLength_;
    }

    frameReady_ = frameLength_ > 0U;
    return frameReady_;
}

bool UartFrameBuffer::frameReady() const
{
    return frameReady_;
}

std::size_t UartFrameBuffer::fetchFrame(std::uint8_t* destination, std::size_t maxLength)
{
    if ((!frameReady_) || (destination == nullptr) || (maxLength == 0U)) {
        return 0U;
    }

    const std::size_t length = (frameLength_ < maxLength) ? frameLength_ : maxLength;

    for (std::size_t i = 0U; i < length; ++i) {
        destination[i] = frame_[i];
    }

    frameReady_ = false;
    frameLength_ = 0U;
    return length;
}

void UartFrameBuffer::clear()
{
    rx_.clear();
    frameLength_ = 0U;
    frameReady_ = false;
}
