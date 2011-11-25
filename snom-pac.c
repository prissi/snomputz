/****************************************************************************************
****	SNOM-PAC:	Pack- und Entpackroutinen
****************************************************************************************/


#include "snomputz.h"
#include "snom-typ.h"
#include "snom-mem.h"
#include "snom-win.h"
#include "snom-pac.h"


/***************************************************************************************/
/* Zuerst DeltaDekodierung */


// Deltakodierung: Es sind Bytewerte, die die Differenz der Worte angeben
LONG DeltaDecodeBlock( LPUWORD pDest, long lDestLen, LPUCHAR pSrc )
{
	LONG last, i, j;
	ULONG ulData;

	lDestLen /= sizeof( SWORD );
	for( last = i = j = 0;  j < lDestLen;  j++ ) {
		ulData = (UCHAR)pSrc[i++];
		if( ulData == BYTE_OVER ) {
			// Zu gross ?
			ulData = ( ( (ULONG)(UCHAR)pSrc[i++] )<<8 );
			ulData |= (UCHAR)pSrc[i++];
		}
		else {
			ulData = last+(signed long)(signed char)ulData;
		}
		last = ulData;
		pDest[j] = (unsigned short)ulData;
	}
	return ( j*sizeof( short ) );
}


/***************************************************************************************/
/* Huffman-compression uas Numerical Recipies:
 * Tabellen bauen!
 */

static unsigned long setbit[32] = {
	0x1L, 0x2L, 0x4L, 0x8L, 0x10L, 0x20L,
	0x40L, 0x80L, 0x100L, 0x200L, 0x400L, 0x800L, 0x1000L, 0x2000L,
	0x4000L, 0x8000L, 0x10000L, 0x20000L, 0x40000L, 0x80000L, 0x100000L,
	0x200000L, 0x400000L, 0x800000L, 0x1000000L, 0x2000000L, 0x4000000L,
	0x8000000L, 0x10000000L, 0x20000000L, 0x40000000L, 0x80000000L
};

void hufapp( LPULONG index, LPULONG nprob, ULONG n, ULONG i )
{
	unsigned long j, k;

	k = index[i];
	while( i <= ( n>>1 ) ) {
		if( ( j = i << 1 ) < n && nprob[index[j]] > nprob[index[j+1]] ) {
			j++;
		}
		if( nprob[k] <= nprob[index[j]] ) {
			break;
		}
		index[i] = index[j];
		i = j;
	}
	index[i] = k;
}


BOOL hufmak( LPULONG nfreq, unsigned long nchin, LPULONG ilong, unsigned long *nlong, HUFFCODE *hcode )
{
	LPULONG	index, nprob;
	LPLONG up;
	int ibit;
	ULONG j, k;
	LONG node, n, nused;

	hcode->nch = nchin;
	index = (LPULONG)pMalloc( 2*hcode->nch*sizeof( LONG ) );
	up = (LPLONG)pMalloc( 2*hcode->nch*sizeof( LONG ) );
	nprob = (LPULONG)pMalloc( 2*hcode->nch*sizeof( LONG ) );
	if( index == NULL  ||  up == NULL  ||  nprob == NULL ) {
		StatusLineRsc( E_MEMORY );
		MemFree( nprob );
		MemFree( up );
		MemFree( index );
		return ( FALSE );
	}

	for( nused = 0, j = 1; j <= hcode->nch; j++ ) {
		nprob[j] = nfreq[j];
		hcode->icod[j] = hcode->ncod[j] = 0;
		if( nfreq[j] ) {
			index[++nused] = j;
		}
	}

	for( j = nused; j >= 1; j-- ) {
		hufapp( index, nprob, nused, j );
	}

	k = hcode->nch;
	while( nused > 1 ) {
		node = index[1];
		index[1] = index[nused--];
		hufapp( index, nprob, nused, 1 );
		nprob[++k] = nprob[index[1]]+nprob[node];
		hcode->left[k] = node;
		hcode->right[k] = index[1];
		up[index[1]] = -(LONG)k;
		up[node] = index[1] = k;
		hufapp( index, nprob, nused, 1 );
	}
	up[hcode->nodemax = k] = 0;
	for( j = 1; j <= hcode->nch; j++ ) {
		if( nprob[j] ) {
			for( n = 0, ibit = 0, node = up[j]; node; node = up[node], ibit++ ) {
				if( node < 0 ) {
					n |= setbit[ibit];
					node = -node;
				}
			}
			hcode->icod[j] = n;
			hcode->ncod[j] = ibit;
		}
	}
	*nlong = 0;
	for( j = 1; j <= hcode->nch; j++ ) {
		if( hcode->ncod[j] > *nlong ) {
			*nlong = hcode->ncod[j];
			*ilong = j-1;
		}
	}
	MemFree( nprob );
	MemFree( up );
	MemFree( index );
	return ( TRUE );
}


// Initialisiert die Huffmancodierung
// für max. maxchr Zeichen (0..maxchr-1) mit der Häudigkeit nfreq[0..maxchr-1]
BOOLEAN	huffinit( ULONG	maxchr, LPULONG nfreq, HUFFCODE *hcode )
{
	ULONG largest_index, max_len;

	hcode->icod = (LPULONG)pMalloc( 2*maxchr*sizeof( ULONG ) );
	hcode->ncod = (LPULONG)pMalloc( 2*maxchr*sizeof( ULONG ) );
	hcode->left = (LPULONG)pMalloc( 2*maxchr*sizeof( ULONG ) );
	hcode->right = (LPULONG)pMalloc( 2*maxchr*sizeof( ULONG ) );
	if( hcode->icod == NULL  ||  hcode->ncod == NULL  ||  hcode->left == NULL  ||  hcode->right == NULL ) {
		StatusLineRsc( E_MEMORY );
		huffexit( hcode );
		return ( FALSE );
	}
	if( !hufmak( nfreq-1, maxchr, &largest_index, &max_len, hcode )  &&  max_len > 32 ) {
		huffexit( hcode );
		return ( FALSE );
	}
	return ( TRUE );
}


// Gibt Speicher von hcode wieder frei
void huffexit( HUFFCODE *hcode )
{
	if( hcode->icod != NULL ) {
		MemFree( hcode->icod );
	}
	if( hcode->ncod != NULL ) {
		MemFree( hcode->ncod );
	}
	if( hcode->left != NULL ) {
		MemFree( hcode->left );
	}
	if( hcode->right != NULL ) {
		MemFree( hcode->right );
	}
}


// Fügt das Zeichen "ich" in den Puffer "*codep" ab der Bitposition "nb" ein
// TRUE, solange alles io
BOOL huffenc( unsigned long ich, LPUCHAR codep, unsigned long *nb, HUFFCODE *hcode )
{
	int l, n;
	unsigned long k, nc;

	k = ich+1;
	if( k > hcode->nch || k < 1 ) {
		// Char too large!
		return ( FALSE );
	}
	for( n = hcode->ncod[k]-1; n >= 0; n--, ++( *nb ) ) {
		nc = ( *nb >> 3 );
		nc++;
		l = ( *nb ) & 7;
		if( !l ) {
			codep[nc] = 0;
		}
		if( hcode->icod[k] & setbit[n] ) {
			codep[nc] |= setbit[l];
		}
	}
	return ( TRUE );
}


// Dekodiert den Stream der länge "len_code", auf den "*code" zeigt ab dem bit "nb"
// Rückgabe == hcode->nch  => Ende erreicht.
unsigned long huffdec( LPUCHAR code, unsigned long lcode, LPULONG lbitpos, HUFFCODE *hcode )
{
	long nc, node, nb = *lbitpos;

	node = hcode->nodemax;
	while( 1 ) {
		nc = ( nb >> 3 )+1;
		if( nc >= lcode ) {
			return ( hcode->nch );
		}

		node = ( code[nc] & setbit[7 & nb++] ?
		         hcode->right[node] : hcode->left[node] );
		if( node <= hcode->nch ) {
			*lbitpos = nb;
			return ( node-1 );
		}
	}
}


// Packt einen Huffman-Block aus ...
LONG HuffmanDecodeBlock( LPUWORD pDest, LONG lDestLen, LPUCHAR pSrc, BOOLEAN PackedBytes )
{
	HUFFCODE hcode;
	// Anfängliche Bittiefe lesen
	unsigned MaxDaten = *(LPUWORD)pSrc;
	// Zeiger auf Frequenztabelle
	LPULONG	plFreqs = (LPULONG)( pSrc+2 );
	LONG i, t;
	ULONG ulBitPos;
	LPUCHAR	pDestBytes = (LPUCHAR)pDest;

	ulBitPos = 16ul+sizeof( LONG )*(ULONG)MaxDaten*8ul;
	// Routine initialisieren
	if( !huffinit( MaxDaten, plFreqs, &hcode ) ) {
		return ( 0 );
	}

	if( PackedBytes ) {
		for( i = 0;  i < lDestLen;  i++ ) {
			t = huffdec( pSrc, lDestLen, &ulBitPos, &hcode );
			if( t < 0 ) {
				break;
			}
			pDestBytes[i] = (UCHAR)t;
		}
	}
	else {
		for( i = 0;  i < lDestLen/sizeof( UWORD );  i++ ) {
			t = huffdec( pSrc, lDestLen, &ulBitPos, &hcode );
			if( t < 0 ) {
				break;
			}
			pDest[i] = (UWORD)t;
		}
	}
	huffexit( &hcode );
	return ( i*sizeof( UWORD ) );
}


/***************************************************************************************/
/* LZW-Komprimierung/Dekomprimierung
 */

typedef short int code_int;                     /* was int */
typedef long int count_int;


/*******************************************************************************
* Leseroutine aus der JpegLib
*******************************************************************************/

static int GetCode( LPBYTE pSrc, long code_size, int flag )
{
	static BYTE buf[4];
	static long lLastSrcPos, lSrcPos;
	static int curbit, lastbit, done;
	int i, j;
	long ret;

	if( flag ) {
		curbit = 0;
		lastbit = 0;
		done = FALSE;
		lSrcPos = 0;
		lLastSrcPos = code_size;
		lastbit = 0;
		return ( 0 );
	}

	if( ( curbit+code_size ) >= lastbit ) {
		buf[0] = buf[2];
		buf[1] = buf[3];
		buf[2] = pSrc[lSrcPos++];
		buf[3] = pSrc[lSrcPos++];
		curbit = ( curbit - lastbit ) + 16;
		if( lSrcPos >= lLastSrcPos ) {
			lastbit = 32+8*( lSrcPos-lLastSrcPos );
		}
		else {
			lastbit = 32;
		}
	}

	ret = 0;
	for( i = curbit, j = 0; j < code_size; ++i, ++j ) {
//		ret |= ((buf[i/8]&(1<<(i% 8)))!=0)<<j;
		ret |= ( ( buf[i>>3]>>( i&7 ) )&1 )<<j;
	}
	curbit += code_size;
	return ( ret );
}


int LZWReadByte( LPBYTE pSrc, long lNewSrcLen, int input_code_size )
{
	static int fresh = FALSE;
	int code, incode, flag = ( lNewSrcLen != 0l );
	static int code_size, set_code_size;
	static int max_code, max_code_size;
	static int firstcode, oldcode;
	static int clear_code, end_code;
	static LPUWORD next;
	static LPBYTE vals;
	static LPBYTE stack;
	static LPBYTE sp;
	register int i;

	if( flag  &&  input_code_size < 0 ) {
		// Exit
		MemFree( next );
		MemFree( vals );
		MemFree( stack );
		return ( 0 );
	}

	if( flag ) { // init flagged => init everything

		next = (LPUWORD)pMalloc( ( 1<<MAX_LZW_BITS )*sizeof( short ) );
		vals = (LPBYTE)pMalloc( ( 1<<MAX_LZW_BITS ) );
		stack = (LPBYTE)pMalloc( ( 1<<( MAX_LZW_BITS+1 ) ) );

		set_code_size = input_code_size;
		code_size = set_code_size+1;
		clear_code = 1<<set_code_size;
		end_code = clear_code+1;
		max_code = clear_code+2;
		max_code_size = 2*clear_code;

		GetCode( pSrc, lNewSrcLen, TRUE );

		fresh = TRUE;
		for( i = 0; i < clear_code; ++i ) {
			next[i] = 0;
			vals[i] = i;
		}

		for(; i < ( 1<<MAX_LZW_BITS ); ++i ) {
			next[i] = vals[0] = 0;
		}

		sp = stack;
		return ( 0 );
	}
	else if( fresh ) {
		fresh = FALSE;
		do {
			firstcode = oldcode = GetCode( pSrc, code_size, FALSE );
		} while( firstcode == clear_code );
		return ( firstcode ) ;
	}

	if( (long)sp > (long)stack ) {
		return ( *--sp ) ;
	}

	while( ( code = GetCode( pSrc, code_size, FALSE ) ) >= 0 ) {
		if( code == clear_code ) {
			for( i = 0; i < clear_code; ++i ) {
				next[i] = 0;
				vals[i] = i;
			}
			for(; i < ( 1<<MAX_LZW_BITS ); ++i ) {
				next[i] = vals[i] = 0;
			}
			code_size = set_code_size+1;
			max_code_size = 2*clear_code;
			max_code = clear_code+2;
			sp = stack;
			firstcode = oldcode = GetCode( pSrc, code_size, FALSE );
			return ( firstcode ) ;
		}
		else if( code == end_code ) {
			return ( -2 ) ;
		}

		incode = code;

		if( code >= max_code ) {
			*sp++ = firstcode;
			code = oldcode;
		}

		while( code >= clear_code ) {
			*sp++ = vals[code];
			if( code == (int)next[code] ) {
				//m_GIFErrorText="Circular table entry, big GIF Error!";
				return ( -1 ) ;
			}
			code = next[code];
		}

		*sp++ = firstcode = vals[code];

		if( ( code = max_code ) < ( 1<<MAX_LZW_BITS ) ) {
			next[code] = oldcode;
			vals[code] = firstcode;
			++max_code;
			if( ( max_code >= max_code_size ) &&
			    ( max_code_size < ( 1<<MAX_LZW_BITS ) ) ) {
				max_code_size *= 2;
				++code_size;
			}
		}

		oldcode = incode;

		if( (long)sp > (long)stack ) {
			return ( *--sp );
		}
	}
	return ( code );
}


//	Entpackt das Array pSrc
long LZWDecodeBlock( LPBYTE pDest, LONG lDestLen, LPBYTE pSrc, LONG lSrcLen )
{
	long i, t;
	// Anfängliche Bittiefe lesen
	unsigned nBits;

	nBits = *pSrc++;

	// Routine initialisieren
	if( LZWReadByte( pSrc, lSrcLen-2, nBits ) < 0 ) {
		return ( FALSE );  // <0 => Fehler!
	}
	for( i = 0;  i < lDestLen;  i++ ) {
		t = LZWReadByte( pSrc, FALSE, nBits );
		if( t < 0 ) {
			break;
		}
		pDest[i] = (UCHAR)t;
	}
	LZWReadByte( pSrc, TRUE, -1 );  // free
	return ( i );
}


/***************************************************************************/


long output( LPBYTE ptr, code_int code, int *n_bits, int init_bits );
void cl_block( int *nbits, int init_bits );
void cl_hash( count_int hsize );


/***************************************************************************
*
*  GIFCOMPR.C       - GIF Image compression routines
*
*  Lempel-Ziv compression based on 'compress'.  GIF modifications by
*  David Rowley (mgardi@watdcsu.waterloo.edu)
*
***************************************************************************/

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003            /* 80% occupancy */

typedef        unsigned char char_type;

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */

static int maxbits = BITS;                /* user settable max # bits/code */

static code_int maxcode;                  /* maximum code, given n_bits */
static code_int maxmaxcode = (code_int)1 << BITS; /* should NEVER generate this code */

#define MAXCODE( n_bits )        ( ( (code_int) 1 << ( n_bits ) ) - 1 )

static count_int HUGE *htab;
static LPUWORD codetab;
static code_int	free_ent = 0;                                          /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

/*
 * compress pixels to GIF packets
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int ClearCode;
static int EOFCode;


long LZWCompressBlock( int init_bits, LPBYTE dest, long dest_len, LPBYTE src, long src_len )
{
	long fcode, data_len, src_pos = 0;
	code_int i /* = 0 */;
	int c;
	code_int ent;
	code_int disp;
	int hshift;
	int nbits;

	htab = (count_int HUGE*)pMalloc( sizeof( count_int )*HSIZE );
	codetab = (LPUWORD)pMalloc( sizeof( UWORD )*HSIZE );

	/*
	 * Set up the necessary values
	 */
	*dest++ = init_bits;
	init_bits++;
	nbits = init_bits;
	clear_flg = 0;
	maxcode = MAXCODE( nbits );

	ClearCode = ( 1 << ( nbits - 1 ) );
	EOFCode = ClearCode + 1;
	free_ent = ClearCode + 2;

	hshift = 0;
	for( fcode = (long) HSIZE;  fcode < 65536L; fcode *= 2L ) {
		++hshift;
	}
	hshift = 8 - hshift;                /* set hash code range bound */

	cl_hash( (count_int) HSIZE );            /* clear hash table */
	output( dest, (code_int)ClearCode, &nbits, init_bits );

	ent = *src++;
	while( src_pos++ < src_len ) {
		c = *src++;
		fcode = (long) ( ( (long) c << maxbits ) + ent );
		i = ( ( (code_int)c << hshift ) ^ ent );    /* xor hashing */

		if( htab[i] == fcode ) {
			ent = codetab[i];
			continue;
		}
		else if( (long)htab[i] < 0 ) {        /* empty slot */
			goto nomatch;
		}

		disp = HSIZE - i;           /* secondary hash (after G. Knott) */
		if( i == 0 ) {
			disp = 1;
		}

probe:
		if( ( i -= disp ) < 0 ) {
			i += HSIZE;
		}

		if( htab[i] == fcode ) {
			ent = codetab[i];
			continue;
		}

		if( (long)htab[i] > 0 ) {
			goto probe;
		}

nomatch:        // kein Treffer => bisher ausgeben
		data_len = output( NULL, (code_int)ent, &nbits, init_bits );
		if( data_len >= dest_len-2 ) {
			return ( -2 );      // Komprimierte Version ist zu groß für den Buffer!
		}
		if( data_len >= src_len ) {
			return ( -1 );      // Länge ist größer als die vom Eingangsstring
		}
		ent = c;
		if( free_ent < maxmaxcode ) {
			codetab[i] = free_ent++; /* code -> hashtable */
			htab[i] = fcode;
		}
		else {
			cl_block( &nbits, init_bits );
		}
	}
	/*
	 * Put out the final code.
	 */
	MemFree( htab );
	MemFree( codetab );
	htab = NULL;
	codetab = NULL;

	output( NULL, (code_int)ent, &nbits, init_bits );
	return ( output( NULL, (code_int)EOFCode, &nbits, init_bits )+1ul );
}


/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static unsigned long masks[] = {
	0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
	0x001F, 0x003F, 0x007F, 0x00FF,
	0x01FF, 0x03FF, 0x07FF, 0x0FFF,
	0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

long output( LPBYTE ptr, code_int code, int *n_bits, int init_bits )
{
	static LPBYTE dest;
	static long len;
	static unsigned long cur_accum;
	static int cur_bits;

	if( ptr ) {
		dest = ptr;
		len = 0l;
		cur_accum = 0l;
		cur_bits = 0;
	}

	cur_accum &= masks[ cur_bits ];

	if( cur_bits > 0 ) {
		cur_accum |= ( (long)code << cur_bits );
	}
	else {
		cur_accum = code;
	}
	cur_bits += *n_bits;

	while( cur_bits >= 8 ) {
		*dest++ = (char)cur_accum;
		len++;
		cur_accum >>= 8;
		cur_bits -= 8;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */
	if( free_ent > maxcode || clear_flg ) {
		if( clear_flg )	{
			maxcode = MAXCODE( *n_bits = init_bits );
			clear_flg = 0;
		}
		else {
			++*n_bits;
			if( *n_bits == maxbits ) {
				maxcode = maxmaxcode;
			}
			else {
				maxcode = MAXCODE( *n_bits );
			}
		}
	}

	/*
	 * At EOF, write the rest of the buffer.
	 */
	if( code == EOFCode ) {
		while( cur_bits > 0 ) {
			*dest++ = (char)cur_accum;
			len++;
			cur_accum >>= 8;
			cur_bits -= 8;
		}
	}
	return ( len );
}


void cl_block( int *nbits, int init_bits )
{
	cl_hash( (count_int)HSIZE );
	free_ent = ClearCode+2;
	clear_flg = 1;
	output( NULL, (code_int)ClearCode, nbits, init_bits );
}


void cl_hash( count_int hsize )
{
	count_int HUGE *htab_p = htab+hsize;

	long i;
	long m1 = -1L;

	i = hsize - 16;

	do {
		*( htab_p-16 ) = m1;
		*( htab_p-15 ) = m1;
		*( htab_p-14 ) = m1;
		*( htab_p-13 ) = m1;
		*( htab_p-12 ) = m1;
		*( htab_p-11 ) = m1;
		*( htab_p-10 ) = m1;
		*( htab_p-9 ) = m1;
		*( htab_p-8 ) = m1;
		*( htab_p-7 ) = m1;
		*( htab_p-6 ) = m1;
		*( htab_p-5 ) = m1;
		*( htab_p-4 ) = m1;
		*( htab_p-3 ) = m1;
		*( htab_p-2 ) = m1;
		*( htab_p-1 ) = m1;

		htab_p -= 16;
	} while( ( i -= 16 ) >= 0 );

	for( i += 16; i > 0; --i ) {
		*--htab_p = m1;
	}
}
