#if !defined(SYSTEM_H)
#define SYSTEM_H

#include "System definitions.h"

#define SYSCLK  64000000
#define SYSTICK (SYSCLK/8) // 8MHz

#define UsToSysTick(us) (us*(SYSCLK/(1000000/32)))

#define IRQ_PRI_LOW      3
#define IRQ_PRI_MEDIUM   2
#define IRQ_PRI_HIGH     1
#define IRQ_PRI_REALTIME 0

extern volatile U32 SYS_SysTickMs;

#define GET_TICK_1MS()     (SYS_SysTickMs)


void SystemInit(void);
void SYS_DelayUs(U32 Delay_us);
U32 SYS_GetSimpleRandomNumber(void);
U32 SYS_GetSimpleRandomNumberFromSeed(U32 Seed);
void SYS_Init(void);
void SYS_StartWatchdog(void);
void SYS_ResetToBootloader(void);
void SYS_ResetWatchdog(void);

#endif // !defined(SYSTEM_H)
