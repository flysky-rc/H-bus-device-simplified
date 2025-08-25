#if !defined(SYSTEM_DEFINITIONS_H)
#define SYSTEM_DEFINITIONS_H

#include "Types.h"

#if !defined(ATTRIBUTE_PACKED)
	#if defined(_WIN32) || defined(_WIN64)
		#define ATTRIBUTE_PACKED
	#else
		#define ATTRIBUTE_PACKED __attribute__((packed))
	#endif
#endif

#define SYS_IN_RAM_SPI_RO_NO_CACHE __attribute((section("RAM_SPI_RO_NO_CACHE")))
#define SYS_IN_RAM_SPI_NO_CACHE    __attribute((section(".bss.RAM_SPI_NO_CACHE")))

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define MAX(X,Y) (((X)>(Y))?(X):(Y))

#if defined(__ARM_FEATURE_IDIV)
	#define DIV10(N) (N/10)
#else
	#define DIV10(N) ((((U32)N)*52429)>>19) // Valid from 0 to 81919
#endif

#if !defined(_WIN32) && !defined(_WIN64)
__attribute__((always_inline)) static __inline void SYS_DisableIRQs(void)
{
	__asm volatile ("cpsid i" : : : "memory");
	return;
}

__attribute__((always_inline)) static __inline void SYS_EnableIRQs(void)
{
	__asm volatile ("cpsie i" : : : "memory");
	return;
}
#endif

#if defined(__ARM_FEATURE_UNALIGNED) && !defined(SYS_NO_UNALIGNED_ACCESS)
__attribute__((always_inline)) static __inline U32 SYS_LoadULong(const void *pPointer)
{
	return *(const U32 *)pPointer;
}

__attribute__((always_inline)) static __inline U16 SYS_LoadUShort(const void *pPointer)
{
	return *(const U16 *)pPointer;
}

__attribute__((always_inline)) static __inline void SYS_StoreULong(void *pPointer,U32 Value)
{
	*(U32 *)(pPointer)=Value;
	return;
}

__attribute__((always_inline)) static __inline void SYS_StoreUShort(void *pPointer,U16 Value)
{
	*(U16 *)(pPointer)=Value;
	return;
}
#else
__attribute__((always_inline)) static __inline U32 SYS_LoadULong(const void *pPointer)
{
	return (U32)(((*((const U8 *)(pPointer)+0))<< 0) | ((*((const U8 *)(pPointer)+1))<< 8) |
		((*((const U8 *)(pPointer)+2))<<16) | ((*((const U8 *)(pPointer)+3))<<24));
}

__attribute__((always_inline)) static __inline U16 SYS_LoadUShort(const void *pPointer)
{
	return (U16) (((*((const U8 *)(pPointer)+0))<< 0) | ((*((const U8 *)(pPointer)+1))<< 8));
}

__attribute__((always_inline)) static __inline void SYS_StoreULong(void *pPointer,U32 Value)
{
	*((U8 *)(pPointer)+0)=(U8)((Value)>> 0);
	*((U8 *)(pPointer)+1)=(U8)((Value)>> 8);
	*((U8 *)(pPointer)+2)=(U8)((Value)>>16);
	*((U8 *)(pPointer)+3)=(U8)((Value)>>24);
	return;
}

__attribute__((always_inline)) static __inline void SYS_StoreUShort(void *pPointer,U16 Value)
{
	*((U8 *)(pPointer)+0)=(U8)((Value)>> 0);
	*((U8 *)(pPointer)+1)=(U8)((Value)>> 8);
	return;
}
#endif

#endif // !defined(SYSTEM_DEFINITIONS_H)
