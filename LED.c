#include <string.h>
#include "stm32g0xx.h"

#include "System definitions.h"
#include "System.h"

#include "LED.h"

static BOOL LED_Initialized;

static sLED_Pattern LED_Pattern;

static U32 LED_MsCounter;
static U32 LED_PatternBitNb;

const sLED_Pattern LED_RESET                ={0x00000002, 2,1000};  //
const sLED_Pattern LED_ENUMERATING          ={0x00000002, 2, 100};
const sLED_Pattern LED_CONFIGURED           ={0x00000001, 1, 100};


void LED_MsIRQHandler(void)
{
	if (!LED_Initialized)
		return;
	LED_MsCounter++;
	if (LED_MsCounter<LED_Pattern.MsPerPatternBit)
		return;
	LED_MsCounter=0;
	LED_PatternBitNb++;
	if (LED_PatternBitNb>=LED_Pattern.NbPatternBits)
		LED_PatternBitNb=0;
	if (LED_Pattern.Pattern&(1<<LED_PatternBitNb))
		GPIOA->BSRR=1<<(16+12);
	else
		GPIOA->BSRR=1<<( 0+12);
	return;
}

void LED_Init(void)
{
	// Set PA12 as output
	GPIOA->BSRR=1<<12;
	GPIOA->MODER=(GPIOA->MODER&~(3UL<<(12*2))) | (1<<(12*2));
	LED_Initialized=TRUE;
	LED_SetPattern(&LED_RESET);
	return;
}

void LED_SetPattern(const sLED_Pattern *pPattern)
{
	if (!memcmp(&LED_Pattern,pPattern,sizeof(sLED_Pattern)))
		return;
	LED_Pattern=*pPattern;
	LED_MsCounter=0;
	LED_PatternBitNb=pPattern->NbPatternBits-1;
	LED_MsCounter=pPattern->MsPerPatternBit-1;
	return;
}
