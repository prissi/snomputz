/****************************************************************************************
****	SNOM-PAC:	Pack- und Entpackroutinen
****************************************************************************************/

#include "myportab.h"


// Verschiedene Kompressionsmodi
#define	DELTA		1	// 8 Bit Deltakodierung
#define HUFFMAN	2	// max. 8 Bit Huffmankodierung
#define NBITS		4	// einfach die Bits zusammengeschoben
#define LZW			8 // LZW-Kompression


// Wert, der signalisiert, dass nun ein Wort folgt
#define	BYTE_OVER	0x00000080ul

// Deltakodierung: Es sind Bytewerte, die die Differenz der Worte angeben
LONG	DeltaDecodeBlock( LPUWORD pDest, long lDestLen, LPUCHAR pSrc );
// 2.10.98


/***************************************************************************************/
/* Huffmankodierung aus Numerical Recipies */

#define NRANSI

typedef struct {
	LPULONG icod;
	LPULONG ncod;
	LPULONG left;
	LPULONG right;
	ULONG		nch, nodemax;
} HUFFCODE;


// Initialisiert die Huffmancodierung
// für max. maxchr Zeichen (0..maxchr-1) mit der Häudigkeit nfreq[0..maxchr-1]
BOOLEAN	huffinit( ULONG	maxchr, LPULONG nfreq, HUFFCODE *hcode );

// Gibt Speicher von hcode wieder frei
void	huffexit( HUFFCODE *hcode );

// Fügt das Zeichen "ich" in den Puffer "*codep" ab der Bitposition "nb" ein
// TRUE, solange alles io
BOOL huffenc(unsigned long ich, LPUCHAR codep, unsigned long *nb, HUFFCODE *hcode);

// Packt einen Huffman-Block aus ...
// Reicht eigentlich schon, der Rest ist eher privat ...
LONG	HuffmanDecodeBlock( LPUWORD pDest, LONG lDestLen, LPUCHAR pSrc, BOOLEAN PackedBytes );
// 2.10.98

/***************************************************************************************/
/* Alte LZW-Routinen (verschiedene Quellen) */

#define MAX_LZW_BITS	12
#define OLD_LZW				64	// Altes LZW-Packen ...
#define COMPRESSED		8
#define INIT_BITS			8

//	Entpackt das Array pSrc
long	LZWDecodeBlock( LPBYTE pDest, LONG lDestLen, LPBYTE pSrc, LONG lSrcLen );

long LZWCompressBlock( int init_bits, LPBYTE dest, long dest_len, LPBYTE src, long src_len );

