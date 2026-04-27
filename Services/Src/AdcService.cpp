#include "AdcService.hpp"

extern "C" {
#include "stm32f410rx.h"
#include "appcons.h"
#include "apptdef.h"
}

volatile uint16_t AdcService::adcBuffer_[2] = {0U, 0U};

AdcService::AdcService(ModbusService& modbus)
    : modbus_(modbus)
{
}

void AdcService::initialize()
{
    configureGpio();
    configureDma();
    configureAdc();
}

void AdcService::configureGpio()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    /* PA3, PA4 analog mode */
    GPIOA->MODER |= (3U << (3U * 2U)) |
                    (3U << (4U * 2U));

    /* No pull-up / pull-down */
    GPIOA->PUPDR &= ~((3U << (3U * 2U)) |
                      (3U << (4U * 2U)));
}

void AdcService::configureDma()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    (void)RCC->AHB1ENR;

    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    while ((DMA2_Stream0->CR & DMA_SxCR_EN) != 0U) {
    }

    DMA2->LIFCR = DMA_LIFCR_CTCIF0 |
                  DMA_LIFCR_CHTIF0 |
                  DMA_LIFCR_CTEIF0 |
                  DMA_LIFCR_CDMEIF0 |
                  DMA_LIFCR_CFEIF0;

    DMA2_Stream0->PAR  = reinterpret_cast<uint32_t>(&ADC1->DR);
    DMA2_Stream0->M0AR = reinterpret_cast<uint32_t>(adcBuffer_);
    DMA2_Stream0->NDTR = 2U;

    DMA2_Stream0->CR =
          (0U << 25U) |        /* Channel 0 */
          DMA_SxCR_MINC |      /* Memory increment */
          DMA_SxCR_CIRC |      /* Circular mode */
          DMA_SxCR_PSIZE_0 |   /* 16-bit peripheral */
          DMA_SxCR_MSIZE_0 |   /* 16-bit memory */
          DMA_SxCR_PL_1;       /* High priority */

    DMA2_Stream0->CR |= DMA_SxCR_EN;
}

void AdcService::configureAdc()
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    /*
     * Scan mode:
     * Sequence: CH3, CH4
     */
    ADC1->CR1 = ADC_CR1_SCAN;

    ADC1->CR2 =
          ADC_CR2_DMA |
          ADC_CR2_DDS |
          ADC_CR2_CONT;

    /*
     * Sampling time:
     * CH3 -> SMPR2 bits [11:9]
     * CH4 -> SMPR2 bits [14:12]
     * 111 = 480 cycles
     */
    ADC1->SMPR2 &= ~((7U << 9U) | (7U << 12U));
    ADC1->SMPR2 |=  ((7U << 9U) | (7U << 12U));

    /*
     * Sequence length = 2 conversions
     * L = 1 means 2 conversions
     */
    ADC1->SQR1 = (1U << 20U);

    /*
     * SQ1 = CH3
     * SQ2 = CH4
     */
    ADC1->SQR3 =
          (3U << 0U) |
          (4U << 5U);

    ADC1->CR2 |= ADC_CR2_ADON;

    for (volatile uint32_t i = 0U; i < 1000U; ++i) {
    }

    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void AdcService::process()
{
    const uint16_t rawCh3 = adcBuffer_[0];
    const uint16_t rawCh4 = adcBuffer_[1];

    (void)rawCh3;

    /*
     * PA4 = supply voltage measurement
     * C code:
     * v_supply = raw_ch4 * 0.008864468f
     * v_modbus = v_supply * 10
     */
    const float vSupply = static_cast<float>(rawCh4) * VoltageScale;
    const uint16_t vModbus = static_cast<uint16_t>(vSupply * 10.0f);

    modbus_.setHoldingRegister(REG_IN_VOL_VALUE, vModbus);
}

uint16_t AdcService::rawChannel3() const
{
    return adcBuffer_[0];
}

uint16_t AdcService::rawChannel4() const
{
    return adcBuffer_[1];
}