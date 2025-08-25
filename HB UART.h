#if !defined(HB_UART_H)
#define HB_UART_H

#include "System definitions.h"

#include "UART.h"

typedef void tHB_UART_ReceiveCallback(U32 NbReceivedBytes);
typedef void tHB_UART_TransmitCallback(void);

extern const U8 HB_UART_ReceiveTimeDelay;
extern const U8 HB_UART_TransmitTimeDelay;
extern const U8 HB_UART_TransmitCompleteTimeDelay;

void HB_UART_Deinit(void);
U32 HB_UART_GetNbReceivedBytes(void);
void HB_UART_Init(const sUART_Config *pUSARTConfig);
void HB_UART_ReceiveSetup(const void *pData,U32 DataLength,BOOL IsCircular,tHB_UART_ReceiveCallback *pCallback);
void HB_UART_ReceiveStart(void);
void HB_UART_ReceiveStop(void);
void HB_UART_TransmitSetup(const void *pData,U32 DataLength,tHB_UART_TransmitCallback *pCallback);
void HB_UART_TransmitStart(void);
void UB_UART_TransmitStop(void);

#endif // !dedined(HB_UART_H)
