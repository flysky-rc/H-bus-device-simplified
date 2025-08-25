#if !defined(GPIO_H)
#define GPIO_H

#include "System definitions.h"

typedef void tGPIO_Callback(void);

void GPIO_DisablePA1Interrupt(void);
void GPIO_EnablePA1Interrupt(tGPIO_Callback *pCallback);
void GPIO_Init(void);

#endif // !defined(GPIO_H)

