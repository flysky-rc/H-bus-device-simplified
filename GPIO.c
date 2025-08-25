#include "stm32g0xx.h"

#include "System definitions.h"
#include "System.h"

#include "GPIO.h"

static tGPIO_Callback *pGPIO_PA1Callback;

void EXTI0_1_IRQHandler(void)
{
	if (EXTI->FPR1&(1<<1))
	{
		EXTI->FPR1=1<<1;
		if (pGPIO_PA1Callback)
			pGPIO_PA1Callback();
		return;
	}
	return;
}

void GPIO_DisablePA1Interrupt(void)
{
	// Mask interrupt request from line 1
	EXTI->IMR1&=~(1<<1);
	EXTI->FPR1=1<<1;
	return;
}

void GPIO_EnablePA1Interrupt(tGPIO_Callback *pCallback)
{
	pGPIO_PA1Callback=pCallback;
	EXTI->FPR1=1<<1;
	// Unmask interrupt request from line 1
	EXTI->IMR1|=1<<1;
	return;
}

void GPIO_Init(void)
{
	// Set an interrupt on PA1 rising and falling
	// Enable external interrupt 1 on port A
	EXTI->EXTICR[1/4]=(EXTI->EXTICR[1/4]&0xFFFF00FF) | 0x00000000;
	// Enable rising trigger from input line 1
	EXTI->RTSR1|=1<<1;
	// Enable falling trigger from input line 1
	EXTI->FTSR1|=1<<1;
	// Set the EXTI0_1 IRQ to medium priority and enable it
	NVIC_SetPriority(EXTI0_1_IRQn,IRQ_PRI_MEDIUM);
	NVIC_EnableIRQ(EXTI0_1_IRQn);
	return;
}
