#include "stm32g0xx.h"

#include "System definitions.h"

#include "LED.h"

#include "System.h"

volatile U32 SYS_SysTickMs;

static U32 SYS_RandomNumberSeed=123456789;

void TIM6_IRQHandler(void);

void TIM6_IRQHandler(void)
{
	if (TIM6->SR&TIM_SR_UIF)
	{
		TIM6->SR=~TIM_SR_UIF;
		SYS_SysTickMs++;
		LED_MsIRQHandler();
	}
	return;
}

void SystemInit(void)
{
	return;
}

void SYS_DelayUs(U32 Delay_us)
{
	U32 DelaySysTick;
	U32 StartSysTick;

	StartSysTick=SysTick->VAL<<8;
	DelaySysTick=UsToSysTick(Delay_us);
	while (StartSysTick-(SysTick->VAL<<8)<DelaySysTick);
	return;
}

U32 SYS_GetSimpleRandomNumber(void)
{
	SYS_RandomNumberSeed=1103515245*SYS_RandomNumberSeed+12345;
	return SYS_RandomNumberSeed;
}

U32 SYS_GetSimpleRandomNumberFromSeed(U32 Seed)
{
    SYS_RandomNumberSeed = Seed;
	return 1103515245*Seed+12345;
}
void SYS_Init(void)
{
	SYS_SysTickMs=0;

	// Enable the SysTick counter and set the maximum reload value
	SysTick->LOAD=0xFFFFFF;
	// SysTick SYSCLK/8=6M
	SysTick->CTRL|=1<<SysTick_CTRL_ENABLE_Pos;

	// Enable the SYSCFG clock
	RCC->APBENR2|=RCC_APBENR2_SYSCFGEN;
	// Enable the power interface clock
	RCC->APBENR1|=RCC_APBENR1_PWREN;

	// LATENCY=2 Two wait state
	// PRFTEN=1 Prefetch enabled
	// ICEN=1 CPU Instruction cache enabled
	// ICRST=0 CPU Instruction cache not reset
	// EMPTY=0 Main Flash memory area programmed
	FLASH->ACR=(FLASH->ACR & ~(FLASH_ACR_LATENCY_Msk | FLASH_ACR_PRFTEN_Msk | FLASH_ACR_ICEN_Msk | FLASH_ACR_ICRST_Msk | FLASH_ACR_PROGEMPTY_Msk)) |
		(FLASH_ACR_LATENCY_0*2) | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN;
	
	// Enable HSE clock
	RCC->CR|=RCC_CR_HSEON;
	// Wait for the HSE clock to be stable
	while (!(RCC->CR&RCC_CR_HSERDY));
	// Set PLL Clock (12(HSE)*16 MHz) to System Clock
	// Disable the PLL
	RCC->CR&=~RCC_CR_PLLON;
	// Wait for the PLL to be disabled
	while (RCC->CR&RCC_CR_PLLRDY);
	// PLLSRC=3 PLL input clock source is HSE
	// PLLM=0 Division factor of PLL input clock divider 1 (12MHz)
	// PLLN=16 PLL frequency multiplication factor 16 (192MHz)
	// PLLPEN=0 PLLPCLK clock output disabled
	// PLLP=2 PLL VCO division factor P for PLLPCLK clock output 3 (64MHz)
	// PLLREN=0 PLLURCLK clock output disabled
	// PLLR=2 PLL VCO division factor R for PLLPCLK clock output 3 (64MHz)
	RCC->PLLCFGR=(RCC_PLLCFGR_PLLSRC_0*3) | (RCC_PLLCFGR_PLLM_0*0) | (RCC_PLLCFGR_PLLN_0*16) | (RCC_PLLCFGR_PLLP_0*2) | (RCC_PLLCFGR_PLLR_0*2);
	// SW=0 HSISYS selected as system clock (default)
	// SWS read only
	// HPRE=0 AHB prescaler 1 HCLK=SYSCLK
	// PPRE=0 APB prescaler 1 PCLK=SYSCLK
	// MCOSEL=0 Microcontroller clock output disabled
	// MCOPRE=0 Microcontroller clock output prescaler 1
	RCC->CFGR=0;
	// Turn on the PLL
	RCC->CR|=RCC_CR_PLLON;
	// Enable the PLLR output
	RCC->PLLCFGR|=RCC_PLLCFGR_PLLREN;
	// Wait for the PLL to be ready (locked)
	while (!(RCC->CR&RCC_CR_PLLRDY));
	// Set the PLL output as system clock source
	// SW=2 PLLRCLK selected as system clock
	RCC->CFGR=(RCC->CFGR & ~RCC_CFGR_SW) | (RCC_CFGR_SW_0*2);
	// Wait for the PLL to have been selected as system clock source
	while ((RCC->CFGR & RCC_CFGR_SWS)!=RCC_CFGR_SWS_0*2);

	// Enable IO ports A, B, C, D and F clocks
	RCC->IOPENR|=RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN | RCC_IOPENR_GPIOFEN;
	// Disable the pull-downs on PB15, PA8 and PD0, PD2
	SYSCFG->CFGR1=SYSCFG_CFGR1_UCPD1_STROBE|SYSCFG_CFGR1_UCPD2_STROBE;
	// Enable the DMA and DMAMUX clocks
	RCC->AHBENR|=RCC_AHBENR_DMA1EN;

#if defined(DEBUG)
	// Enable the DBGMCU module clock	
	RCC->APBENR1|=RCC_APBENR1_DBGEN;
#endif

	// Set TIM6 to be a 1ms periodic interrupt
	// Enable TIM6 clock
	RCC->APBENR1|=RCC_APBENR1_TIM6EN;
	// Reset TIM6
	RCC->APBRSTR1|=RCC_APBRSTR1_TIM6RST;
	RCC->APBRSTR1&=~RCC_APBRSTR1_TIM6RST;
	// CEN=0 counter disabled
	// UDIS=0 Update enabled
	// URS=1 Only counter overflow/underflow generates an update interrupt 
	// OPM=0 Counter is not stopped at update event
	// ARPE=0 Auto-reload preload disnabled
	// UIFREMAP=0 No remapping. UIF status bit is not copied to TIMx_CNT register bit 31
	TIM6->CR1=TIM_CR1_URS;
	// UIE=1 Update interrupt enable
	// UDE=0 Update DMA request disabled
	TIM6->DIER=TIM_DIER_UIE;
	// Prescaler=1MHz timer counter clock
	TIM6->PSC=SYSCLK/1000000-1;
	// Generate an update event to update the prescaler
	TIM6->EGR=TIM_EGR_UG;
	// Auto reload register set to 999 (counter overflows every 1ms)
	TIM6->ARR=1000-1;
	// Enable the timer counter
	TIM6->CR1|=TIM_CR1_CEN;
	// Set the timer 6 IRQ to low priority and enable it
	NVIC_SetPriority(TIM6_IRQn,IRQ_PRI_LOW);
	NVIC_EnableIRQ(TIM6_IRQn);
	// Trigger an event
	TIM6->EGR|=TIM_EGR_UG;

#if !defined(DEBUG)
	// Enable the watchdog
	// LSI RC oscillator frequency varies from 30 to 50KHz, set the watchdog on a 50KHz calculation for safety
	// The desired watchdog minimum trigger time is 10ms
	// Enable register access
	IWDG->KR=0x5555;
	// Set the prescaler to 4 (50KHz/4=12500Hz)
	IWDG->PR=0;
	// Set the reload register to 2500 (200ms timeout)
	IWDG->RLR=2500;
#endif

#if defined(DEBUG)
	// Freeze everything when debug stop
	DBG->APBFZ1=DBG_APB_FZ1_DBG_TIM3_STOP | DBG_APB_FZ1_DBG_TIM6_STOP | DBG_APB_FZ1_DBG_TIM7_STOP | DBG_APB_FZ1_DBG_RTC_STOP |
		DBG_APB_FZ1_DBG_WWDG_STOP | DBG_APB_FZ1_DBG_IWDG_STOP | DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP;
	DBG->APBFZ2=DBG_APB_FZ2_DBG_TIM1_STOP | DBG_APB_FZ2_DBG_TIM14_STOP | DBG_APB_FZ2_DBG_TIM15_STOP |
		DBG_APB_FZ2_DBG_TIM16_STOP | DBG_APB_FZ2_DBG_TIM17_STOP;
#endif

	return;
}

void SYS_ResetWatchdog(void)
{
#if !defined(DEBUG)
	IWDG->KR=0xAAAA;
#endif
	return;
}
void SYS_StartWatchdog(void)
{
#if !defined(DEBUG)
	IWDG->KR=0xCCCC;
#endif
	return;
}
