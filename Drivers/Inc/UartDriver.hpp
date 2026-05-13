#ifndef UARTDRIVER_HPP
#define UARTDRIVER_HPP

#include <cstdint>
#include <cstddef>
#include "UartRingBuffer.hpp"

static const std::uint16_t UART_RX_BUF_SIZE = 256U;
static const std::uint16_t UART_TX_BUF_SIZE = 256U;
static const std::uint8_t UART_CMD_QUEUE_SIZE = 8U;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile std::uint16_t baud_rate;
extern volatile std::uint8_t uart_tx_busy;
extern volatile std::uint8_t rtu_frame_ready;
extern volatile std::uint16_t modbus_t35_steps;
extern volatile std::uint16_t modbus_timer_counter;
extern volatile std::uint8_t modbus_timer_running;

void UART_Init(std::uint32_t baud);
std::uint8_t UART_Send(const std::uint8_t* data, std::uint16_t len);
std::uint16_t UART_GetFrame(std::uint8_t* dst, std::uint16_t max_len);
std::uint8_t UART_FrameReady(void);
void UART_ClearFrameFlag(void);
void UART_RTU_Poll(void);
std::uint16_t UART_RxAvailable(void);
std::uint16_t UART_RxRead(std::uint8_t* dst, std::uint16_t len);

void USART1_IRQHandler(void);
void DMA2_Stream7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

class UartDriver {
public:
    static const std::size_t BufferSize = UART_TX_BUF_SIZE;

    UartDriver();
    UartDriver(const UartDriver&) = delete;
    UartDriver& operator=(const UartDriver&) = delete;
    UartDriver(UartDriver&&) = delete;
    UartDriver& operator=(UartDriver&&) = delete;

    void initialize(std::uint32_t baudrate);
    static bool send(const std::uint8_t* data, std::size_t length);
    static void pollFrameTimeout(std::uint32_t nowUs);

    static bool frameAvailable();
    static std::size_t fetchFrame(std::uint8_t* destination, std::size_t maxLength);

    void simulateReceive(const std::uint8_t* data, std::size_t length);

    static void onIdleInterrupt();
    static void onTxCompleteInterrupt();
    void onErrorInterrupt();

    static UartDriver* active();

private:
    static UartDriver* active_;

    std::uint32_t baudrate_;
    bool txBusy_;
    bool frameReady_;
    std::size_t rxLength_;
    std::size_t frameLength_;
    std::uint32_t lastByteTimestampUs_;

    std::uint8_t rxBuffer_[BufferSize];
    std::uint8_t frameBuffer_[BufferSize];
    UartFrameBuffer frameAssembler_;

    void clearRxState();
};

#endif