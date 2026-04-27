#include "LcdDisplay.hpp"

extern "C" {
#include "stm32f410rx.h"
#include "delay.h"
}

namespace
{
    constexpr uint32_t GPIOC_LCD_PINS_MODER_MASK =
        (3U << 0U)  |   // PC0
        (3U << 2U)  |   // PC1
        (3U << 4U)  |   // PC2
        (3U << 6U)  |   // PC3
        (3U << 26U) |   // PC13
        (3U << 28U) |   // PC14
        (3U << 30U);    // PC15

    constexpr uint32_t GPIOC_LCD_PINS_OUTPUT_MODE =
        (1U << 0U)  |
        (1U << 2U)  |
        (1U << 4U)  |
        (1U << 6U)  |
        (1U << 26U) |
        (1U << 28U) |
        (1U << 30U);

    constexpr uint32_t GPIOB_LCD_PIN_MODER_MASK = (3U << 16U); // PB8
    constexpr uint32_t GPIOB_LCD_PIN_OUTPUT_MODE = (1U << 16U);

    constexpr uint32_t GPIOC_LCD_PINS_OTYPER_MASK =
        (1U << 0U)  |
        (1U << 1U)  |
        (1U << 2U)  |
        (1U << 3U)  |
        (1U << 13U) |
        (1U << 14U) |
        (1U << 15U);

    constexpr uint32_t GPIOB_LCD_PIN_OTYPER_MASK = (1U << 8U);

    constexpr uint32_t GPIOC_LCD_PINS_PUPDR_MASK =
        (3U << 0U)  |
        (3U << 2U)  |
        (3U << 4U)  |
        (3U << 6U)  |
        (3U << 26U) |
        (3U << 28U) |
        (3U << 30U);

    constexpr uint32_t GPIOB_LCD_PIN_PUPDR_MASK = (3U << 16U);
}

LcdDisplay::LcdDisplay()
    : finalBuffer_{0}
{
}

void LcdDisplay::rsHigh()
{
    GPIOC->BSRR = GPIO_BSRR_BS_1;
}

void LcdDisplay::rsLow()
{
    GPIOC->BSRR = GPIO_BSRR_BR_1;
}

void LcdDisplay::rwLow()
{
    GPIOC->BSRR = GPIO_BSRR_BR_0;
}

void LcdDisplay::eHigh()
{
    GPIOC->BSRR = GPIO_BSRR_BS_2;
}

void LcdDisplay::eLow()
{
    GPIOC->BSRR = GPIO_BSRR_BR_2;
}

void LcdDisplay::backlightHigh()
{
    GPIOC->BSRR = GPIO_BSRR_BS_3;
}

void LcdDisplay::writeDb4To7(uint8_t nibble)
{
    uint32_t gpioCBsrrValue = 0U;
    uint32_t gpioBBsrrValue = 0U;

    /* DB4 -> PC14 */
    if ((nibble & 0x01U) != 0U) {
        gpioCBsrrValue |= (1U << 14U);
    } else {
        gpioCBsrrValue |= (1U << (14U + 16U));
    }

    /* DB5 -> PC15 */
    if ((nibble & 0x02U) != 0U) {
        gpioCBsrrValue |= (1U << 15U);
    } else {
        gpioCBsrrValue |= (1U << (15U + 16U));
    }

    /* DB6 -> PB8 */
    if ((nibble & 0x04U) != 0U) {
        gpioBBsrrValue |= (1U << 8U);
    } else {
        gpioBBsrrValue |= (1U << (8U + 16U));
    }

    /* DB7 -> PC13 */
    if ((nibble & 0x08U) != 0U) {
        gpioCBsrrValue |= (1U << 13U);
    } else {
        gpioCBsrrValue |= (1U << (13U + 16U));
    }

    GPIOC->BSRR = gpioCBsrrValue;
    GPIOB->BSRR = gpioBBsrrValue;
}

void LcdDisplay::send4(uint8_t nibble)
{
    writeDb4To7(nibble & 0x0FU);

    Delay_us(1U);
    eHigh();
    Delay_us(5U);
    eLow();
    Delay_us(5U);
}

void LcdDisplay::command(uint8_t cmd)
{
    rsLow();
    rwLow();

    send4(static_cast<uint8_t>(cmd >> 4U));
    send4(static_cast<uint8_t>(cmd & 0x0FU));

    if (cmd <= 0x02U) {
        Delay_ms(2U);
    } else {
        Delay_us(50U);
    }
}

void LcdDisplay::data(uint8_t value)
{
    rsHigh();
    rwLow();

    send4(static_cast<uint8_t>(value >> 4U));
    send4(static_cast<uint8_t>(value & 0x0FU));

    Delay_us(50U);
}

uint32_t LcdDisplay::pow10(uint8_t exp)
{
    uint32_t result = 1U;
    while (exp > 0U) {
        result *= 10U;
        --exp;
    }
    return result;
}

void LcdDisplay::initialize()
{
    Delay_ms(60U);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    /* MODER: output */
    GPIOC->MODER &= ~GPIOC_LCD_PINS_MODER_MASK;
    GPIOC->MODER |=  GPIOC_LCD_PINS_OUTPUT_MODE;

    GPIOB->MODER &= ~GPIOB_LCD_PIN_MODER_MASK;
    GPIOB->MODER |=  GPIOB_LCD_PIN_OUTPUT_MODE;

    /* OTYPER: push-pull */
    GPIOC->OTYPER &= ~GPIOC_LCD_PINS_OTYPER_MASK;
    GPIOB->OTYPER &= ~GPIOB_LCD_PIN_OTYPER_MASK;

    /* OSPEEDR: low speed */
    GPIOC->OSPEEDR &= ~GPIOC_LCD_PINS_MODER_MASK;
    GPIOB->OSPEEDR &= ~GPIOB_LCD_PIN_MODER_MASK;

    /* PUPDR: no pull-up, no pull-down */
    GPIOC->PUPDR &= ~GPIOC_LCD_PINS_PUPDR_MASK;
    GPIOB->PUPDR &= ~GPIOB_LCD_PIN_PUPDR_MASK;

    rwLow();
    rsLow();
    eLow();
    backlightHigh();

    /* 4-bit initialization sequence */
    send4(0x03U);
    Delay_ms(5U);

    send4(0x03U);
    Delay_us(200U);

    send4(0x03U);
    Delay_us(200U);

    send4(0x02U);
    Delay_ms(2U);

    command(0x28U); /* 4-bit, 2 lines, 5x8 */
    command(0x08U); /* display off */
    command(0x01U); /* clear */
    Delay_ms(2U);
    command(0x06U); /* entry mode */
    command(0x0CU); /* display on, cursor off, blink off */

    Delay_ms(2U);
}

void LcdDisplay::clear()
{
    command(0x01U);
    Delay_ms(2U);
}

void LcdDisplay::setCursor(uint8_t row, uint8_t column)
{
    uint8_t address = 0x80U;

    if (row == 0U) {
        address = static_cast<uint8_t>(0x80U + column);
    } else {
        address = static_cast<uint8_t>(0xC0U + column);
    }

    command(address);
}

void LcdDisplay::writeChar(char c)
{
    data(static_cast<uint8_t>(c));
}

void LcdDisplay::writeString(const char* text)
{
    if (text == nullptr) {
        return;
    }

    while (*text != '\0') {
        data(static_cast<uint8_t>(*text));
        ++text;
    }
}

void LcdDisplay::createChar(uint8_t location, const uint8_t* charmap)
{
    if (charmap == nullptr) {
        return;
    }

    location &= 0x07U;
    command(static_cast<uint8_t>(0x40U | (location << 3U)));

    for (uint8_t i = 0U; i < 8U; ++i) {
        data(charmap[i]);
    }
}

void LcdDisplay::writeFixed(int32_t value, uint8_t fracDigits)
{
    uint8_t index = 0U;
    uint32_t absValue = 0U;

    if (fracDigits > 9U) {
        fracDigits = 9U;
    }

    if (value < 0) {
        if (index < (kBufferSize - 1U)) {
            finalBuffer_[index++] = '-';
        }
        absValue = static_cast<uint32_t>(-(value + 1)) + 1U;
    } else {
        absValue = static_cast<uint32_t>(value);
    }

    const uint32_t divisor = pow10(fracDigits);
    uint32_t integerPart = absValue / divisor;
    uint32_t fractionalPart = absValue % divisor;

    char integerDigits[12];
    int8_t integerIndex = 0;

    if (integerPart == 0U) {
        integerDigits[integerIndex++] = '0';
    } else {
        while ((integerPart > 0U) && (integerIndex < static_cast<int8_t>(sizeof(integerDigits)))) {
            integerDigits[integerIndex++] = static_cast<char>((integerPart % 10U) + '0');
            integerPart /= 10U;
        }
    }

    while ((integerIndex > 0) && (index < (kBufferSize - 1U))) {
        finalBuffer_[index++] = integerDigits[--integerIndex];
    }

    if ((fracDigits > 0U) && (index < (kBufferSize - 1U))) {
        finalBuffer_[index++] = '.';

        uint32_t tempDiv = divisor / 10U;
        while ((tempDiv > 0U) && (index < (kBufferSize - 1U))) {
            finalBuffer_[index++] = static_cast<char>((fractionalPart / tempDiv) + '0');
            fractionalPart %= tempDiv;
            tempDiv /= 10U;
        }
    }

    finalBuffer_[index] = '\0';
    writeString(finalBuffer_);
}

void LcdDisplay::test()
{
    setCursor(0U, 0U);
    writeString("HELLO");

    setCursor(1U, 0U);
    writeString("STM32");
}