#ifndef UARTDRIVER_HPP
#define UARTDRIVER_HPP

#include <stdint.h>
#include <stddef.h>

static const uint16_t UART_RX_BUF_SIZE = 256U;
static const uint16_t UART_TX_BUF_SIZE = 256U;
static const uint8_t UART_CMD_QUEUE_SIZE = 8U;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint16_t baud_rate;
extern volatile uint8_t uart_tx_busy;
extern volatile uint8_t rtu_frame_ready;
extern volatile uint16_t modbus_t35_steps;
extern volatile uint16_t modbus_timer_counter;
extern volatile uint8_t modbus_timer_running;

void UART_Init(uint32_t baud);
uint8_t UART_Send(const uint8_t* data, uint16_t len);
uint16_t UART_GetFrame(uint8_t* dst, uint16_t max_len);
uint8_t UART_FrameReady(void);
void UART_ClearFrameFlag(void);
void UART_RTU_Poll(void);
uint16_t UART_RxAvailable(void);
uint16_t UART_RxRead(uint8_t* dst, uint16_t len);

void USART1_IRQHandler(void);
void DMA2_Stream7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

class UartDriver {
public:
    static const size_t BufferSize = UART_TX_BUF_SIZE;

    UartDriver();

    void initialize(uint32_t baudrate);
    bool send(const uint8_t* data, size_t length);
    void pollFrameTimeout(uint32_t nowUs);

    bool frameAvailable() const;
    size_t fetchFrame(uint8_t* destination, size_t maxLength);

    void simulateReceive(const uint8_t* data, size_t length);

    void onIdleInterrupt();
    void onTxCompleteInterrupt();
    void onErrorInterrupt();

    static UartDriver* active();

private:
    static UartDriver* active_;

    uint32_t baudrate_;
    bool txBusy_;
    bool frameReady_;
    size_t rxLength_;
    size_t frameLength_;
    uint32_t lastByteTimestampUs_;

    uint8_t rxBuffer_[BufferSize];
    uint8_t frameBuffer_[BufferSize];

    void clearRxState();
};

#endif