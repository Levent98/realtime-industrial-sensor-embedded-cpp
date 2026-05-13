// Host-side UART driver stub used by test_modbus_service.cpp.
// It captures transmitted Modbus responses in g_lastUartTx instead of using real UART/RS485 hardware.

#include "UartDriver.hpp"
#include <vector>

std::vector<std::uint8_t> g_lastUartTx;
std::uint32_t g_fakeTimeUs = 1234U;

extern "C" std::uint32_t GetTime_us(void)
{
    return g_fakeTimeUs;
}

volatile std::uint16_t baud_rate = 9600U;
volatile std::uint8_t uart_tx_busy = 0U;
volatile std::uint8_t rtu_frame_ready = 0U;
volatile std::uint16_t modbus_t35_steps = 35U;
volatile std::uint16_t modbus_timer_counter = 0U;
volatile std::uint8_t modbus_timer_running = 0U;

UartDriver* UartDriver::active_ = nullptr;

UartDriver::UartDriver()
    : baudrate_(0U),
      txBusy_(false),
      frameReady_(false),
      rxLength_(0U),
      frameLength_(0U),
      lastByteTimestampUs_(0U),
      rxBuffer_{0},
      frameBuffer_{0},
      frameAssembler_()
{
}

UartDriver* UartDriver::active()
{
    return active_;
}

void UartDriver::initialize(std::uint32_t baudrate)
{
    baud_rate = static_cast<std::uint16_t>(baudrate);
    active_ = this;
}

bool UartDriver::send(const std::uint8_t* data, std::size_t length)
{
    if ((data == nullptr) || (length == 0U) || (length > BufferSize)) {
        return false;
    }

    g_lastUartTx.assign(data, data + length);
    return true;
}

void UartDriver::pollFrameTimeout(std::uint32_t) {}
bool UartDriver::frameAvailable() { return false; }
std::size_t UartDriver::fetchFrame(std::uint8_t*, std::size_t) { return 0U; }
void UartDriver::simulateReceive(const std::uint8_t*, std::size_t) {}
void UartDriver::onIdleInterrupt() {}
void UartDriver::onTxCompleteInterrupt() {}

void UartDriver::onErrorInterrupt()
{
    clearRxState();
}

void UartDriver::clearRxState()
{
    rxLength_ = 0U;
    frameLength_ = 0U;
    frameReady_ = false;
    frameAssembler_.clear();
}