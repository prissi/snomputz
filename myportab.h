/**************************************************************************************
****	Meine PORTAB.H!
**************************************************************************************/

#ifndef __MYPORTAB
#define __MYPORTAB

/* Datei muss nach Windows.h eingeladen werden */

// Keine Warnungen (unknown prgma, 0.0*float)
#pragma warning( disable : 4068 4244 )
// Keine Warnung (significant digits)
#pragma warn -sig


#define LEN_OK 1
#if defined(__WIN32__) || defined(_WIN32)
#define BIT32
#endif

#ifndef NDEBUG
#include <assert.h>
#define ASSERT assert
#else
#define ASSERT( i )
#endif


/* ACHTUNG:
 *	wegen der schwachsinnigen Definition von MS-Windows ist
 *
 *	WORD	unsigned short		statt	UWORD
 *	INT		short							statt WORD
 *
 * wie in "portab.h" eigentlich vorgesehen.
 *
 * Ausserdem heissen alle Pointer LPxxx (wegen der Kompatibilität
 * zu 16 Bit Windows ...)
 */

typedef unsigned short UWORD;
typedef signed short SWORD;
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef signed char SCHAR;

#ifndef	BIT32
typedef unsigned char CHAR;
#define HUGE huge
#else
#define HUGE
typedef void HUGE *  LPVOID;
#endif
typedef ULONG HUGE * LPULONG;
typedef UWORD HUGE * LPUWORD;
typedef SWORD HUGE * LPSWORD;
typedef SCHAR HUGE *  LPCHAR;
typedef UCHAR HUGE *  LPUCHAR;
typedef float HUGE * LPFLOAT;
typedef double HUGE *LPDOUBLE;

#ifndef BIT32
#undef TRUE
#undef FALSE
#undef BOOLEAN
typedef enum {FALSE, TRUE} BOOLEAN;
#endif


/*
 * Die Abkürzungen für ungarische Notation sind
 *	c : char (immer unsigned, sonst sc)
 *  i : int  (short oder long, nach Implementation)
 *  u : unsigned (default short)
 *  s : signed (default short)
 *  l : long (default signed, sonst ul)
 *  p : pointer (immer huge)
 *	f : float
 *	lf : double
 *	str : Zeiger auf String
 *	void :void
 */

#endif