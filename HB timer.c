#include "stm32g0xx.h"
#include <string.h>

#include "System definitions.h"
#include "System.h"

#include "HB timer.h"

#define TIM_CCMR_OCM_Msk (TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)

static BOOL HB_TIM_Initialized;

static U32 HB_TIM_TimerCounter;
static U32 HB_TIM_RemainingTime;
static tHB_TIM_TimerCompareIRQHandler *pHB_TIM_TimerCompareIRQHandler;

static void HB_TIM_SetNextCompareIRQTimeInternal(void);


void TIM16_IRQHandler(void)
{
	if (TIM16->SR & TIM_SR_CC1IF)
	{
		TIM16->SR&=~TIM_SR_CC1IF;
		if (HB_TIM_RemainingTime==0)
		{
			if (pHB_TIM_TimerCompareIRQHandler)
				pHB_TIM_TimerCompareIRQHandler();
		}
		else
			HB_TIM_SetNextCompareIRQTimeInternal();
	}
	return;
}

void HB_TIM_FreezeCompareIRQ(void)
{
	TIM16->DIER&=~TIM_DIER_CC1IE;
	TIM16->SR&=~TIM_SR_CC1IF;
	return;
}

void HB_TIM_Init(void)
{
	if (HB_TIM_Initialized)
		return;
	// Enable and reset TIM16
	RCC->APBENR2|=RCC_APBENR2_TIM16EN;
	RCC->APBRSTR2|=RCC_APBRSTR2_TIM16RST;
	RCC->APBRSTR2&=~RCC_APBRSTR2_TIM16RST;

	// Enable channel 1
	// CC1E=1 Capture/Compare 1 output enabled
	// CC1P=0 Capture/Compare 1 output active high
	// CC1NE=0 OC1N output not active
	// CC1NP=0 Not used
	TIM16->CCER=TIM_CCER_CC1E;
	// Set output compare values of all channels to zero
	TIM16->CCR1=0;
	// Set a 1MHz counter
	TIM16->PSC=SYSCLK/1000000-1;
	// CC1S=0 CC1 channel is configured as output
	// OC1FE=0 Output Compare 1 fast disbled
	// OC1PE=0 Output Compare 1 preload disabled
	// OC1M=0 Frozen
	TIM16->CCMR1=(TIM_CCMR1_OC1M_0*0);
	// Set a full 16-bit counter
	TIM16->ARR=0xFFFF;
	// Enable outputs
	TIM16->BDTR|=TIM_BDTR_MOE;
	// Set the timer TIM16 IRQ to medium priority
	NVIC_SetPriority(TIM16_IRQn,IRQ_PRI_MEDIUM);
	// Enable the timer TIM16 IRQ
	NVIC_EnableIRQ(TIM16_IRQn);
	TIM16->CR1=TIM_CR1_CEN;

	// Start timer
	// Reinitialize the counter and generates an update of the registers
	TIM16->EGR=TIM_EGR_UG;
	// Enable the counter of the timer and keep the defaults
	TIM16->CR1=TIM_CR1_CEN;

	HB_TIM_Initialized=TRUE;
	return;
}

void HB_TIM_SetNextCompareIRQTimeFromNow(U32 Time,tHB_TIM_TimerCompareIRQHandler *pTimerCompareIRQHandler)
{
	HB_TIM_RemainingTime=Time;
	pHB_TIM_TimerCompareIRQHandler=pTimerCompareIRQHandler;
	TIM16->SR&=~TIM_SR_CC1IF;
	TIM16->DIER=TIM_DIER_CC1IE;
	HB_TIM_TimerCounter=TIM16->CNT;
	HB_TIM_SetNextCompareIRQTimeInternal();
	return;
}

static void HB_TIM_SetNextCompareIRQTimeInternal(void)
{
	U32 RemainingTime;
	U32 TimerCounter;

	RemainingTime=HB_TIM_RemainingTime;
	TimerCounter=HB_TIM_TimerCounter;
	if (RemainingTime<=0x10000)
	{
		TimerCounter=(TimerCounter+RemainingTime)&0xFFFF;
		RemainingTime=0;
	}
	else if (RemainingTime<=0x20000)
	{
		U32 NextTime;
		
		NextTime=RemainingTime/2;
		TimerCounter=(TimerCounter+NextTime)&0xFFFF;
		RemainingTime-=NextTime;
	}
	else
		RemainingTime-=0x10000;
	HB_TIM_RemainingTime=RemainingTime;
	HB_TIM_TimerCounter=TimerCounter;
	TIM16->CCR1=TimerCounter;
	return;
}
