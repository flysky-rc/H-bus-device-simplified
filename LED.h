#if !defined(LED_H)
#define LED_H

#include "System definitions.h"

typedef struct
{
	U32 Pattern;
	U16 NbPatternBits;
	U16 MsPerPatternBit;
} sLED_Pattern;

extern const sLED_Pattern LED_RESET;       
extern const sLED_Pattern LED_ENUMERATING; 
extern const sLED_Pattern LED_CONFIGURED;  


void LED_MsIRQHandler(void);
void LED_Init(void);
void LED_SetPattern(const sLED_Pattern *pPattern);

#endif // !defined(LED_H)
