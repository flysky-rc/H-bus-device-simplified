#if !defined(UART_H)
#define UART_H

#include "System definitions.h"

#define UART_PARITY_NONE 0
#define UART_PARITY_ODD  1
#define UART_PARITY_EVEN 2
#define UART_STOPBIT_1   0
#define UART_STOPBIT_1_5 1
#define UART_STOPBIT_2   2
#define UART_BAUDRATE_TO_BIT_DURATION(BaudRate) ((500000000+BaudRate/2)/BaudRate)
typedef struct __attribute__((packed))
{
	U16 BitDuration; // Duration of one bit on the UART in units of 2ns
	U8 Parity:2;
	U8 StopBits:2;
	U8 Inverted:1;
	U8 StrongDriveTXOnly:1; // Reception not used
	U8 Reserved:2;
} sUART_Config;

#endif // !defined(UART_H)
