#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void TimConfig(void);
void SysTick_Init(void);
void Delay_us(uint16_t us);
void Delay_ms(uint16_t ms);
uint32_t GetTick(void);
uint32_t GetTime_us(void);

#ifdef __cplusplus
}
#endif

#endif