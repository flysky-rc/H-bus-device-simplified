#if !defined(NPT_TIMER_H)
#define NPT_TIMER_H

#include "System definitions.h"

typedef void tHB_TIM_TimerCompareIRQHandler(void);

void HB_TIM_EnableChannel(BOOL InitialOutputLevel,
	tHB_TIM_TimerCompareIRQHandler *pTimerCompareIRQHandler);
void HB_TIM_FreezeCompareIRQ(void);
void HB_TIM_Init(void);
// The IRQ compare must be frozen before calling this function
void HB_TIM_SetNextCompareIRQTimeFromNow(U32 Time,tHB_TIM_TimerCompareIRQHandler *pTimerCompareIRQHandler);

#endif // !defined(NPT_TIMER_H)
