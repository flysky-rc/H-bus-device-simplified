#include "stm32g0xx.h"

#include "System definitions.h"
#include "System.h"

#include "UART.h"

#include "HB UART.h"

#include "protocol_slip.h"
#include "Main.h"

#define HB_UART_CR3_INITIAL_VALUE (USART_CR3_ONEBIT | USART_CR3_OVRDIS)

static tHB_UART_TransmitCallback *pHB_UART_TransmitCallback;
static tHB_UART_ReceiveCallback *pHB_UART_ReceiveCallback;
static unsigned long HB_UART_ReceiveDataMaxLength;
static unsigned long HB_UART_CR1InitialValue;

static BOOL HB_UART_IDLEInterruptEnabled;
static BOOL HB_UART_Transmitting;

const unsigned char HB_UART_ReceiveTimeDelay=9; // In microseconds
const unsigned char HB_UART_TransmitTimeDelay=9; // In microseconds
const unsigned char HB_UART_TransmitCompleteTimeDelay=15; // In microseconds

void USART3_4_IRQHandler(void);
static void UART_IDLEIRQHandler();
static void UART_RTOIRQHandler();
static void UART_TCIRQHandler();

void USART3_4_IRQHandler(void)
{
	if (USART4->ISR&USART_ISR_IDLE)
	{
		// Clear the IDLE flag
		USART4->ICR=USART_ICR_IDLECF;
		UART_IDLEIRQHandler();
	}
	if (USART4->ISR&USART_ISR_TC)
	{
		// Clear the Transmit complete flag
		USART4->ICR=USART_ICR_TCCF;
		UART_TCIRQHandler();
	}
	return;
}

static void UART_IDLEIRQHandler()
{
	tHB_UART_ReceiveCallback *pCallback;

	if (!HB_UART_IDLEInterruptEnabled)
		return;
	pCallback=pHB_UART_ReceiveCallback;
	if (pCallback)
	{
		unsigned long NbReceivedBytes;
		
		NbReceivedBytes=HB_UART_ReceiveDataMaxLength-DMA1_Channel4->CNDTR;
		if (NbReceivedBytes)
		{
			HB_UART_IDLEInterruptEnabled=FALSE;
			// RE=0 Receiver disabled
			// TE=0 Transmitter disabled
			// TCIE=0 Transmission complete interrupt disabled
			// RTOIE=0 Receiver timeout interrupt disabled
			USART4->CR1=HB_UART_CR1InitialValue;
			// Disable TX and RX DMA
			DMA1_Channel4->CCR=0;
			DMA1_Channel4->CCR=0;
			pHB_UART_ReceiveCallback=NULL;
			pCallback(NbReceivedBytes);
		}
	}
	return;
}

static void UART_TCIRQHandler(void)
{
	tHB_UART_TransmitCallback *pCallback;

	// Clear the Transmit complete flag
	USART4->ICR=USART_ICR_TCCF;
	if (!HB_UART_Transmitting)
		return;
	HB_UART_Transmitting=FALSE;
	// Set the TX GPIO as input
	GPIOA->MODER=(GPIOA->MODER & ~(3<<0)) | (0<<0);
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// TCIE=0 Transmission complete interrupt disabled
	// RTOIE=0 Receiver timeout interrupt disabled
	USART4->CR1=HB_UART_CR1InitialValue;
	// Disable TX and RX DMA
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	pCallback=pHB_UART_TransmitCallback;
	if (pCallback)
	{
		pHB_UART_TransmitCallback=NULL;
		pCallback();
	}
	return;
}

unsigned long HB_UART_GetNbReceivedBytes()
{
	unsigned long CNDTR;

	CNDTR=DMA1_Channel4->CNDTR;
	if (HB_UART_ReceiveDataMaxLength>CNDTR)
		return HB_UART_ReceiveDataMaxLength-CNDTR;
	else
		return 0;
}

// May be called several times
void HB_UART_Init(const sUART_Config *pUARTConfig)
{
	pHB_UART_TransmitCallback=NULL;
	pHB_UART_ReceiveCallback=NULL;
	// Disable the USART IRQ
	NVIC_DisableIRQ(USART3_4_IRQn);
	HB_UART_IDLEInterruptEnabled=FALSE;
	SYS_DisableIRQs();
	// Set the TX GPIO output level in advance
	GPIOA->BSRR=((!pUARTConfig->Inverted)?0:16)<<(0/2);
	// Set the TX GPIO alternate function number (USART_TX)
	GPIOA->AFR[0/16]=(GPIOA->AFR[0/16] & ~(0xF<<((0*2)%32))) |
		(4<<((0*2)%32));
	// Set the TX GPIO pull-up or pull-down depending on USART inversion
	GPIOA->PUPDR=(GPIOA->PUPDR & ~(3UL<<0)) | (((!pUARTConfig->Inverted)?1:2)<<0);
	// Set the TX GPIO as input
	GPIOA->MODER=(GPIOA->MODER & ~(3UL<<0)) | (0<<0);
	// Set the RX GPIO alternate function number (USART_RX)
	GPIOA->AFR[0]=(GPIOA->AFR[0] & ~(0xF<<(4))) | (4<<4);
	// Set the RX GPIO as alternate function
	GPIOA->MODER=(GPIOA->MODER & ~(3UL<<2)) | (2<<2);
	SYS_EnableIRQs();
	// Enable and reset the USART module clock
		RCC->APBENR1|=RCC_APBENR1_USART4EN;
		RCC->APBRSTR1|=RCC_APBRSTR1_USART4RST;
		RCC->APBRSTR1&=~RCC_APBRSTR1_USART4RST;
	// Disable TX and RX DMA
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	// Clear the transfer complete flag
	USART4->ICR=USART_ICR_TCCF;
	HB_UART_ReceiveDataMaxLength=0;
	pHB_UART_TransmitCallback=NULL;
	pHB_UART_ReceiveCallback=NULL;
	// UE=0 USART disabled
	// UESM=0 USART not able to wake up the MCU from low-power mode
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// IDLEIE=1 IDLE interrupt enabled for USART3 and USART4
	// RXNEIE=0 Receive data register not empty interrupt disabled
	// TCIE=0 Transmission complete interrupt disabled
	// TXEIE=0 TX empty interrupt disabled
	// PEIE=0 Parity error interrupt disabled
	// PS=0 or 1 depending on Parity
	// PCE=0 or 1 Parity control enabled or disabled depending on Parity
	// WAKE=0 Not used
	// M0=0 or 1 8 or 9 bit word length depending on parity
	// MME=0 Mute mode disabled
	// CMIE=0  Character match interrupt disabled
	// OVER8=0 Oversampling by 16
	// DEDT=0 Driver Enable deassertion time is null
	// DEAT=0 Driver Enable assertion time is null
	// RTOIE=0 Receiver timeout interrupt disabled
	// EOBIE=0 End of Block interrupt disabled
	// M1=0 1 start, 8 or 9 data bits
	// FIFOEN=0 FIFO mode is disabled
	HB_UART_CR1InitialValue=((pUARTConfig->Parity!=UART_PARITY_NONE)?USART_CR1_PCE:0) |
		((pUARTConfig->Parity==UART_PARITY_ODD)?USART_CR1_PS:0) |
		((pUARTConfig->Parity==UART_PARITY_NONE)?0:USART_CR1_M0);
	HB_UART_CR1InitialValue|=USART_CR1_IDLEIE;
	USART4->CR1=HB_UART_CR1InitialValue;
	{
		unsigned long CR2;

		// SLVEN=0 Slave mode disabled
		// DIS_NSS=0 SPI slave selection depends on NSS input pin
		// ADDM7=0 Not used
		// LBDL=0 LIN break detection not used
		// LBDIE=0 LIN break detection interrupt disabled
		// LBCL=0 Not used
		// CPHA=0 Not used
		// CPOL=0 Not used
		// CLKEN=0 SCLK pin disabled
		// STOP depends on StopBit
		// LINEN=0 LIN mode disabled
		// SWAP=0 RX and TX lines not swapped
		// RXINV depends on IsInverted
		// TXINV depends on IsInverted
		// DATAINV=0 Data not inverted
		// MSBFIRST=0 LSB first
		// ABREN=0 Auto baud rate disabled
		// ABRMOD=0 Not used
		// RTOEN=1 Receiver timeout feature enabled
		// ADD=0 Not used
		CR2=((pUARTConfig->StopBits==UART_STOPBIT_1)?0:USART_CR2_STOP_1) |
			((pUARTConfig->StopBits==UART_STOPBIT_1_5)?USART_CR2_STOP_0:0) |
			((pUARTConfig->Inverted)?USART_CR2_RXINV:0) |
			((pUARTConfig->Inverted)?USART_CR2_TXINV:0);
		// Receive timeout after 10 data bits
		if (3<=1)
		{
			CR2|=USART_CR2_RTOEN;
			USART4->RTOR=10;
		}
		USART4->CR2=CR2;
	}
	USART4->CR3=HB_UART_CR3_INITIAL_VALUE;
	// Set the baud rate
	USART4->BRR=(pUARTConfig->BitDuration*((SYSCLK/1000000)*(1<<16)/500)+(1<<15))>>16;
	// Enable the USART
	HB_UART_CR1InitialValue|=USART_CR1_UE;
	USART4->CR1=HB_UART_CR1InitialValue;
	// Clear the IDLE, the transmission complete and the receiver timeout flags
	USART4->ICR=USART_ICR_IDLECF | USART_ICR_TCCF | USART_ICR_RTOCF;
	// Clear the USART IRQ pending flag
	NVIC_ClearPendingIRQ(USART3_4_IRQn);
	// Set the USART IRQ to medium priority
	NVIC_SetPriority(USART3_4_IRQn,IRQ_PRI_MEDIUM);
	// Enable the USART IRQ
	NVIC_EnableIRQ(USART3_4_IRQn);
	return;
}

void HB_UART_ReceiveSetup(const void *pData,unsigned long DataLength,BOOL IsCircular,tHB_UART_ReceiveCallback *pCallback)
{
	pHB_UART_ReceiveCallback=pCallback;
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// TCIE=0 Transmission complete interrupt disabled
	// RTOIE=0 Receiver timeout interrupt disabled
	USART4->CR1=HB_UART_CR1InitialValue;
	// Disable TX and RX DMA
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	SYS_DisableIRQs();
	// Set the TX GPIO as input
	GPIOA->MODER=(GPIOA->MODER & ~(3UL<<0)) | (0<<0);
	SYS_EnableIRQs();
	// Set the DMAMUX
	// DMAREQ_ID=x Select the request source peripheral
	// SOIE=0 Synchronization overrun interrupt disabled
	// EGE=0 Event generation disabled
	// SE=0 Synchronization disabled
	// SPOL=0 Not used
	// NBREQ=0 Not used
	// SYNC_ID=0 Not used
	DMAMUX1[3].CCR=56<<DMAMUX_CxCR_DMAREQ_ID_Pos;
	// Set up the RX DMA
	DMA1_Channel4->CPAR=(unsigned long)&USART4->RDR;
	DMA1_Channel4->CMAR=(unsigned long)pData;
	DMA1_Channel4->CNDTR=DataLength;
	HB_UART_ReceiveDataMaxLength=DataLength;
	// EN=1 DMA channel enabled
	// TCIE=0 Transfer complete interrupt disabled
	// HTIE=0 Half transfer interrupt disabled
	// TEIE=0 Transfer error interrupt disabled
	// DIR=0 Read from peripheral
	// CIRC=x Circular mode enabled depending on IsCircular
	// PINC=0 Peripheral increment mode disabled
	// MINC=1 Memory increment mode enabled
	// PSIZE=0 8-bit peripheral size
	// MSIZE=0 8-bit memory size
	// PL=0 Low priority
	// MEM2MEM=0 Memory to memory mode disabled
	if (IsCircular)
	{
		DMA1_Channel4->CCR=             DMA_CCR_CIRC | DMA_CCR_MINC;
		DMA1_Channel4->CCR=DMA_CCR_EN | DMA_CCR_CIRC | DMA_CCR_MINC;
	}
	else
	{
		DMA1_Channel4->CCR=             DMA_CCR_MINC;
		DMA1_Channel4->CCR=DMA_CCR_EN | DMA_CCR_MINC;
	}
	// Clear the reception buffer
	while (USART4->ISR&USART_ISR_RXNE_RXFNE)
		USART4->RDR;
	// Enable RX DMA
	USART4->CR3=HB_UART_CR3_INITIAL_VALUE | USART_CR3_DMAR;
	return;
}

void HB_UART_ReceiveStart()
{
	if (pHB_UART_ReceiveCallback)
		HB_UART_IDLEInterruptEnabled=TRUE;
	// Enable reception
	USART4->CR1=HB_UART_CR1InitialValue | USART_CR1_RE;
	return;
}

void HB_UART_ReceiveStop()
{
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// TCIE=0 Transmission complete interrupt disabled
	// RTOIE=0 Receiver timeout interrupt disabled
	USART4->CR1=HB_UART_CR1InitialValue;
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	return;
}


extern U8 APP_TXBuffer[APP_TX_BUFFER_SIZE];

void HB_UART_TransmitSetup(const void *pData,unsigned long DataLength,tHB_UART_TransmitCallback *pCallback)
{
    
#ifdef SLIP_EN	    
    DataLength = pack_data( pData, DataLength, APP_TXBuffer );
    pData = APP_TXBuffer;
#endif    
	HB_UART_IDLEInterruptEnabled=FALSE;
	pHB_UART_TransmitCallback=pCallback;
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// TCIE=0 Transmission complete interrupt disabled
	// RTOIE=0 Receiver timeout interrupt disabled
	USART4->CR1=HB_UART_CR1InitialValue;
	// Disable TX and RX DMA
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	// Set up the TX DMA
	// Set the DMAMUX
	// DMAREQ_ID=x Select the request source peripheral
	// SOIE=0 Synchronization overrun interrupt disabled
	// EGE=0 Event generation disabled
	// SE=0 Synchronization disabled
	// SPOL=0 Not used
	// NBREQ=0 Not used
	// SYNC_ID=0 Not used
	DMAMUX1[3].CCR=57<<DMAMUX_CxCR_DMAREQ_ID_Pos;
	DMA1_Channel4->CPAR=(unsigned long)&USART4->TDR;
	DMA1_Channel4->CMAR=(unsigned long)pData;
	DMA1_Channel4->CNDTR=DataLength;
	// EN=1 DMA channel enabled
	// TCIE=0 Transfer complete interrupt disabled
	// HTIE=0 Half transfer interrupt disabled
	// TEIE=0 Transfer error interrupt disabled
	// DIR=1 Read from memory
	// CIRC=0 Circular mode disabled
	// PINC=0 Peripheral increment mode disabled
	// MINC=1 Memory increment mode enabled
	// PSIZE=0 8-bit peripheral size
	// MSIZE=0 8-bit memory size
	// PL=0 Low priority
	// MEM2MEM=0 Memory to memory mode disabled
	DMA1_Channel4->CCR=             DMA_CCR_DIR | DMA_CCR_MINC;
	DMA1_Channel4->CCR=DMA_CCR_EN | DMA_CCR_DIR | DMA_CCR_MINC;
	// Enable TX DMA
	USART4->CR3=HB_UART_CR3_INITIAL_VALUE | USART_CR3_DMAT;
	return;
}

void HB_UART_TransmitStart()
{
	HB_UART_Transmitting=TRUE;
	SYS_DisableIRQs();
	// Enable transmission and the transmit complete interrupt
	USART4->CR1=HB_UART_CR1InitialValue | USART_CR1_TE | USART_CR1_TCIE;
	// Set the TX GPIO as alternate function
	GPIOA->MODER=(GPIOA->MODER & ~(3UL<<0)) | (2<<0);
	SYS_EnableIRQs();
	return;
}

void UB_UART_TransmitStop(void)
{
	// Set the TX GPIO as input
	GPIOA->MODER=(GPIOA->MODER & ~(3<<0)) | (0<<0);
	// RE=0 Receiver disabled
	// TE=0 Transmitter disabled
	// TCIE=0 Transmission complete interrupt disabled
	// RTOIE=0 Receiver timeout interrupt disabled
	USART4->CR1=HB_UART_CR1InitialValue;
	// Disable TX and RX DMA
	DMA1_Channel4->CCR=0;
	DMA1_Channel4->CCR=0;
	// Clear the Transmit complete flag
	USART4->ICR=USART_ICR_TCCF;
	HB_UART_Transmitting=FALSE;
	return;
}
