#if !defined(TYPES_H)
#define TYPES_H

typedef          char      X8;
typedef unsigned char      U8;
typedef   signed char      S8;
typedef unsigned short     U16;
typedef   signed short     S16;
typedef unsigned long      U32;
typedef   signed long      S32;
typedef unsigned long long U64;
typedef   signed long long S64;

#if !defined(NULL)
	#define NULL  0
#endif
#if !defined(BOOL)
	typedef U8 BOOL;
#endif
#if !defined(FALSE)
	#define FALSE 0
#endif
#if !defined(TRUE)
	#define TRUE 1
#endif

#endif // !defined(TYPES_H)
