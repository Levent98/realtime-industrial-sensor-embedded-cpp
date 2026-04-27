#include "I2cBus.hpp"

extern "C" {
#include "stm32f410rx.h"
#include "core_cm4.h"
}

namespace {
    static const uint32_t I2C_TIMEOUT_VALUE = 300UL;

    static inline void smallDelay()
    {
        for (volatile uint32_t i = 0U; i < 200U; ++i) {
        }
    }

    static void generateStop()
    {
        I2C1->CR1 |= I2C_CR1_STOP;
    }

    static void clearErrorFlags()
    {
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_AF;
        }
        if ((I2C1->SR1 & I2C_SR1_ARLO) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_ARLO;
        }
        if ((I2C1->SR1 & I2C_SR1_BERR) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_BERR;
        }
        if ((I2C1->SR1 & I2C_SR1_OVR) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_OVR;
        }
    }

    static bool waitSet(volatile uint32_t& reg, uint32_t mask)
    {
        uint32_t timeout = I2C_TIMEOUT_VALUE;

        while ((reg & mask) == 0U) {
            if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
                I2C1->SR1 &= ~I2C_SR1_AF;
                generateStop();
                return false;
            }

            if (timeout-- == 0U) {
                generateStop();
                return false;
            }
        }

        return true;
    }

    static void configurePinsAlternateFunction()
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
        (void)RCC->AHB1ENR;

        GPIOB->MODER &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));
        GPIOB->MODER |=  ((2U << (6U * 2U)) | (2U << (7U * 2U)));

        GPIOB->OTYPER |= (1U << 6U) | (1U << 7U);

        GPIOB->OSPEEDR &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));
        GPIOB->OSPEEDR |=  ((1U << (6U * 2U)) | (1U << (7U * 2U)));

        /* External pull-up var, internal pull-up/down yok */
        GPIOB->PUPDR &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));

        GPIOB->AFR[0] &= ~((0xFU << 24U) | (0xFU << 28U));
        GPIOB->AFR[0] |=  ((4U << 24U) | (4U << 28U));
    }

    static void resetPeripheral()
    {
        RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
        smallDelay();
        RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
        smallDelay();
    }

    static void configurePeripheral24MHz()
    {
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        (void)RCC->APB1ENR;

        I2C1->CR1 &= ~I2C_CR1_PE;

        resetPeripheral();

        I2C1->CR1 = 0U;
        I2C1->CR2 = 0U;
        I2C1->CCR = 0U;
        I2C1->TRISE = 0U;

        I2C1->CR2   = 24U;
        I2C1->CCR   = 120U;
        I2C1->TRISE = 25U;

        I2C1->CR1 |= I2C_CR1_ACK;
        I2C1->CR1 |= I2C_CR1_PE;
    }

    static bool waitTxE()
    {
        uint32_t timeout = I2C_TIMEOUT_VALUE;

        while ((I2C1->SR1 & I2C_SR1_TXE) == 0U) {
            if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
                I2C1->SR1 &= ~I2C_SR1_AF;
                generateStop();
                return false;
            }

            if (timeout-- == 0U) {
                generateStop();
                return false;
            }
        }

        return true;
    }

    static bool waitBtf()
    {
        uint32_t timeout = I2C_TIMEOUT_VALUE;

        while ((I2C1->SR1 & I2C_SR1_BTF) == 0U) {
            if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
                I2C1->SR1 &= ~I2C_SR1_AF;
                generateStop();
                return false;
            }

            if (timeout-- == 0U) {
                generateStop();
                return false;
            }
        }

        return true;
    }

    static bool waitRxNE()
    {
        uint32_t timeout = I2C_TIMEOUT_VALUE;

        while ((I2C1->SR1 & I2C_SR1_RXNE) == 0U) {
            if (timeout-- == 0U) {
                generateStop();
                return false;
            }
        }

        return true;
    }
}

I2cBus::I2cBus()
    : initialized_(false)
{
}

void I2cBus::initialize()
{
    configurePinsAlternateFunction();
    configurePeripheral24MHz();

    if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        recoverBus();
    }

    initialized_ = true;
}

I2cStatus I2cBus::write(uint8_t address, const uint8_t* data, size_t size)
{
    if ((!initialized_) || (data == nullptr) || (size == 0U)) {
        return I2cStatus::InvalidArgument;
    }

    clearErrorFlags();

    if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        recoverBus();

        if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
            return I2cStatus::BusError;
        }
    }

    I2C1->CR1 |= I2C_CR1_START;

    if (!waitSet(I2C1->SR1, I2C_SR1_SB)) {
        return I2cStatus::Timeout;
    }

    I2C1->DR = static_cast<uint8_t>(address << 1U);

    uint32_t timeout = I2C_TIMEOUT_VALUE;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            generateStop();
            return I2cStatus::Nack;
        }

        if (timeout-- == 0U) {
            generateStop();
            return I2cStatus::Timeout;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    for (size_t i = 0U; i < size; ++i) {
        if (!waitTxE()) {
            return I2cStatus::Timeout;
        }

        I2C1->DR = data[i];
    }

    if (!waitBtf()) {
        return I2cStatus::Timeout;
    }

    generateStop();

    return I2cStatus::Ok;
}

I2cStatus I2cBus::read(uint8_t address, uint8_t* data, size_t size)
{
    if ((!initialized_) || (data == nullptr) || (size == 0U)) {
        return I2cStatus::InvalidArgument;
    }

    clearErrorFlags();

    if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        recoverBus();

        if ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
            return I2cStatus::BusError;
        }
    }

    I2C1->CR1 &= ~I2C_CR1_POS;
    I2C1->CR1 |= I2C_CR1_ACK;

    I2C1->CR1 |= I2C_CR1_START;

    if (!waitSet(I2C1->SR1, I2C_SR1_SB)) {
        return I2cStatus::Timeout;
    }

    I2C1->DR = static_cast<uint8_t>((address << 1U) | 1U);

    uint32_t timeout = I2C_TIMEOUT_VALUE;
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U) {
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            generateStop();
            return I2cStatus::Nack;
        }

        if (timeout-- == 0U) {
            generateStop();
            return I2cStatus::Timeout;
        }
    }

    if (size == 1U) {
        I2C1->CR1 &= ~I2C_CR1_ACK;

        __disable_irq();
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        generateStop();
        __enable_irq();

        if (!waitRxNE()) {
            I2C1->CR1 |= I2C_CR1_ACK;
            return I2cStatus::Timeout;
        }

        data[0] = static_cast<uint8_t>(I2C1->DR);
    } else if (size == 2U) {
        I2C1->CR1 |= I2C_CR1_POS;
        I2C1->CR1 &= ~I2C_CR1_ACK;

        __disable_irq();
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        __enable_irq();

        if (!waitBtf()) {
            I2C1->CR1 &= ~I2C_CR1_POS;
            I2C1->CR1 |= I2C_CR1_ACK;
            return I2cStatus::Timeout;
        }

        __disable_irq();
        generateStop();
        data[0] = static_cast<uint8_t>(I2C1->DR);
        __enable_irq();

        data[1] = static_cast<uint8_t>(I2C1->DR);

        I2C1->CR1 &= ~I2C_CR1_POS;
    } else {
        (void)I2C1->SR1;
        (void)I2C1->SR2;

        size_t i = 0U;

        while (i < (size - 3U)) {
            if (!waitRxNE()) {
                I2C1->CR1 |= I2C_CR1_ACK;
                return I2cStatus::Timeout;
            }

            data[i++] = static_cast<uint8_t>(I2C1->DR);
        }

        if (!waitBtf()) {
            I2C1->CR1 |= I2C_CR1_ACK;
            return I2cStatus::Timeout;
        }

        I2C1->CR1 &= ~I2C_CR1_ACK;

        __disable_irq();
        data[i++] = static_cast<uint8_t>(I2C1->DR);
        generateStop();
        __enable_irq();

        data[i++] = static_cast<uint8_t>(I2C1->DR);

        if (!waitRxNE()) {
            I2C1->CR1 |= I2C_CR1_ACK;
            return I2cStatus::Timeout;
        }

        data[i++] = static_cast<uint8_t>(I2C1->DR);
    }

    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 &= ~I2C_CR1_POS;

    return I2cStatus::Ok;
}

I2cStatus I2cBus::writeRead(uint8_t address,
                            const uint8_t* txData,
                            size_t txSize,
                            uint8_t* rxData,
                            size_t rxSize)
{
    const I2cStatus status = write(address, txData, txSize);

    if (status != I2cStatus::Ok) {
        return status;
    }

    return read(address, rxData, rxSize);
}

bool I2cBus::recoverBus()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB1ENR;

    I2C1->CR1 &= ~I2C_CR1_PE;

    GPIOB->MODER &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));
    GPIOB->MODER |=  (1U << (6U * 2U));

    GPIOB->OTYPER |= (1U << 6U) | (1U << 7U);

    /* External pull-up var, internal pull-up/down yok */
    GPIOB->PUPDR &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));

    GPIOB->BSRR = (1U << 6U);
    smallDelay();

    for (uint8_t i = 0U; i < 9U; ++i) {
        GPIOB->BSRR = (1U << (6U + 16U));
        smallDelay();

        GPIOB->BSRR = (1U << 6U);
        smallDelay();
    }

    configurePinsAlternateFunction();
    configurePeripheral24MHz();

    clearErrorFlags();
    initialized_ = true;

    return true;
}