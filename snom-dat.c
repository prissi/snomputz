/*****************************************************************************************
****	SNOM-DAT: Routinen zum Laden und Speicher (Packroutinen siehe SNOM-PAC)
*****************************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	"myportab.h"

#include	"snomputz.h"
#include	"snom-typ.h"
#include	"snom-var.h"
#include	"snomlang.h"

#include	"snom-pac.h"
#include	"snom-dat.h"
#include	"snom-mem.h"
#include	"snom-mat.h"
#include	"snom-wrk.h"
#include	"snom-win.h"
#include	"snom-dlg.h"

#include	"hdf-file.h"
#include	"hitachi.h"
#include	"WSxM.h"

#define	DEFAULT_DIGITAL_HEADER_LEN 8192l
#define MAX_DI_HDLEN 65536


/*****************************************************************************************
 * Konvertiert einen Datenblock in ein Snombild (Topografie, Fehler oder Lumineszens)
 * Wird zuletzt immer dann aufgerufen, wenn Daten geladen sind.
 * Die Daten werden als erstes Element (pBmp->pSnom[0]) eingefügt
 * w beziechnet die Breite einer Zeile in Vielfachen von uBits;
 * dies kann mehr sein, als die Weite w, der Rest wird ignoriert.
 * uBits ist die Breite in Bits 1..16, uAdd ein Offset, der aufaddiert werden soll (meist 0 oder 0x8000)
 * für signed/unsigned.
 */
BOOLEAN	LadeBlock( LPBMPDATA pBmp, LPVOID pvPtr, LONG w, LONG ww, LONG h, int iBits, WORKMODE Mode, unsigned short uAdd, BOOLEAN InitRest )
{
	LPSNOMDATA pSnom;
	LPUWORD puData, puZeile;
	LONG i, iMax = 0, iMin = 65536;
	LONG x, y;

	// Hat die Routine Schwachsinn erhalten?
	ASSERT(  pBmp != NULL  &&  pvPtr != NULL  &&  pBmp->iAktuell == 0  &&  ww >= w  );

	// Sicherstellen, dass es immer ein pBmp->pSnom[0] mit "sinnvollen" Werten gibt
	ASSERT( ( Mode == TOPO  &&  pBmp->pSnom[0].Topo.puDaten == NULL )
	        || ( Mode == ERRO  &&  pBmp->pSnom[0].Error.puDaten == NULL )
	        || ( Mode == LUMI  &&  pBmp->pSnom[0].Lumi.puDaten == NULL ) );

	// Ausmaüe initialisieren
	pSnom = &( pBmp->pSnom[pBmp->iAktuell] );
	if( pSnom->w == 0  ||  pSnom->h == 0 ) {
		pSnom->w = w;
		pSnom->h = h;
	}
	// Kann keine zwei verschiedenen Größen gleichzeitig behandeln
	if( pSnom->w > w  ||   pSnom->h > h ) {
		FehlerRsc( E_TOO_SMALL );
		return ( FALSE );
	}
	w = pSnom->w;
	h = pSnom->h;

	// Speicher anfordern
	puData = (LPUWORD)pMalloc( pSnom->w*pSnom->h*sizeof( WORD ) );
	if( puData == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	// Eh 16-Bit Daten? Dann einfach kopieren
	if( iBits == 16 ) {
		LPUWORD	puSrc = (LPUWORD)pvPtr;

		puZeile = puData;
		for( y = 0;  y < h;  y++ ) {
			for( x = 0;  x < w;  x++ ) { // attention: Never put the next two lines into a single one!!!! (MS compiler error!!!!)
				i = (signed short)puSrc[x];
				i += uAdd;
				if( i > iMax ) {
					iMax = i;
				}
				if( i < iMin ) {
					iMin = i;
				}
				puZeile[x] = i;
			}
			// Bedenke: w kann != ww sein!
			puZeile += w;
			puSrc += ww;
		}
	}
	else if( iBits == 8 ) {
		// sonst Daten nach 16-Bit konvertieren
		// 8 Bit Daten
		LPUCHAR	pcSrc = (LPUCHAR)pvPtr;
		SWORD iTemp;

		puZeile = puData;
		for( y = 0;  y < h;  y++ ) {
			for( x = 0;  x < w;  x++ ) {
				iTemp = (SWORD)( (UCHAR)( pcSrc[x] ) )+uAdd;
				if( iTemp > iMax ) {
					iMax = iTemp;
				}
				if( iTemp < iMin ) {
					iMin = iTemp;
				}
				puZeile[x] = iTemp;
			}
			// Bedenken: w kann > ww sein!
			puZeile += w;
			pcSrc += ww;
		}
	}
	else if( iBits > 0 ) {
		// Krumme Bits nach 16 Bit ...
		// Viel Bitschieben
		LPUCHAR	pcSrc = (LPUCHAR)pvPtr;
		ULONG ulData;
		LONG i, lByteOffset;
		int iBitLen, iBitOffset;
// Krumme Bits sollten sinvolleweise auch gepackt sein ...
		ASSERT( w == ww );

		puZeile = puData;
		for( x = i = 0;  i < w*h*iBits;  i += iBits ) {
			lByteOffset = i/8;
			iBitOffset = i&7;
			iBitLen = iBits-( 8-iBitOffset );
			ulData = pcSrc[lByteOffset++]&( 0x00FF>>iBitOffset );
			while( iBitLen > 0 ) {
				ulData <<= 8;
				ulData |= pcSrc[lByteOffset++];
				iBitLen -= 8;
			}
			if( iBitLen < 0 ) {
				ulData >>= -iBitLen;
			}
			ulData += uAdd;
			puZeile[x++] = (UWORD)ulData;
			ASSERT(  ulData <= 65536ul  );
			if( ulData > iMax ) {
				iMax = (ULONG)ulData;
			}
			if( ulData < iMin ) {
				iMin = (ULONG)ulData;
			}
		}
	}

	// Minimum auf Null legen
	if( Mode == TOPO ) {
		pSnom->Topo.puDaten = puData;
		pSnom->Topo.Typ = TOPO;
		BildMinMax( ( &pSnom->Topo ), iMin, iMax, w, h );
		// Evt. initialisieren
		if( InitRest ) {
			pSnom->Topo.bPseudo3D = FALSE;
			pSnom->Topo.bModuloKonturen = FALSE;
			pSnom->Topo.bKonturen = FALSE;
			pSnom->Topo.Farben[0] = 0x0l;
			pSnom->Topo.Farben[1] = 0x00004080ul;
			pSnom->Topo.Farben[2] = 0x00FFFFFFul;
			pSnom->Topo.bNoLUT = TRUE;
			pSnom->Topo.fStart = 0.0;
			pSnom->Topo.fEnde = 100.0;
			pBmp->Links = TOPO;
			pSnom->Topo.fXYWinkel3D = f3DXYWinkel;
			pSnom->Topo.fZWinkel3D = f3DZWinkel;
			pSnom->Topo.fZSkal3D = f3DZWinkel;
			pSnom->Topo.bSpecialZUnit = FALSE;
			pSnom->Topo.bShowNoZ = FALSE;
			lstrcpy( pSnom->Topo.strZUnit, STR_TOPO_UNIT );
			pSnom->Topo.iNumColors = 0;
		}
	}
	if( Mode == ERRO ) {
		pSnom->Error.puDaten = puData;
		pSnom->Error.Typ = ERRO;
		BildMinMax( ( &pSnom->Error ), iMin, iMax, w, h );
		if( InitRest ) {
			pSnom->Error.bPseudo3D = FALSE;
			pSnom->Error.bModuloKonturen = FALSE;
			pSnom->Error.bKonturen = FALSE;
			pSnom->Error.Farben[0] = 0x0l;
			pSnom->Error.Farben[1] = 0x00004080ful;
			pSnom->Error.Farben[2] = 0x00FFFFFFul;
			pSnom->Error.bNoLUT = TRUE;
			pSnom->Error.fStart = 0.0;
			pSnom->Error.fEnde = 100.0;
			pBmp->Rechts = ERRO;
			pSnom->Error.fXYWinkel3D = f3DXYWinkel;
			pSnom->Error.fZWinkel3D = f3DZWinkel;
			pSnom->Error.fZSkal3D = f3DZWinkel;
			pSnom->Error.bSpecialZUnit = FALSE;
			pSnom->Error.bShowNoZ = TRUE;
			pSnom->Error.strZUnit[0] = 0;
			pSnom->Error.iNumColors = 0;
		}
	}
	if( Mode == LUMI ) {
		pSnom->Lumi.puDaten = puData;
		pSnom->Lumi.Typ = LUMI;
		BildMinMax( ( &pSnom->Lumi ), iMin, iMax, w, h );
		if( InitRest ) {
			pSnom->Lumi.bPseudo3D = FALSE;
			pSnom->Lumi.bModuloKonturen = FALSE;
			pSnom->Lumi.bKonturen = FALSE;
			pSnom->Lumi.Farben[0] = 0x00ff0000ul;
			pSnom->Lumi.Farben[1] = 0x0000ff00ul;
			pSnom->Lumi.Farben[2] = 0x000000fful;
			pSnom->Lumi.bNoLUT = TRUE;
			pSnom->Lumi.fStart = 0.0;
			pSnom->Lumi.fEnde = 100.0;
			pBmp->Rechts = LUMI;
			pSnom->Lumi.fXYWinkel3D = f3DXYWinkel;
			pSnom->Lumi.fZWinkel3D = f3DZWinkel;
			pSnom->Lumi.fZSkal3D = f3DZWinkel;
			pSnom->Lumi.bSpecialZUnit = TRUE;
			pSnom->Lumi.bShowNoZ = FALSE;
			lstrcpy( pSnom->Lumi.strZUnit, STR_LUMI_UNIT );
			pSnom->Lumi.iNumColors = 0;
		}
	}
	// sanity checks
	if(  pSnom->fY/pSnom->fX < 0.1  ||  pSnom->fY/pSnom->fX > 10.0  ) {
		pSnom->fY = pSnom->fX;
	}
	return ( TRUE );
}
// 26.7.97


/*****************************************************************************************
 * Spezielle Dateiformate
 *****************************************************************************************/



/*************************************** WSxM-Format ***************************************
 * See description from www.nanotec.es
 */
BOOL ReadWSxM( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	LPBYTE pcBuf, pcC, pcC2;
	CHAR str[1024], str2[16];
	long iLen = 0, lDataLen, lHeaderLen = MAX_DI_HDLEN;
	WORKMODE Header = 0;
	double dWx, dWy, dWz;
	BOOL bDoubleData = FALSE;               // hopefully ...

	// Einige Defaultwerte eintagen
	Header = TOPO;
	pBmp->pPsi.fRot = 0.0;
	// Header ist hoffentlich kleiner als 65536 (zumindest was den INHALT angeht ... )
	pcBuf = (LPBYTE)pMalloc( MAX_DI_HDLEN );
	if( pcBuf == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	// Ganzen Header auf einmal lesen
	_llseek( hFile, 0l, 0 );
	_lread( hFile, pcBuf, MAX_DI_HDLEN );

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );
	lstrcpy( pSnom->Error.strTitel, STR_ERROR );
	lstrcpy( pSnom->Lumi.strTitel, STR_LUMI );

	pcC = pcBuf;
	while( iLen++ < lHeaderLen ) {
		// Skip leading Spaces
		while( iLen < lHeaderLen  &&  *pcC <= ' ' ) {
			pcC++;
			iLen++;
		}
		// copy now to string
		pcC2 = str;
		for( lDataLen = 0;  lDataLen < 1022  &&  iLen++ < lHeaderLen  &&  *pcC >= ' ';  lDataLen++ ) {
			*pcC2++ = *pcC++;
		}
		while( *pcC <= 13  &&  iLen++ < lHeaderLen )
			pcC++;
		*pcC2++ = 0;

		switch( str[0] ) { // Um das Laden zu beschleunigen ...
			// Channel
			case 'A':
				if( strstr( str, IMAGE_HEADER_GENERAL_INFO_ACQUISITION ) ) {
					if( strstr( str, "Topo" ) ) {
						Header = TOPO;
					}
					else {
						Header = ERRO;
					}
				}
				break;

			// length of Header
			case 'I':
				if( strstr( str, IMAGE_HEADER_SIZE_TEXT ) ) {
					lHeaderLen = atoi( str+19 );
					break;
				}
				// Newer Versions save double Data ...
				if( strstr( str, IMAGE_DOUBLE_DATA ) ) {
					bDoubleData = TRUE;
				}
				break;

			// width/height
			case 'N':
				if( strstr( str, IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS ) ) {
					pSnom->w = atoi( str+19 );
					break;
				}
				if( strstr( str, IMAGE_HEADER_GENERAL_INFO_NUM_ROWS ) )	{
					pSnom->h = atoi( str+15 );
					break;
				}
				break;

			// Scaling
			case 'X':
				if( strstr( str, IMAGE_HEADER_CONTROL_X_AMPLITUDE ) ) {
					sscanf( str+12, "%Flf %Fs", (LPDOUBLE)&dWx, (LPSTR)str2 );
					if( str2[0] == 'ü' ) { // micrometer
						dWx *= 1000.0;
					}
					if( str2[0] == 'ü' ) { // Aangstroem
						dWx /= 10.0;
					}
				}
				break;

			case 'Y':
				if( strstr( str, IMAGE_HEADER_CONTROL_Y_AMPLITUDE ) ) {
					sscanf( str+12, "%Flf %Fs", (LPDOUBLE)&dWy, (LPSTR)str2 );
					if( str2[0] == 'ü' ) { // micrometer
						dWy *= 1000.0;
					}
					if( str2[0] == 'ü' ) { // Aangstroem
						dWy /= 10.0;
					}
				}
				break;

			case 'Z':
				// Attention: This is the full Ampitude from MAX to MIN!!! ...
				if( strstr( str, IMAGE_HEADER_GENERAL_INFO_Z_AMPLITUDE ) ) {
					sscanf( str+12, "%Flf %Fs", (LPDOUBLE)&dWz, (LPSTR)str2 );
					if( str2[0] == 'ü' ) { // micrometer
						dWz *= 1000.0;
					}
					if( str2[0] == 'ü' ) { // Aangstroem
						dWz /= 10.0;
					}
				}
				break;

			case '[':
				if( strstr( str, "[Header_end]" ) ) {
					goto ReadWSxMNow;
				}
		}
	}
ReadWSxMNow:
	// Ok, there is always only a single Bitmap to read
	pSnom->fX = dWx/pSnom->w;
	pSnom->fY = dWy/pSnom->h;
	if( ( pBmp->pExtra = pMalloc( lHeaderLen ) ) != NULL ) {
		pBmp->Typ = WSxM;
		pBmp->lExtraLen = lHeaderLen;
		MemMove( pBmp->pExtra, pcBuf, lHeaderLen );
	}
	MemFree( pcBuf );
	_llseek( hFile, lHeaderLen, 0 );
	lDataLen = pSnom->w*pSnom->h*sizeof( short );
	pcBuf = pMalloc( lDataLen );
	if( !bDoubleData ) {
		if( pcBuf == 0  ||  _hread( hFile, pcBuf, lDataLen ) < lDataLen ) {
			MemFree( pcBuf );
			_lclose( hFile );
			FehlerRsc( E_FILE );
			return ( FALSE );
		}
	}
	else {
		// this translates to trouble data ...
	}
	_lclose( hFile );
	LadeBlock( pBmp, pcBuf, pSnom->w, pSnom->w, pSnom->h, 16, Header, 32767, TRUE );
	MemFree( pcBuf );
	if( Header == TOPO ) {
		pSnom->Topo.fSkal = dWz/(double)pSnom->Topo.uMaxDaten;
	}
	else {
		pSnom->Error.fSkal = dWz/(double)pSnom->Error.uMaxDaten;
	}
	return ( TRUE );
}


// Finish read WSxM


//*************************************** Hitachi-Format ***************************************

// Format description see "Hitachi.h"
BOOL ReadHitachi( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	HitachiSpm *pHeader;
	LONG i;
	WORD *pBuffer;

	// read all header
	_llseek( hFile, 0l, 0 );
	// Header has fixed length
	pHeader = pMalloc( sizeof( HitachiSpm ) );
	if( pHeader == NULL ) {
		_lclose( hFile );
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}
	_lread( hFile, (LPVOID)pHeader, sizeof( HitachiSpm ) );
	// Keep original info
	pBmp->pExtra = (LPUCHAR)pHeader;
	pBmp->lExtraLen = sizeof( HitachiSpm );
	pBmp->Typ = HITACHI;
	// Now copy Header data into Snomputz data

	pSnom->w = pHeader->nx;
	pSnom->h = pHeader->ny;

	pSnom->Topo.fSkal = pHeader->rz*1e9;

	pSnom->fX = pHeader->rx*(double)pHeader->vx*1e9/(double)pHeader->nx;
	pSnom->fY = pHeader->ry*(double)pHeader->vy*1e9/(double)pHeader->ny;

	// and finally read the data
	i = (long)pHeader->nx*(long)pHeader->ny*sizeof( UWORD );
	pBuffer = pMalloc( i );
	if( pBuffer == 0  ||  _hread( hFile, pBuffer, i ) < i )	{
		MemFree( pBuffer );
		_lclose( hFile );
		FehlerRsc( E_FILE );
		return ( FALSE );
	}
	_lclose( hFile );

	// make to some useful data format ...
	for( i = 0;  i < pHeader->nx*pHeader->ny;  i++ ) {
		pBuffer[i] += pHeader->ns;
	}
	LadeBlock( pBmp, pBuffer, pSnom->w, pSnom->w, pSnom->h, 16, TOPO, 32766, TRUE );
	return ( TRUE );
}


//***************************************** SM2-Format *****************************************

// Liest Seiko-Datei (*.xqd)
BOOL ReadSeiko( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	WORD *pBuffer;
	LONG lEoF, lEoH, i;

	/****************************************************************************
	 *	Das Format ist nur aufgrund von Try and Error entziffert worden!
	 *	Verwendet vorzeichenlose Intel-Notation.
	 *	Lünge des Headers immer (?) 2944 Bytes, steht aber auch in der Datei
	 *
	 *	Es folgt der bis jetzt entzifferte Header: (Kennung: "SPIZ000DFM300A\0\0")
	 * $10 Long Versionsnummer? (20001)
	 * $14 Long End File
	 * $18 Long Start Daten (immer 2944?)
	 * ???
	 * $28 Nullterminierter Kommentarstring (max $70 Byte?)
	 * $98 Double x (in Snomputzkonvention (multiplikator ergibt nm)
	 * $A0 Double y
	 * $A8 Double z
	 * $B0 Motorola x value?
	 * ???
	 * $480 Comment again
	 * $500 DFM info
	 * $900 Graphics
	 * $A00 Scanner info
	 *
	 *****************************************************************************/

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );      // Immer Topografie
	pSnom->Topo.Typ = TOPO;

	_llseek( hFile, 0x14l, 0 );
	_lread( hFile, &lEoF, 4l );
	_lread( hFile, &lEoH, 4l );

	_llseek( hFile, 0x98l, 0 );
	_lread( hFile, &( pSnom->fX ), sizeof( double ) );
	_lread( hFile, &( pSnom->fY ), sizeof( double ) );
	_lread( hFile, &( pSnom->Topo.fSkal ), sizeof( double ) );

	pBmp->pPsi.iCol = pSnom->w = sqrt( ( ( lEoF-lEoH )/2+1 ) );
	pBmp->pPsi.iRow = pSnom->h = pSnom->w;

	// Original-Header lesen
	_llseek( hFile, 0l, 0 );
	pBmp->pExtra = pMalloc( lEoH );
	if( pBmp->pExtra == 0  ||  _hread( hFile, pBmp->pExtra, lEoH ) < lEoH )	{
		pBmp->Typ = SEIKO;
		pBmp->lExtraLen = lEoH;
	}

	// Daten laden
	i = lEoF-lEoH;
	_llseek( hFile, lEoH, 0 );
	pBuffer = pMalloc( i );
	if( pBuffer == 0  ||  _hread( hFile, pBuffer, i ) < i )	{
		MemFree( pBuffer );
		_lclose( hFile );
		FehlerRsc( E_FILE );
		return ( FALSE );
	}
	_lclose( hFile );

	// Und endlich konvertieren ...
	LadeBlock( pBmp, pBuffer, pSnom->w, pSnom->w, pSnom->h, 16, TOPO, 0x8000u, TRUE );
	MemFree( pBuffer );
	return ( TRUE );
}


// 1.4.00


//***************************************** SM2-Format *****************************************


// Liest SM2-Datei (*.IMG)
BOOL ReadRHK( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	BYTE c, str[34], str2[6], seperator[] = " \n\t";
	short int w = 0, h = 0;
	double f, g, faktor;
	long i, offset;
	LPVOID buffer;


	/****************************************************************************
	 *	Das Format ist nur aufgrund von Try and Error entziffert worden!
	 *	Verwendet vorzeichenlose Intel-Notation.
	 *	Lünge des Headers immer (?) 512 Bytes
	 *  Besteht aus mindestens 2 Strings, die immer genau 32 Byte lang sind!
	 *
	 *	Es folgt der bis jetzt entzifferte Header: (Kennung: "STiMage" "x.y" (Versionsnummer) + Datum+Uhrzeit)
	 *	flag flag flag Wort (Breite) Wort (Hühe) Long (Dateilünge) flag
	 *
	 *	Dann kommen evt variable Daten:
	 *	Erst Kennung:
	 *		'X ':		float (X-Scalierung pro Pixel) float (=Offset immer 0.0?) str (Einheit immer 'm'?)
	 *		'Y ':		float (Y-Scalierung pro Pixel) float (=Offset immer 0.0?) str (Einheit immer 'm'?)
	 *		'Z ':		float (Z-Scalierung pro Pixel) float (=Offset immer 0.0?) str (Einheit immer 'm'?)
	 *		'XY ':	float ???
	 *		'IV ':	float ???
	 *		'scan':	Kommentar?
	 *		'id':
	 *
	 *	Ab 33E folgen 16 Bit Intel-Worte, vorzeichenbehaftet
	 *****************************************************************************/

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );      // Immer Topografie
	pSnom->Topo.Typ = TOPO;

	offset = _llseek( hFile, 0, 2 );        // Dateilünge
	_llseek( hFile, 0l, 0 );
	str[32] = 0;    // Alle Strings 32 Byte lang, werden zur Not mit Leerzeichen aufgefüllt ...

	// Erste Zeile: Datum
	_lread( hFile, (LPVOID)str, 32 );
	sscanf( str, "%*s %*f %Fhi/%Fhi/%Fhi %Fhi:%Fhi:%Fhi", (LPUWORD)&pBmp->iMonat, (LPUWORD)&pBmp->iTag, (LPUWORD)&pBmp->iJahr, (LPUWORD)&pBmp->iStunde, (LPUWORD)&pBmp->iMinute, (LPUWORD)&pBmp->iSekunde );
	if( pBmp->iJahr < 90 ) {
		pBmp->iJahr += 2000;
	}
	else if( pBmp->iJahr < 100 ) {
		pBmp->iJahr += 1900;
	}

	// Zweite Zeile (Format: s.o.)
	_lread( hFile, (LPVOID)str, 32 );
	sscanf( str, "%*c %*c %*c %Fli %Fli %Fli", (LPLONG)&w, (LPLONG)&h, (LPLONG)&i );
	strncpy( str2, str+6, 4 );
	str2[4] = 0;
	w = atoi( str2 );
	strncpy( str2, str+10, 4 );
	str2[4] = 0;
	h = atoi( str2 );
	pSnom->w = w;
	pBmp->pPsi.iCol = w;
	pSnom->h = h;
	pBmp->pPsi.iRow = h;
	offset -= w*h*2;

	// Und nun die Strings ...
	for( i = 2;  i < 16;  i++ ) {
		_lread( hFile, (LPVOID)str, 32 );
		switch( str[0] ) {
			case 'X':
				if( str[1] == ' ' ) {
					sscanf( str+2, "%Flf %Flf %Fc", (LPDOUBLE)&f, (LPDOUBLE)&g, (LPBYTE)&c );
					if( c == 'm' ) {
						// alles in Meter ...
						faktor = 1e9;
					}
					else {
						faktor = 1.0;
					}
					f *= faktor;
					pBmp->pPsi.fW = (float)f*w;
					pSnom->fX = f;
					pSnom->Topo.fSkal = f;
				}
				break;

			case 'Y':
				if( str[1] == ' ' ) {
					sscanf( str+2, "%Flf %Flf %Fc", (LPDOUBLE)&f, (LPDOUBLE)&g, (LPBYTE)&c );
					if( c == 'm' ) {
						// alles in Meter ...
						faktor = 1e9;
					}
					else {
						faktor = 1.0;
					}
					f *= faktor;
					pBmp->pPsi.fH = (float)f*h;
					pSnom->fY = f;
				}
				break;

			case 'Z':
				if( str[1] == ' ' ) {
					sscanf( str+2, "%Flf %Flf %Fc", (LPDOUBLE)&f, (LPDOUBLE)&g, (LPBYTE)&c );
					if( c == 'm' ) {
						// alles in Meter ...
						faktor = 1e9;
					}
					else {
						faktor = 1.0;
					}
					f *= faktor;
					pSnom->Topo.fSkal = f;
				}
				break;
		}
	}

	// Original-Header lesen
	_llseek( hFile, 0l, 0 );
	pBmp->pExtra = pMalloc( offset );
	if( pBmp->pExtra == 0  ||  _hread( hFile, pBmp->pExtra, offset ) < offset ) {
		pBmp->Typ = SM2;
		pBmp->lExtraLen = offset;
	}

	// Daten laden
	i = (long)w*(long)h*sizeof( UWORD );
	_llseek( hFile, offset, 0 );
	buffer = pMalloc( i );
	if( buffer == 0  ||  _hread( hFile, buffer, i ) < i ) {
		MemFree( buffer );
		_lclose( hFile );
		FehlerRsc( E_FILE );
		return ( FALSE );
	}
	_lclose( hFile );

	// Und endlich konvertieren ...
	LadeBlock( pBmp, buffer, pSnom->w, pSnom->w, pSnom->h, 16, TOPO, 0x8000u, TRUE );
	MemFree( buffer );
	return ( TRUE );
}


// 1.4.00


//**** Schreibt RHK-Datei (Format: s.o.) ****
BOOL WriteRHK( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei )
{
	HFILE hFile;
	OFSTRUCT of;
	LPSNOMDATA pSnom = &( pBmp->pSnom[iAktuell] );
	double fSkal;
	LPUWORD	puDaten;
	LONG i;
	BYTE str[72];

	if( pBmp == NULL  ||  szDatei == NULL ) {
		return ( FALSE );
	}

	// Zeiger auf gültige Daten holen
	if( what&TOPO  &&  pSnom->Topo.puDaten != NULL ) {
		puDaten = pSnom->Topo.puDaten;
		if( (LONG)puDaten < 256L ) {    // Indirekt adressiert?
			puDaten = pBmp->pSnom[(LONG)puDaten-1].Topo.puDaten;
		}
		fSkal = pSnom->Topo.fSkal*1e-9;
		what = TOPO;
	}
	else if( what&ERRO  &&  pSnom->Error.puDaten != NULL ) {
		puDaten = pSnom->Error.puDaten;
		if( (LONG)puDaten < 256L ) {    // Indirekt adressiert?
			puDaten = pBmp->pSnom[(LONG)puDaten-1].Error.puDaten;
		}
		fSkal = pSnom->Error.fSkal*1e-9;
		what = ERRO;
	}
	else if( what&LUMI  &&  pSnom->Lumi.puDaten != NULL ) {
		puDaten = pSnom->Lumi.puDaten;
		if( (LONG)puDaten < 256L ) {    // Indirekt adressiert?
			puDaten = pBmp->pSnom[(LONG)puDaten-1].Lumi.puDaten;
		}
		fSkal = pSnom->Lumi.fSkal;
		what = LUMI;
	}
	else {
		return ( FALSE );
	}

	if( HFILE_ERROR == ( hFile = OpenFile( szDatei, &of, OF_WRITE | OF_CREATE ) ) )	{
		// FEHLERMELDUNG!
		return ( FALSE );
	}

	// Schreibt SM2-Datei (RHK-Format)
	sprintf( str, "STiMage 3.1 %02i/%02i/%i %02i:%02i:%02i   ", pBmp->iMonat, pBmp->iTag, pBmp->iJahr, pBmp->iStunde, pBmp->iMinute, pBmp->iSekunde );
	_hwrite( hFile, (LPVOID)str, 32 );
	sprintf( str, "0 1 0 %4li%4li %li 1                ", pSnom->w, pSnom->h, pSnom->w*pSnom->h*2l );
	_hwrite( hFile, (LPVOID)str, 32 );
	sprintf( str, "X %lg %lf m                     ", pSnom->fX*1e-9, 0.0 );
	_hwrite( hFile, (LPVOID)str, 32 );
	sprintf( str, "Y %lg %lf m                     ", pSnom->fY*1e-9, 0.0 );
	_hwrite( hFile, (LPVOID)str, 32 );
	sprintf( str, "Z %lg %lf m                     ", fSkal, 0.0 );
	_hwrite( hFile, (LPVOID)str, 32 );
	sprintf( str, "                                " );
	for( i = 5;  i < 16;  i++ ) {
		_hwrite( hFile, (LPVOID)str, 32 );
	}

	// Ab 512 kommen die Daten ...
	// in Signed verwandeln ...
	for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
		puDaten[i] -= 0x8000u;
	}
	_hwrite( hFile, (LPCSTR)puDaten, pSnom->w*pSnom->h*2l );
	for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
		puDaten[i] += 0x8000u;
	}
	_lclose( hFile );
	return ( what );
}


// 1.4.00


//***************************************** ECS-Format *****************************************
// Liest einen Pascalstring ab Position lOffset
BYTE ReadECSString( HFILE hFile, LONG lOffset, LPSTR str )
{
	BYTE c;
	_llseek( hFile, lOffset, 0 );
	_lread( hFile, (LPVOID)&c, 1 );
	_lread( hFile, (LPVOID)str, (unsigned char)c );
	str[c] = 0;
	return ( c );
}


// 14.5.99


#define	ECSNumOffsets	8
LONG ECSOffset[] = { 0x9C, 0xEB, 0x13A, 0x19C, 0x29A, 0x2C3, 0x2EC, 0x315 };

// Liest ECS-Datei (*.IMG)
BOOL ReadECS( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	BYTE c, str[256];
	short int w = 0, h = 0;
	double f, g, faktor = 1.0;
	long i;
	LPVOID buffer;

	/****************************************************************************
	 *	Das Format ist nur aufgrund von Try and Error entziffert worden!
	 *	Verwendet Intel-Notation.
	 *	Lünge des Headers immer 830 Bytes
	 *	Strings in Pascal-Konvention!
	 *
	 *	Es folgt der bis jetzt entzifferte Header: (Kennung: A0 00)
	 *	Wort (Breite) Wort (Hühe)
	 *		???
	 *	Es folgen Strings bei:
	 *		9C, EB, 13A, 19C, 29A, 2C3, 2EC, 315
	 *
	 *	Dabei scheinen folgende Bedeutungen zu herrschen:
	 *		 9C	Datum (Form: Monat/Tag/Jahr)
	 *		 EB	Uhrzeit (Form: Stunde:Minuten(als float))
	 *		13A ???
	 *		19C	Probenkommentar
	 *		29A	"Topography"
	 *		2C3 Stroeme etc.
	 *		2EC	Scanbereich "Scan Size: x"(float)
	 *		315 Startposition als String: "Tip position: x,y" (beides float)
	 *
	 *	Ab 33E folgen 16 Bit Intel-Worte, vorzeichenbehaftet
	 *****************************************************************************/

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );
	_llseek( hFile, 2l, 0 );
	//Versuchen wir also unser Glück
	_lread( hFile, (LPVOID)&w, 2 );
	pSnom->w = w;
	pBmp->pPsi.iCol = w;
	_lread( hFile, (LPVOID)&h, 2 );
	pSnom->h = h;
	pBmp->pPsi.iRow = h;

	// Und nun die Strings ...
	pBmp->pKommentar[0] = 0;
	for( i = 0;  i < ECSNumOffsets;  i++ ) {
		ReadECSString( hFile, ECSOffset[i], str );

		// Datum lesen?
		if( ECSOffset[i] == 0x9Cl  &&  isdigit( *str ) ) {
			sscanf( str, "%Fhi/%Fhi/%Fhi", (LPUWORD)&pBmp->iTag, (LPUWORD)&pBmp->iMonat, (LPUWORD)&pBmp->iJahr );
		}

		// Uhrzeit lesen?
		if( ECSOffset[i] == 0xEBl  &&  isdigit( *str ) ) {
			float fMinuten;

			sscanf( str, "%Fhi:%Ff", (LPUWORD)&pBmp->iStunde, (LPFLOAT)&fMinuten );
			pBmp->iMinute = (UWORD)fMinuten;
			pBmp->iSekunde = (UWORD)( ( (LONG)( fMinuten*60.0 ) )%60 );
		}

		// Waren das evt. Nutzdaten?
		if( strstr( str, "Topography" ) ) {
			pSnom->Topo.Typ = TOPO;
		}
		else if( strstr( str, "Scan Size:" ) ) {
			sscanf( str+10, "%Flf %Flf%Fc", (LPDOUBLE)&f, (LPDOUBLE)&g, (LPBYTE)&c );
			if( c == 0x8F ) {
				// alles in Angstrom
				faktor = 0.1;
			}
			f *= faktor;
			pBmp->pPsi.fW = (float)f;
			pBmp->pPsi.fH = (float)f;
			pSnom->fX = f/(float)w;
			pSnom->fY = f/(float)h;
			pSnom->Topo.fSkal = g*faktor/65536.0;
		}
		else if( strstr( str, "Tip position:" ) ) {
			sscanf( str+13, "%Flf,%Flf", (LPDOUBLE)&( pSnom->fXOff ), (LPDOUBLE)&( pSnom->fYOff ) );
			// Einheit korrigieren
			pSnom->fXOff *= faktor;
			pSnom->fYOff *= faktor;
			pBmp->pPsi.fXOff0 = (float)pSnom->fXOff;
			pBmp->pPsi.fYOff0 = (float)pSnom->fYOff;
		}
		else {
			strcat( pBmp->pKommentar, str );
			strcat( pBmp->pKommentar, "\r\n" );
		}
	}

	// Original-Header lesen
	_llseek( hFile, 0l, 0 );
	pBmp->pExtra = pMalloc( 830l );
	if( pBmp->pExtra == 0  ||  _hread( hFile, pBmp->pExtra, 830 ) < 830 ) {
		pBmp->Typ = ECS;
		pBmp->lExtraLen = 830l;
	}

	// Daten laden
	_llseek( hFile, 830l, 0 );
	i = (long)w*(long)h*sizeof( UWORD );
	buffer = pMalloc( i );
	if( buffer == 0  ||  _hread( hFile, buffer, i ) < i ) {
		MemFree( buffer );
		_lclose( hFile );
		FehlerRsc( E_FILE );
		return ( FALSE );
	}
	_lclose( hFile );

	// Und endlich konvertieren ...
	LadeBlock( pBmp, buffer, pSnom->w, pSnom->w, pSnom->h, 16, TOPO, 0x8000u, TRUE );
	MemFree( buffer );
	return ( TRUE );
}


// 14.5.99


//***************************************** Omicron-Format *****************************************


//  Liest Bilder im Omicron-Format ein
//	Format siehe weiter unten ...
BOOL ReadOmicron( LPCSTR datei, HFILE hParDatei, LPBMPDATA pBmp )
{
	HFILE hMessDatei;
	WORKMODE mode = NONE;
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	LPBYTE pcBuf, pcC, pcC2;
	BYTE str[1024];
	LONG lHeaderLen, iLen, i;

	// Ganzen Header auf einmal lesen
	lHeaderLen =	_llseek( hParDatei, 0l, 2 );
	_llseek( hParDatei, 0l, 0 );

	// Header ist Def.-müüig kleiner als 8096
	pcBuf = (LPBYTE)pMalloc( 8096l );
	if( pcBuf == NULL ) {
		_lclose( hParDatei );
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}
	lHeaderLen = _lread( hParDatei, pcBuf, lHeaderLen );
	_lclose( hParDatei );

	// Original-Header kopieren ...
	pBmp->pExtra = pcBuf;
	pBmp->Typ = OMICRON;
	pBmp->lExtraLen = lHeaderLen;

	/********************************************
	 * Das Format ist folgendes:
	 *
	 *	(Kommentarzeilen beginnen mit Semikolon)
	 *
	 *	Immer 'Schlüsselwort' Leerzeichen/Tabs :Wert
	 * Alle Daten Floats
	 *
	 *	Date												 : Datum Uhrzeit
	 *
	 *	Field X Size in nm           : Groesse gesamt
	 *	Field Y Size in nm           : Groesse gesamt
	 *	Image Size in X              : Punkte X
	 *	Image Size in Y              : Punkte Y
	 *	Increment X                  : Grüüe/Punkte in X
	 *	Increment Y                  : Grüüe/Punkte in Y
	 *	Scan Angle                   : ...
	 *	X Offset                     : ...
	 *	Y Offset                     : ...
	 *
	 * (Nach dem folgenden Schlüsselwort kommen die einzelnen Werte in Leerzeilen
	 *	Topografic Channel					 : Z *Or I!*
	 *					Scanrichtung
	 *					Minimum (-32768)
	 *					Maximum (32767)
	 *					Minimumwert in Einheit (z.B. 69.0)
	 *					Maximumwert
	 *					Faktor (=Minmumwert/Minimum)
	 *					Einheit (z.B. nm)
	 *					Name der Messdatendatei
	 *
	 * Dann kann man wieder was überspringen bis
	 *
	 *	Scan Speed                   : 162.780 (Einheit: chinesische Kartoffeln? Nein, nm/s!)
	 *
	 * Die krummen Werte werden wie folgt berechnet:
	 *
	 *								width [nm]
	 * speed = ----------------------------
	 *					n * 10^-5s * points per line
	 *
	 *	mit anderen Worte: Alle 10 Mikrosekunden wird eine Regelung vorgenommen und bei jedem n.
	 * Punkt die Z-Koordinate festgehalten und weiter bewegt.
	 *
	 * Um das ganze Verfahren zu beschleunigen, wir erst der erste Buchstabe in einer
	 * Case-Anweisung abgefragt, bevor dann die Substrings genauer untersucht werden.
	 *******************************************************************************************/

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );
	lstrcpy( pSnom->Error.strTitel, STR_ERROR );
	pcC = pcBuf;
	iLen = 0;
	while( iLen++ < lHeaderLen ) {
		if( *pcC == ';' ) {
			// Also kein gültiger Befehl!
			// Alles bis zum Zeilenende überspringen
			while( iLen++ < lHeaderLen  &&  *++pcC >= ' ' )
				;
			while( iLen++ < lHeaderLen  &&  *++pcC <= 32 )
				; // Weiter mit neuer Zeile
			continue;
		}

		// Ab hier ist es ein gültiger Befehl ...
		// in einen String kopieren
		pcC2 = str;
		for( i = 0;  i < 1024  &&  iLen++ < lHeaderLen  &&  *pcC != ':'  &&  *pcC >= ' ';  i++ ) {
			*pcC2++ = *pcC++;
		}
		*pcC2++ = 0;
		pcC2 = pcC+1;
		while( *pcC >= 32  &&  iLen++ < lHeaderLen )
			pcC++;
		while( *pcC <= 32  &&  iLen++ < lHeaderLen )
			pcC++;
		switch( str[0] ) {      // Um das Laden zu beschleunigen ...
			//****	Allgemeine Infos ****

			case 'C':
				// Version herausfinden
				if( strstr( str, "Comment" ) == str ) {
					;
				}
				break;

			case 'D':
				// Datum lesen
				sscanf( pcC2, "%Fhi.%Fhi.%Fhi %Fhi:%Fhi", (LPUWORD)&pBmp->iTag, (LPUWORD)&pBmp->iMonat, (LPUWORD)&pBmp->iJahr, (LPUWORD)&pBmp->iStunde, (LPUWORD)&pBmp->iMinute );
				pBmp->iSekunde = 0;
				if( pBmp->iJahr < 90 ) {
					pBmp->iJahr += 2000;
				}
				else if( pBmp->iJahr < 100 ) {
					pBmp->iJahr += 1900;
				}
				break;

			case 'I':
				if( strstr( str, "Image Size in X" ) == str ) {
					pSnom->w = atol( pcC2 );
				}
				else if( strstr( str, "Image Size in Y" ) == str ) {
					pSnom->h = atol( pcC2 );
				}
				else if( strstr( str, "Increment X" ) == str ) {
					pSnom->fX = atof( pcC2 );
				}
				else if( strstr( str, "Increment Y" ) == str ) {
					pSnom->fY = atof( pcC2 );
				}
				break;

			case 'S':
				// Drehwinkel (->pPsi)
				if( strstr( str, "Scan Angle" ) == str ) {
					pBmp->pPsi.fRot = atof( pcC2 );
				}
				// Scangeschwindigkeit in some obscure Unit (nm/min ?)
				else if( strstr( str, "Scan Speed" ) == str ) {
					pBmp->pPsi.fLinePerSec = atof( pcC2 );
					pBmp->pPsi.fW = pSnom->w*pSnom->fX;
					pBmp->pPsi.fH = pSnom->h*pSnom->fY;
					if( pBmp->pPsi.fW > 0 ) {
						pBmp->pPsi.fLinePerSec /= ( 2.0*pBmp->pPsi.fW );
					}
				}
				break;

			case 'X':
				// X-Offset
				if( strstr( str, "X Offset" ) == str ) {
					pBmp->pPsi.fXOff0 = pSnom->fXOff = atof( pcC2 );
				}
				break;

			case 'Y':
				// Y-Offset (->pPsi)
				if( strstr( str, "Y Offset" ) == str ) {
					pBmp->pPsi.fYOff0 = pSnom->fYOff = atof( pcC2 );
				}
				break;

			//**** Infos des nüchsten zu ladenen Bildes ****
			case 'T':
				// Lünge des Headers
				if( strstr( str, "Topographic Channel" ) == str ) {
					LPUWORD	pPtr;
					LONG lDiff, lMin;
					double fScale;

					mode = NONE;
					while( *pcC2 == ' ' )
						pcC2++;
					if( *pcC2 == 'I' ) {
						mode = ERRO;
					}
					else if( *pcC2 == 'Z' ) {
						mode = TOPO;
					}
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' )
						;
					pBmp->pPsi.x_dir = ( *pcC == 'F' );
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ;
					lDiff = -( lMin = atol( pcC ) );
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ;
					lDiff += atol( pcC );
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ;
					fScale = -atof( pcC );
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ;
					fScale += atof( pcC );
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ; // Resolution, geht genauer anders ... :
					if( mode == ERRO ) {
						pSnom->Error.fSkal = fScale/(double)lDiff;
					}
					else if( mode == TOPO ) {
						pSnom->Topo.fSkal = fScale/(double)lDiff;
					}
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ; // Hoffentlich in nm, wird noch nicht getestet!
					if( mode == ERRO ) {
						pcC2 = pSnom->Error.strZUnit;
					}
					else if( mode == TOPO ) {
						pcC2 = pSnom->Topo.strZUnit;
					}
					while( iLen++ < lHeaderLen  &&  *pcC > ' ' )
						*pcC2++ = *pcC++;
					*pcC2 = 0;
					while( iLen++ < lHeaderLen  &&  *pcC++ >= ' ' ) ;
					while( iLen++ < lHeaderLen  &&  *++pcC <= ' ' ) ; // Jetzt sollte pcC auf den Beginn des Filenamens zeigen
					lstrcpy( str, datei );
					pcC2 = strrchr( str, '\\' );
					if( pcC2 == 0 ) {
						pcC2 = str;
					}
					else {
						pcC2++;
					}
					while( iLen++ < lHeaderLen  &&  *pcC > ' ' )
						*pcC2++ = *pcC++;
					*pcC2 = 0;
					// Dateiname nun in datei => Daten laden
					if( ( hMessDatei = _lopen( str, OF_READ|OF_SHARE_DENY_WRITE ) ) == -1 )	{
						_lclose( hMessDatei );
						MemFree( pcBuf );
						FehlerRsc( E_FILE );
						return ( FALSE );
					}
					pPtr = (LPUWORD)pMalloc( pSnom->w*pSnom->h*2l );
					if( pPtr == NULL ) {
						_lclose( hMessDatei );
						MemFree( pcBuf );
						FehlerRsc( E_MEMORY );
						return ( FALSE );
					}
					_hread( hMessDatei, pPtr, pSnom->w*pSnom->h*2l );
					_lclose( hMessDatei );
					for( i = 0;  i < pSnom->w*pSnom->h; i++ ) {
						pPtr[i] = Big2Little( pPtr[i] );
					}
					LadeBlock( pBmp, pPtr, pSnom->w, pSnom->w, pSnom->h, 16, mode, lMin, TRUE );
					// und noch schnell ein paar Initialisierungen ...
				}
				break;
		} // switch
	} // Kein Header definiert: Nichts tun ...
	return ( TRUE );
}


// 22.12.98


//***************************************** Digital-Format *****************************************

LPSTR sAllMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

//**** Schreibt Digital-Datei (Format: s.u.) ****
BOOL WriteDigital( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei )
{
	HFILE hFile;
	OFSTRUCT of;
	LPSNOMDATA pSnom = &( pBmp->pSnom[iAktuell] );
	LPUWORD	puTopo = NULL, puError = NULL, puLumi = NULL;
	LONG lOffset = DEFAULT_DIGITAL_HEADER_LEN;
	LONG i = 0;
	LPBYTE sHeader;
	BYTE str[256];

	if( pBmp == NULL  ||  szDatei == NULL ) {
		return ( FALSE );
	}

	// Zuerst ermitteln, wieviele Datenfiles es eigentlich zu speichern gibt ...
	// Zeiger auf gültige Daten holen
	if( what&TOPO  &&  pSnom->Topo.puDaten != NULL ) {
		puTopo = pSnom->Topo.puDaten;
		if( (LONG)puTopo < 256L ) {     // Indirekt adressiert?
			puTopo = pBmp->pSnom[(LONG)puTopo-1].Topo.puDaten;
		}
		i++;
	}
	if( what&ERRO  &&  pSnom->Error.puDaten != NULL ) {
		puError = pSnom->Error.puDaten;
		if( (LONG)puError < 256L ) {    // Indirekt adressiert?
			puError = pBmp->pSnom[(LONG)puError-1].Error.puDaten;
		}
		i++;
	}
	if( what&LUMI  &&  pSnom->Lumi.puDaten != NULL ) {
		puLumi = pSnom->Lumi.puDaten;
		if( (LONG)puLumi < 256L ) {     // Indirekt adressiert?
			puLumi = pBmp->pSnom[(LONG)puLumi-1].Lumi.puDaten;
		}
		i++;
	}

	if( i == 0 ) {
		return ( FALSE );   // Nix zu speichern!
	}
	if( HFILE_ERROR == ( hFile = OpenFile( szDatei, &of, OF_WRITE | OF_CREATE ) ) )	{
		// FEHLERMELDUNG!
		return ( FALSE );
	}

	sHeader = pMalloc( DEFAULT_DIGITAL_HEADER_LEN );

	// File-Header ...
	strcpy( sHeader, "\\*File list\xD\xA\\Version: 0x04230203\xD\xA" );
	sprintf( str, "\\Date: %02i:%02i:%02i %Fs ??? %Fs %2i %i\xD\xA", pBmp->iStunde%12, pBmp->iMinute, pBmp->iSekunde, (LPSTR)( ( pBmp->iStunde > 12 ) ? "PM" : "AM" ), (LPSTR)( sAllMonth[pBmp->iMonat-1] ), pBmp->iTag, pBmp->iJahr );
	strcat( sHeader, str );
	if( i > 1 ) {
		sprintf( str, "\\Start context: OL%i\xD\xA", i );
	}
	else {
		sprintf( str, "\\Start context: OL\xD\xA" );
	}
	strcat( sHeader, str );
	sprintf( str, "\\Data length: %li\xD\xA", DEFAULT_DIGITAL_HEADER_LEN );
	strcat( sHeader, str );
	// Vorgeplünkel
	strcat( sHeader, "\\*NC Afm list\xD\xA\\Operating mode: Image\xD\xA\\Non-square scan: Yes\xD\xA" );
	sprintf( str, "\\Scan size: %lg nm\xD\xA", pSnom->fX*pSnom->w );
	strcat( sHeader, str );
	sprintf( str, "\\Aspect ratio: 1:%.1lf\xD\xA", ( pSnom->fX*pSnom->w )/( pSnom->fY*pSnom->h ) );
	strcat( sHeader, str );
	sprintf( str, "\\X offset: %.2lf nm\xD\xA", pSnom->fXOff );
	strcat( sHeader, str );
	sprintf( str, "\\Y offset: %.2lf nm\xD\xA", pSnom->fYOff );
	strcat( sHeader, str );
	sprintf( str, "\\Rotate Ang.: %g\xD\xA", pBmp->pPsi.fRot );
	strcat( sHeader, str );
	sprintf( str, "\\Samps/line: %li\xD\xA", pSnom->w );
	strcat( sHeader, str );
	sprintf( str, "\\Lines: %li\xD\xA", pSnom->h );
	strcat( sHeader, str );
	sprintf( str, "\\Scan rate: %f\xD\xA", pBmp->pPsi.fLinePerSec );
	strcat( sHeader, str );
	strcat( sHeader, "\\Sample period: 160\xD\xA\\Units: Metric\xD\xA\\Color Table: 2\xD\xA\\Y disable: Enabled\xD\xA\\Z atten.: 65536\xD\xA\\Input attenuation mode: 1x\xD\xA" );
#if 0
	// Scheinbar braucht's auch noch 'nen Microscope und 'nen Controller-Header ...
	strcat( sHeader, "\\*Microscope list\xD\xA\\Id: SnomPutz\xD\xA\\Linearization mode: Normal\xD\xA" );
	strcat( sHeader, "\\*Controller list\xD\xA\\In1 max: 10\xD\xA\\In2 max: 10\xD\xA\\Sample max: 6553.6\xD\xA" );
#endif
	// Die unterschiedlichen Daten und die Binürdaten schreiben
	while( i-- > 0 ) {
		// Für alle Bilder mehrmals
		strcat( sHeader, "\\*NCAFM image list\xD\xA" );
		sprintf( str, "\\Data offset: %li\xD\xA\\Data length: %li\xD\xA", lOffset, pSnom->w*pSnom->h*2 );
		strcat( sHeader, str );
		strcat( sHeader, "\\Start context: OL\xD\xA\\Data type: AFM\xD\xA" );
		if( pBmp->pPsi.y_dir ) {
			strcat( sHeader, "\\Frame direction: Up\xD\xA" );
		}
		else {
			strcat( sHeader, "\\Frame direction: Down\xD\xA" );
		}

		sprintf( str, "\\Samps/line: %li\xD\xA", pSnom->w );
		strcat( sHeader, str );
		sprintf( str, "\\Number of lines: %li\xD\xA", pSnom->h );
		strcat( sHeader, str );
		sprintf( str, "\\Aspect ratio: 1:%.1lf\xD\xA", ( pSnom->fX*pSnom->w )/( pSnom->fY*pSnom->h ) );
		strcat( sHeader, str );
		sprintf( str, "\\Scan size: %lg nm\xD\xA", pSnom->fX*pSnom->w );
		strcat( sHeader, str );
		if( pBmp->pPsi.x_dir ) {
			strcat( sHeader, "\\Line direction: Trace\xD\xA" );
		}
		else {
			strcat( sHeader, "\\Line direction: Retrace\xD\xA" );
		}
		// Bis hierhin für alle Daten gleich

		if( puTopo != NULL ) {
			strcat( sHeader, "\\Image data: Height\xD\xA" );
			sprintf( str, "\\Z magnify image: %.7lg\xD\xA", pSnom->Topo.fSkal*655.36 );
			strcat( sHeader, str );
			sprintf( str, "\\Z scale: %.7lg nm (65536)\xD\xA", pSnom->Topo.fSkal*65536.0 );
			strcat( sHeader, str );
			_llseek( hFile, lOffset, 0 );
			_hwrite( hFile, puTopo, pSnom->w*pSnom->h*sizeof( UWORD ) );
			puTopo = NULL;
			lOffset += pSnom->w*pSnom->h*sizeof( UWORD );
		}
		else if( puError != NULL ) {
			strcat( sHeader, "\\Image data: Deflection\xD\xA" );
			sprintf( str, "\\Z magnify image: %.7lg\xD\xA", pSnom->Error.fSkal*655.36 );
			strcat( sHeader, str );
			sprintf( str, "\\Z scale: %.7lg nm (65536)\xD\xA", pSnom->Error.fSkal*65536.0 );
			strcat( sHeader, str );
			_llseek( hFile, lOffset, 0 );
			_hwrite( hFile, puError, pSnom->w*pSnom->h*sizeof( WORD ) );
			puError = NULL;
			lOffset += pSnom->w*pSnom->h*sizeof( UWORD );
		}
		else if( puLumi != NULL ) {
			strcat( sHeader, "\\Image data: Luminescence\xD\xA" );
			sprintf( str, "\\Z magnify image: %.7lg\xD\xA", pSnom->Lumi.fSkal*655.36 );
			strcat( sHeader, str );
			sprintf( str, "\\Z scale: %.7lg nW (65536)\xD\xA", pSnom->Lumi.fSkal*65536.0 );
			strcat( sHeader, str );
			_llseek( hFile, lOffset, 0 );
			_hwrite( hFile, puLumi, pSnom->w*pSnom->h*sizeof( WORD ) );
			puLumi = NULL;
			lOffset += pSnom->w*pSnom->h*sizeof( UWORD );
		}
		strcat( sHeader, "\\Realtime Planefit: Line\xD\xA\\Offline planefit: Offset\xD\xA\\Highpass: 0\xD\xA\\Lowpass: 0\xD\xA" );
	}
	_llseek( hFile, 0l, 0 );
	_hwrite( hFile, sHeader, 2048 );
	_lclose( hFile );

	return ( TRUE );
}
// 5.4.2000


//  Liest eine Datei im Digital-Format ein

//	Format siehe weiter unten ...
BOOL ReadDigital( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	LPBYTE pcBuf, pcC, pcC2;
	BYTE str[1024], str2[16];
	int iLen = 0, i;
	LONG lData_start = 0l, lHeaderLen = 0, lData_len = 0l, lVersion = 0l;                   // Um Alte und Neue zu trennen
	WORKMODE Header = 0;
	LONG lModuloSkal = 0l;
	double lfX, lfY, lfAspect = 1.0;        // X zu Y

	double z_sense_per_volt = 1.0;	// needed for new format
	double z_defl_per_volt = 1.0;	// needed for new format

	// Einige Defaultwerte eintagen
	pBmp->pPsi.fRot = 0.0;
	// header shorter than 65535 (hopefully)
	pcBuf = (LPBYTE)pMalloc( MAX_DI_HDLEN );
	if( pcBuf == NULL ) {
		FehlerRsc( E_MEMORY );
		return ( FALSE );
	}

	// Ganzen Header auf einmal lesen
	_llseek( hFile, 0l, 0 );
	_lread( hFile, pcBuf, MAX_DI_HDLEN );

	/********************************************
	 * Das Format ist folgendes:
	 * Die Dateien sind in drei Teile gegliedert.
	 * Beim neuen Format steht vorher noch eine Versionsnummer (ein hexadezimaler String)
	 *
	 * Dann folgend die einzelnen Sektionen. Eine Sektion wird mit "\*" eingeleitet.
	 * Zuerst kommt
	 *		\*File list
	 *		\Version: 0x04230203 (sollte wohl größer 4 sein, wird ignoriert)
	 *		\Date: 03:14:05 PM Thu Apr 16 1998 (Datum der Messung)
	 *		\Start context: OL2	 (???)
	 *		\Data length: 8192	 Länge des Headers, Aendert sich von Version zu Version!
	 *
	 * Dann mit "\*NC Afm list:" die allgemeinen Einstellungen. Hier sind erst einmal nur
	 * drei Dinge interessant: (f:float, x:integer, s:string)
	 * Ab Version 0x044... ist es "\Ciao scan list"
	 *
	 *		(folgende finden sich auch wieder unten ...)
	 *   \Scan size: f s      Scangröße (real) + Einheit (nm oder um=µm)
	 *   \Aspect ratio: f:f   Größenverhältnis (x zu y oder y zu x?)
	 *   \X offset: f(s)			 X-Offset (s=Einheit (nm!))
	 *   \Y offset: f(s)			 X-Offset (s=Einheit (nm!))
	 *
	 *   \Rotate Ang.: f      Scanwinkel
	 *   \Scan rate: f        Scanfrequenz (Hz)
	 *   \AFM mode: s         Modus (z.B. Contact oder Tapping)
	 *
	 * Die nun folgenden Sektionen sind erstmal uninteressant. Es sind im einzelnen:
	 *		\*Microscope list (Details zu Scanner etc.)
	 *		\*Controller list (Details zum Controller)
	 *
	 * Erst bei "\*NCAFM image list:" wir es
	 * wieder interessant. Hier beginnen jetzt die eigentlichen Bilddaten.
	 * Aber Version 0x044 oder so heisst der Parameter "\*Ciao image list"
	 *
	 *   \X offset: f(s)			 X-Offset (s=Einheit (nm!))
	 *   \Y offset: f(s)			 X-Offset (s=Einheit (nm!))
	 *   \Samps/line: x       Anzahl Punkte pro Zeile (alt: Punkte pro Zeile + Zeilen!)
	 *   \Number of lines: x  Anzahl Zeilen (nur neu)
	 *   \Aspect ratio: f:f   Größenverhältnis (x zu y oder y zu x?)
	 *   \Scan size: f s      Scangröße (real) + Einheit (nm oder um/~m=üm)
	 *												 (V>4.2 \Scan size: f f s)
	 *   \Z magnifiy image: f nur interne Größe (alt: Skalierung?)
	 *   \Z sensitivity: f    ???
	 *   \Z scale: f s (x)    Länge mit Einheit (nur neu)
	 *								 Dabei gilt: fSkal = f/65536.0
	 *								 (x) gibt die Quantelung der Daten an: Abstand ist Vielfaches von 65536/x
	 *   \Z scale height:     Weite Z-Skala (nur alt)
	 *   \Line direction: s   Scanrichtung (Trace oder Retrace)
	 *   \Frame direction: s  von unten nach oben oder von oben nach unten
	 *   \Image data: s       möglich sind hier speziell "Height", "Deflection", ... ?
	 *   \Data Offset: x      Start der Binärdaten
	 *   \Data length: x      und deren Lünge
	 *
	 *	Neue Formate: (V4.4 und hoeher)
	 *   \Scan size: f f s    Scangröße (real) x z Einheit (nm oder ~m=üm)
	 *		\@2:Image Data: S [Height] "Height" Art der Daten und Name
	 *		\@Z magnify: C [x:Z scale] f x is always 2?
	 *   \@Z scale: V [Sens. Zscan] (f V/LSB) x V x=65536, f=x/65536.0
	 *	Im Instrumentheader(!) stand
	 *   \@Sens. Zscan: V f nm/V	(Maybe final data is SensZscan*ZmagnifyX*ZmagnifyZscale*ZscaleFlsb)
	 *   \@Sens. Deflection: V 183.9473 nm/V dito.
	 *
	 * Newest version
	 *   in general header
	 * 	 \@Sens. Zsens: V f nm/V
	 *   in Ciao image ...
	 *   \@2:Z scale: V [Sens. Zsens] (f V/LSB) f V
	 *   to get a z scale take the frist float to get the z-sense and multiply it by the full scale (last from ciao element) and divide by 65535
	 *
	 * Neue Erkenntnisse sind willkommen ...
	 *
	 * Um das ganze Verfahren zu beschleunigen, wird erst der erste Buchstabe in einer
	 * Case-Anweisung abgefragt, bevor dann die Substrings genauer untersucht werden.
	 *******************************************************************************************/

	lstrcpy( pSnom->Topo.strTitel, STR_TOPO );
	lstrcpy( pSnom->Error.strTitel, STR_ERROR );
	lstrcpy( pSnom->Lumi.strTitel, STR_LUMI );

	pcC = pcBuf;
	while( iLen++ < MAX_DI_HDLEN ) {
		if( *pcC != '\\'  &&  *pcC != 0x1A ) {
			// Also kein gültiger Befehl und nicht Dateiende!
			// Alles bis zum Zeilenende überspringen
			while( iLen++ < MAX_DI_HDLEN  &&  *pcC++ >= ' ' )
				;
			while( iLen++ < MAX_DI_HDLEN  &&  *++pcC < 32 )
				; // Weiter mit neuer Zeile
			continue;
		}

		// Ab hier ist es ein gültiger Befehl ...
		// in einen String kopieren
		str[0] = *pcC++;
		pcC2 = str+1;
		for( i = 0;  i < 1024  &&  iLen++ < MAX_DI_HDLEN  &&  *pcC >= ' ';  i++ ) {
			*pcC2++ = *pcC++;
		}
		while( *pcC <= 13  &&  iLen++ < MAX_DI_HDLEN )
			pcC++;
		*pcC2++ = 0;

		switch( str[1] ) {      // Um das Laden zu beschleunigen ...
			//****	Allgemeine Infos ****

			case 'V':
				// Version herausfinden
				if( strstr( str, "\\Version: " ) == str ) {
					lVersion = strtol( str+10, NULL, 0 );
				}
				break;

			case 'R':
				// Drehwinkel (->pPsi)
				if( strstr( str, "\\Rotate ang.: " ) == str ) {
					pBmp->pPsi.fRot = atof( str+14 );
				}
				break;

			case 'S':
				// Drehwinkel (->pPsi)
				if( strstr( str, "\\Scan rate: " ) == str ) {
					pBmp->pPsi.fLinePerSec = atof( str+12 );
				}
				// (alt: columns rows) Colums
				else if( strstr( str, "\\Samps/line: " ) == str ) {
					if( lVersion ) {
						pSnom->w = atol( str+13 );
					}
					else {
						sscanf( str+13, "%Fli %Fli", (LPLONG)&( pSnom->w ), (LPLONG)&( pSnom->h ) );
					}
				}
				// Ausmaüe (hoffentlich!)
				else if( strstr( str, "\\Scan size: " ) == str  ||  strstr( str, "\\Scan Size: " ) == str ) {
					if( lVersion < 0x04400000l  ||  3!=sscanf( str+12, "%Flf %Flf %Fs", (LPDOUBLE)&lfX, (LPDOUBLE)&lfY, (LPSTR)str2 )   ) {
						sscanf( str+12, "%Flf %Fs", (LPDOUBLE)&lfX, (LPSTR)str2 );
						lfY = lfX;
					}
					if( ReadWord( str2 ) == ReadWord( "um" )  ||  ReadWord( str2 ) == ReadWord( "~m" ) ) {
						lfX *= 1000.0;
						lfY *= 1000.0;
					}
				}
				break;

			case 'X':
				// X-Offset (wird nur in PS-Struktur eingetragen)
				if( strstr( str, "\\X offset: " ) == str ) {
					sscanf( str+11, "%Flf %Fs", (LPDOUBLE)&( pSnom->fXOff ), (LPSTR)str2 );
					if( ReadWord( str2 ) == ReadWord( "um" ) ) {
						pSnom->fXOff *= 1000.0;
					}
				}
				break;

			case 'Y':
				// Y-Offset (->pPsi)
				if( strstr( str, "\\Y offset: " ) == str ) {
					sscanf( str+11, "%Flf %Fs", (LPDOUBLE)&( pSnom->fYOff ), (LPSTR)str2 );
					if( ReadWord( str2 ) == ReadWord( "um" ) ) {
						pSnom->fYOff *= 1000.0;
					}
				}
				break;

			//**** Infos des nächsten zu ladenen Bildes ****
			case 'D':
				// Datum lesen
				if( strstr( str, "\\Date: " ) == str ) {
					// sscanf wollte nicht ...
					pBmp->iStunde = atoi( str+7 );
					pBmp->iMinute = atoi( str+10 );
					pBmp->iSekunde = atoi( str+13 );
					if( str[16] == 'P' ) {
						pBmp->iStunde += 12;
					}
					for( i = 0;  i < 12;  i++ ) {
						if( strstr( str+23, sAllMonth[i] ) != NULL ) {
							pBmp->iMonat = i+1;
							break;
						}
					}
					sscanf( (LPSTR)( str+27 ), "%Fhi %Fhi", (LPUWORD)&pBmp->iTag, (LPUWORD)&pBmp->iJahr );
				}

				// start of next data
				if( strstr( str, "\\Data offset: " ) == str ) {
					lData_start = atol( str+14 );
				}
				// length of next data
				else if( strstr( str, "\\Data length: " ) == str ) {
					lData_len = atol( str+14 );
					if( lHeaderLen == 0 ) {
						lHeaderLen = lData_len;
						pBmp->pExtra = pMalloc( lHeaderLen );
						if( pBmp->pExtra == 0 )	{ // Save Header as extradata
							pBmp->Typ = DIGITAL;
							MoveMemory( pBmp->pExtra, pcBuf, lHeaderLen );
							pBmp->lExtraLen = lHeaderLen;
						}
					}
				}

			case 'I':
				// Was für ein Header eigentlich? (Datentyp)
				if( strstr( str, "\\Image data: " ) == str ) {
					if( strstr( str+13, "Height" ) ) {
						Header = TOPO;
					}
					else if( strstr( str+13, "Deflection" )  ||  strstr( str+13, "Amplitude" ) ) {
						Header = ERRO;
						lstrcpy( pSnom->Error.strTitel, str+13 );
					}
					else if( strstr( str+13, "Luminescence" ) ) {
						Header = LUMI;
						lstrcpy( pSnom->Lumi.strTitel, str+13 );
					}
				}
				else if( strstr( str, "\\Image Data: " ) == str ) {
					BYTE *c = str+13;
					while(  *c!='\"'  &&  *c  ) c++;
					c++;
					c[strlen(c)-1] = 0;
					if( strstr( str+13, "Height" ) ) {
						Header = TOPO;
						lstrcpy( pSnom->Topo.strTitel, c );
					}
					else if( strstr( str+13, "Deflection" )  ||  strstr( str+13, "Amplitude" ) ) {
						Header = ERRO;
						lstrcpy( pSnom->Error.strTitel, c );
					}
					else if( strstr( str+13, "Luminescence" ) ) {
						Header = LUMI;
						lstrcpy( pSnom->Lumi.strTitel, c );
					}
				}
				break;

			case 'N':
				// rows (neu!)
				if( strstr( str, "\\Number of lines: " ) == str ) {
					pSnom->h = atol( str+18 );
				}
				break;

			case 'A':
				// Ausmasse
				if( strstr( str, "\\Aspect ratio: " ) == str  ||  strstr( str, "\\Aspect Ratio: " ) == str ) {
					sscanf( str+15, "%Flf%*c%Flf", (LPDOUBLE)&lfAspect, (LPDOUBLE)&lfY );
					lfAspect /= lfY;
				}
				break;

			case '@':       // New Header (wozu gab es eigentlich die alten Felder???)
				if( strstr( str, "\\@Sens. Zsens: V " ) == str  ) {
					z_sense_per_volt = atof( str+17 );
				}
				else if( strstr( str, "\\@2:Z scale: V [Sens. Zsens]" ) == str  ) {
					char *c = strrchr( str, ')' );
					if(  c  ) {
						pSnom->Topo.fSkal = z_sense_per_volt*atof( c+1 )/65535.0;
					}
				}
				else if( strstr( str, "\\@Z magnify: " ) == str ) { // Beispiel: "\@Z magnify: C [2:Z scale] 0.3451050 "
					                                      // Verstaerkung, nm pro pixel!
					BYTE *c = strstr( str, "]" );
					if( Header == TOPO ) {
						pSnom->Topo.fSkal = atof( c+1 )/20;
					}
					if( Header == ERRO ) {
						pSnom->Error.fSkal = atof( c+1 )/1000;
					}
					if( Header == LUMI ) {
						pSnom->Lumi.fSkal = atof( c+1 )/1000;
					}
				}
				else if( strstr( str, "\\@2:Image Data: " ) == str ) { // Art der Daten
					// Beispiel "\@2:Image Data: S [Height] "Height""
					BYTE *t = NULL, *s = strstr( str, " \"" );
					if( strstr( str, " [Height]" ) ) {
						Header = TOPO;
						t = pSnom->Topo.strTitel;
					}
					else if( strstr( str, "[Deflection" )  ||  strstr( str, "Error]" )  ) {
						Header = ERRO;
						t = pSnom->Error.strTitel;
					}
					else if( pSnom->Lumi.puDaten==NULL  ) {
						Header = LUMI;
						t = pSnom->Lumi.strTitel;
					}
					// Copy title name
					if( s != NULL  &&  t != NULL ) {
						s += 2;
						while( *s != '\"' )
							*t++ = *s++;
						*t = 0;
					}
				}
				break;

			case 'Z':
				// Z-Verstaerkung (hoffentlich in nm)
				if( lVersion  &&  strstr( str, "\\Z scale: " ) == str )	{
					if( Header == TOPO ) {
						LPSTR c1 = str+10, c2 = pBmp->pPsi.cZGainUnit;

						pSnom->Topo.fSkal = atof( str+10 )/65536.0;       // nun in nm
						while( *c1 > ' ' )
							c1++;
						c1++;
						if( *c1 == '~' ) {      // üm?
							pSnom->Topo.fSkal *= 1000.0;    // nun in üm
							*c2++ = 'ü';
							*c1++;
						}
						while( *c1 > ' ' )
							*c2++ = *c1++;
						*c2 = 0;
						while( *c1 >= ' '  &&  *c1 != '(' )
							c1++;
						c1++;
						lModuloSkal = atol( (LPSTR)c1 );
					}
					if( Header == ERRO ) {
						LPSTR c1 = str+10;
						pSnom->Error.fSkal = atof( str+10 )/65536.0;      // nun in nm
						while( *c1 > ' '  &&  *c1 != '(' )
							c1++;
						if( *c1 == '~' ) {      // üm?
							pSnom->Error.fSkal *= 1000.0;   // nun in üm
						}
						while( *c1 >= ' '  &&  *c1 != '(' )
							c1++;
						c1++;
						lModuloSkal = atol( (LPSTR)c1 );
					}
					if( Header == LUMI ) {
						LPSTR c1 = str+10;
						pSnom->Lumi.fSkal = atof( str+10 )/65536.0;       // nun in nm
						while( *c1 > ' '  &&  *c1 != '(' )
							c1++;
						while( *c1 >= ' '  &&  *c1 != '(' )
							c1++;
						c1++;
						lModuloSkal = atol( (LPSTR)c1 );
					}
				}
				// Z-Verstürkung alt (hoffentlich!) in 1/100 nm
				else if( !lVersion  &&  strstr( str, "\\Z magnify image: " ) == str ) {
					if( Header == TOPO ) {
						pSnom->Topo.fSkal = atof( str+18 )/655.36;        // nun in nm
					}
					if( Header == ERRO ) {
						pSnom->Error.fSkal = atof( str+18 )/655.36;       // nun in nm
					}
					if( Header == LUMI ) {
						pSnom->Lumi.fSkal = atof( str+18 )/655.36;        // nun in nW
					}
				}

				break;

			case 'F':
				// Oben nach unten?
				if( strstr( str, "\\Frame direction: " ) == str ) {
					pBmp->pPsi.y_dir = ( strstr( str+18, "Down" ) == NULL );
				}
				break;

			case 'L':
				// Links nach recht?
				if( strstr( str, "\\Line direction: " ) == str ) {
					if( Header == TOPO ) {
						pBmp->pPsi.x_dir = ( strstr( str+17, "Trace" ) == NULL );
					}
					// Die Richtung des Fehlersignals interessiert uns nicht.
				}
				break;
		} // switch

		// newer files always terminate by "File list end"
		if(  strstr( str, "\\*NCAFM image list" ) == str  ||  strstr( str, "\\*Ciao image list" ) == str  ||  strstr( str, "\\**File list end" ) == str  ||  *str == 0x1A ) {
			if( Header ) {
				LONG lOldpos = _llseek( hFile, 0l, 1 );
				LPBYTE pPtr;

				// XY-Skalierung
				pSnom->fY = lfX/( lfAspect*(float)pSnom->h );
				pSnom->fX = lfX/(float)pSnom->w;

				// Daten jetzt einsetzen und laden
				if( pSnom->w == 0  ||  pSnom->h == 0  ||  lData_start != _llseek( hFile, lData_start, 0 )  ||  ( pSnom->w*pSnom->h*2l ) > lData_len ) {
					_lclose( hFile );
					MemFree( pcBuf );
					FehlerRsc( E_FILE_CORRUPT );
					return ( FALSE );
				}

				// Daten lesen
				pPtr = (LPBYTE)pMalloc( lData_len );
				if( pPtr == NULL ) {
					_lclose( hFile );
					MemFree( pcBuf );
					FehlerRsc( E_MEMORY );
					return ( FALSE );
				}

				_hread( hFile, pPtr, lData_len	);
				LadeBlock( pBmp, pPtr, pSnom->w, pSnom->w, pSnom->h, 16, Header, 0x8000u, TRUE );
				if( lModuloSkal > 0 ) {
					BildCalcConst( GetBildPointer( pBmp, Header ), pSnom->w, pSnom->h, '/', 65536.0/(double)lModuloSkal, FALSE );
				}
				lModuloSkal = 0;
				MemFree( pPtr );
				_llseek( hFile, lOldpos, 0 );
			}
			Header = FALSE;
		}
	} // Kein Header definiert: Nichts tun ...
	MemFree( pcBuf );
	_lclose( hFile );
	return ( TRUE );
}
// 6.8.97



//  Liest eine Datei im Gwyddionformat ein
long get_gwygstr( HFILE hFile, char *dest, long len )
{
	long i = 0;
	while(  len>0  &&  _lread( hFile, dest, 1 )==1  ) {
		if(  *dest == 0  ) {
			return i;
		}
		dest ++;
		i ++;
		len --;
	}
	*dest = 0;
	return i;
}


BOOL ReadGwyddion( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );

	LPDOUBLE pPtr = NULL;
	double xscale, yscale, zscale, zmag;
	long height, width;
	int version = 0, current_section = 3;
	long lHeaderLen;
	char str[1024];

	pSnom->fX = 0.0;

	_llseek( hFile, 0l, 0 );
	get_gwygstr( hFile, str, 1024 );
	_lread( hFile, &lHeaderLen, 4 );

	// ok now find out about version
	if (strncmp(str, "GWYO", 4) == 0) {
        version = 1;
	}
	else if (strncmp(str, "GWYP", 4) == 0) {
        version = 2;
	}
	else if (strncmp(str, "GWYQ", 4) == 0) {
        version = 3;
	}
	else {
		FehlerRsc( E_FILE_CORRUPT );
		_lclose( hFile );
		return ( FALSE );
	}

	// ok, now we can start with the parameters ...
	while(1) {
		char typ, str2[1024];
		long value=0, size=0, dummy=0;
		double fvalue;
		str[2] = 0;

		// name/path
		get_gwygstr( hFile, str, 1024 );
		if(  _lread( hFile, &typ, 1 )!=1  ) {
			break;
		}

		switch( typ ) {
			case 'b':	_lread( hFile, &str2, 1 );
						value = str2[0];
						break;
			case 'i':	_lread( hFile, &value, 4 );
						break;
			case 'd':	_lread( hFile, &fvalue, 8 );
						break;
			case 's':	get_gwygstr( hFile, str2, 1024 );
						break;
			case 'o':	get_gwygstr( hFile, str2, 1024 );
						_lread( hFile, &size, 4 );
						if(  version==3  ) {
							_lread( hFile, &dummy, 4 );
						}
						break;
			case 'D':	// now an array with double datas ... (the image)
						_lread( hFile, &value, 4 );	// the actual size
						if(  value==0  ) {
							break;
						}
						if(  current_section>2  ) {
							// ignore additional sections ...
							_llseek( hFile, value*8, 1 );
							continue;
						}
						if(  value==width*height  ) {
							LPUWORD pData = (LPUWORD)pMalloc( value*sizeof(UWORD) );
							if(  pData == NULL  ) {
								_lclose( hFile );
								FehlerRsc( E_MEMORY );
								return ( FALSE );
							}
							pPtr = (LPDOUBLE)pMalloc( value*8 );
							if(  pPtr == NULL  ) {
								_lclose( hFile );
								MemFree( pData );
								FehlerRsc( E_MEMORY );
								return ( FALSE );
							}
							//_lread( hFile, pPtr, width*height*sizeof(double) );
							{
								DWORD len=0;
								ReadFile( hFile, pPtr, value*8, &len, NULL );
								if(  value*sizeof(double)!=len  ) {
									_lclose( hFile );
									MemFree( pData );
									MemFree( pPtr );
									FehlerRsc( E_FILE_CORRUPT );
									return ( FALSE );
								}
							}
							// now build the current header
							pSnom->w = width;
							pSnom->h = height;
							if(  pSnom->fX==0.0  ) {
								pSnom->fX = 1e9*xscale/(double)width;
								pSnom->fY = 1e9*yscale/(double)height;
							}
							// do we have the z-scale?
							if(  zscale!=0.0  ) {
								int i, mode = (1 << current_section);
								double fmax = -1e200, fmin = 1e200;
								for(  i=0;  i<value;  i++  ) {
									if(  pPtr[i] > fmax  ) {
										fmax = pPtr[i];
									}
									if(  pPtr[i] < fmin  ) {
										fmin = pPtr[i];
									}
								}
								for(  i=0;  i<value;  i++  ) {
									pData[i] = (32760.0*pPtr[i]/(fmax-fmin)+0.5);
								}
								
								LadeBlock( pBmp, pData, width, width, height, 16, mode, 0, TRUE );
								if(  current_section==0  ) {
									pSnom->Topo.fSkal = (fmax-fmin)/32760.0e-9;
								}
								else if(  current_section==1  ) {
									pSnom->Error.fSkal = (fmax-fmin)/32760.0e-9;
								}
								else if(  current_section==2  ) {
									pSnom->Lumi.fSkal = (fmax-fmin)/32760.0e-9;
								}
							}
							MemFree( pPtr );
							MemFree( pData );
						}
						else {
							FehlerRsc( E_FILE_CORRUPT );
							_lclose( hFile );
							return ( FALSE );
						}
						zscale = 0.0;
						width = height = 0;
						break;
		}

		// determine current section ...
		if(  typ=='o'  &&  str[0]=='/'  ) {
			current_section = atoi( str+1 );
		}

		// here one will be in the right image at least ...
		if(  strcmp(str,"Z scale")==0  ) {
			zscale = atof( str2 );
		}
		else if(  strcmp(str,"Z magnify image")==0  ) {
			zmag = atof( str2 );
		}
		else if(  strcmp(str,"xres")==0  ) {
			width = value;
		}
		else if(  strcmp(str,"yres")==0  ) {
			height = value;
		}
		else if(  strcmp(str,"xreal")==0  ) {
			xscale = fvalue;
		}
		else if(  strcmp(str,"yreal")==0  ) {
			yscale = fvalue;
		}
	}
	_lclose( hFile );
	return ( TRUE );
}
// 20.22012



//***************************************** TE-Rechnungen aus Jena *****************************************
/*	ASCII-Format:
**	1: "TE" + evt. Kommentar (wird ignoriert)
**	2: Titel
**	3: Weite Hühe (mit Leerzeichen)
**  4 ... n: X-Werte (floats oder integers!)
**/

// Kopiert eine Zeile im Speicher
LONG CopyString( LPBYTE pSrc, LPBYTE pDest, LONG iLen, LPLONG iCountDown )
{
	LONG i = 0;
	// Müll überspringen
	while( *pSrc < 32  &&  *iCountDown > 0 ) {
		( *iCountDown )--;
		i++;
		pSrc++;
	}
	// Zeile lesen
	while( ( *pSrc >= 32||*pSrc == '\t' )  &&  *iCountDown > 0  &&  iLen > 1 ) {
		( *iCountDown )--;
		i++;
		iLen--;
		if( *pSrc == '\t' ) {
			*pSrc++;
			*pDest++ = 32;
		}
		else {
			*pDest++ = *pSrc++;
		}
	}
	*pDest = 0;
	return ( i );
}
// 5.10.99


//  Liest eine Datei im RAW-Snomformat ein
BOOL ReadJena( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	LPUWORD	pTopo;
	UWORD uMax = 0;
#if 0
#else
	double *pData;
	double fMax = 0.0;
#endif
	LONG iLen = 0;
	LONG x, lWeite, lHoehe;
	LPBYTE pBuf, c;
	BYTE pStr[1024];

	iLen = _llseek( hFile, 0l, FILE_END );
	pBuf = (LPBYTE)pMalloc( iLen+1l );
	if( pBuf == NULL ) {
		if( pBuf ) {
			MemFree( pBuf );
		}
		_lclose( hFile );
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	_llseek( hFile, 0l, FILE_BEGIN );
	iLen = _hread( hFile, pBuf, iLen );
	_lclose( hFile );

	c = pBuf;
	c += CopyString( c, pStr, 1024, &iLen );        // TE übersprinmgen
	c += CopyString( c, pStr, 1024, &iLen );        // TE übersprinmgen
	strncpy( pSnom->Topo.strTitel, pStr, 31 ),
	c += CopyString( c, pStr, 32, &iLen );          // Zeile, Spalten
	sscanf( pStr, "%ld %ld", &lWeite, &lHoehe );
	pTopo = (LPWORD)pMalloc( lWeite*lHoehe*sizeof( WORD ) );
	pData = (double*)pMalloc( lWeite*lHoehe*sizeof( double ) );
	pBmp->pPsi.iCol = lWeite;
	pBmp->pPsi.iRow = lHoehe;

	for( x = 0;  x < lWeite*lHoehe;  x++ ) {
		c += CopyString( c, pStr, 32, &iLen );
#if 0
		pDatum = pStr;
		while( ( *pDatum > 0  &&  *pDatum <= 32 )  ||  *pDatum == '.' )
			pDatum++;
		pTopo[x] = atoi( pDatum );
		if( pTopo[x] > uMax ) {
			uMax = pTopo[x];
		}
	}
	pSnom->Topo.fSkal = 1.0;
#else
		pData[x] = atof( pStr );
		if( pData[x] > fMax ) {
			fMax = pData[x];
		}
	}
	for( x = 0;  x < lWeite*lHoehe;  x++ ) {
		pTopo[x] = pData[x]/fMax*65534.0;
	}
	pSnom->Topo.fSkal = fMax/65534.0;
	uMax = 65535u;
	MemFree( pData );
#endif
	MemFree( pBuf );

	pSnom->w = lWeite;
	pSnom->h = lHoehe;
	pSnom->fX = 1.0;
	pSnom->fY = 1.0;
	pBmp->pPsi.fW = 1.0;
	pBmp->pPsi.fH = 1.0;
	pSnom->Topo.puDaten = pTopo;
	pSnom->Topo.uMaxDaten = uMax;
	pSnom->Topo.Typ = TOPO;
	pSnom->Topo.bPseudo3D = FALSE;
	pSnom->Topo.bModuloKonturen = FALSE;
	pSnom->Topo.bKonturen = FALSE;
	pSnom->Topo.Farben[0] = 0x0l;
	pSnom->Topo.Farben[1] = 0x7f7f7f7ful;
	pSnom->Topo.Farben[2] = 0xFFFFFFFFul;
	pSnom->Topo.bNoLUT = TRUE;
	pSnom->Topo.fStart = 0.0;
	pSnom->Topo.fEnde = 100.0;
	pBmp->Links = TOPO;
	pSnom->Topo.fXYWinkel3D = f3DXYWinkel;
	pSnom->Topo.fZWinkel3D = f3DZWinkel;
	pSnom->Topo.fZSkal3D = f3DZWinkel;
	pSnom->Topo.bSpecialZUnit = FALSE;
	pSnom->Topo.bShowNoZ = FALSE;
	lstrcpy( pSnom->Topo.strZUnit, "C/nmü" );
	BildMax( ( &pSnom->Topo ), lWeite, lHoehe );

	return ( TRUE );
}


// 6.1.98


/* Format
   "!ASC" = Kennung
   "Name des Bildes"
   "Zeilen, Spalten"
   Daten ....
 */
//  Liest eine Datei im RAW-Snomformat ein
BOOL ReadAscii( HFILE hFile, LPBMPDATA pBmp )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[0] );
	LPUWORD	pTopo;
	UWORD uMax = 0;
	LONG iLen = 0;
	LONG x, lWeite, y, lHoehe;
	LPDOUBLE pData;
	LPBYTE pBuf, c, pDatum;
	BYTE pStr[0x8000];
	double fMax = -1e199, f, fMin = 1e199;

	iLen = _llseek( hFile, 0l, FILE_END );
	pBuf = (LPBYTE)pMalloc( iLen+1l );
	if( pBuf == NULL ) {
		if( pBuf ) {
			MemFree( pBuf );
		}
		_lclose( hFile );
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	_llseek( hFile, 0l, FILE_BEGIN );
	iLen = _hread( hFile, pBuf, iLen );
	_lclose( hFile );

	c = pBuf;
	c += CopyString( c, pStr, 0x8000, &iLen );      // !ASC übersprinmgen
	c += CopyString( c, pStr, 0x8000, &iLen );      // Titel
	strncpy( pSnom->Topo.strTitel, pStr, 31 ),
	c += CopyString( c, pStr, 32, &iLen );          // Zeile, Spalten
	sscanf( pStr, "%ld %ld", &lWeite, &lHoehe );
	pTopo = (LPWORD)pMalloc( lWeite*lHoehe*sizeof( WORD ) );
	pData = (double*)pMalloc( lWeite*lHoehe*sizeof( double ) );
	pBmp->pPsi.iCol = lWeite;
	pBmp->pPsi.iRow = lHoehe;

	for( y = 0;  y < lHoehe;  y++ )	{
		c += CopyString( c, pStr, 0x8000, &iLen );
		pDatum = pStr;
		for( x = 0;  x < lWeite  &&  *pDatum > 0;  x++ ) {
			pData[x+y*lWeite] = f = atof( pDatum );
			if( f > fMax ) {
				fMax = f;
			}
			if( f < fMin ) {
				fMin = f;
			}
			while( *pDatum > 32 )
				pDatum++;
			while( *pDatum <= 32  &&  *pDatum > 0 )
				pDatum++;
		}
	}
	fMax -= fMin;
	for( x = 0;  x < lWeite*lHoehe;  x++ ) {
		pTopo[x] = ( pData[x]-fMin )/fMax*65534.0;
	}
	pSnom->Topo.fSkal = fMax/65534.0;
	uMax = 65535u;
	MemFree( pData );
	MemFree( pBuf );

	pSnom->w = lWeite;
	pSnom->h = lHoehe;
	pSnom->fX = 1.0;
	pSnom->fY = 1.0;
	pBmp->pPsi.fW = 1.0;
	pBmp->pPsi.fH = 1.0;
	pSnom->Topo.puDaten = pTopo;
	pSnom->Topo.uMaxDaten = uMax;
	pSnom->Topo.Typ = TOPO;
	pSnom->Topo.bPseudo3D = FALSE;
	pSnom->Topo.bModuloKonturen = FALSE;
	pSnom->Topo.bKonturen = FALSE;
	pSnom->Topo.Farben[0] = 0x0l;
	pSnom->Topo.Farben[1] = 0x7f7f7f7ful;
	pSnom->Topo.Farben[2] = 0xFFFFFFFFul;
	pSnom->Topo.bNoLUT = TRUE;
	pSnom->Topo.fStart = 0.0;
	pSnom->Topo.fEnde = 100.0;
	pBmp->Links = TOPO;
	pSnom->Topo.fXYWinkel3D = f3DXYWinkel;
	pSnom->Topo.fZWinkel3D = f3DZWinkel;
	pSnom->Topo.fZSkal3D = f3DZWinkel;
	pSnom->Topo.bSpecialZUnit = FALSE;
	pSnom->Topo.bShowNoZ = FALSE;
	lstrcpy( pSnom->Topo.strZUnit, "C/nmü" );
	BildMax( ( &pSnom->Topo ), lWeite, lHoehe );

	return ( TRUE );
}


// 6.1.98

#if 0
//****************************************************** ASCII-Format **************************************

//  Liest eine Datei im RAW-Snomformat ein
BOOL ReadRawSnom( HFILE hFile, BMPDATA huge *pBmp )
{
	SNOMDATA huge *pSnom = &( pBmp->pSnom[0] );
	BYTE sBuf[1024];                                // Puffer für Scanf ...
	BYTE huge *Buf, huge *c;
	LONG x, y = 0, w = 0, h = 0, topo, lumi, len, i;
	WORD huge *pTopo, huge *pLumi;

	pTopo = (WORD huge*)GlobalAllocPtr( GPTR, 1024l*1024l*2l );
	pLumi = (WORD huge*)GlobalAllocPtr( GPTR, 1024l*1024l*2l );
	len = _llseek( hFile, 0l, 2 );
	Buf = (BYTE huge*)GlobalAllocPtr( GPTR, len+1l );
	if( pTopo == NULL  ||  pLumi == NULL  ||  Buf == NULL )	{
		if( pTopo ) {
			GlobalFreePtr( pTopo );
		}
		if( pLumi ) {
			GlobalFreePtr( pLumi );
		}
		if( Buf ) {
			GlobalFreePtr( Buf );
		}
		_lclose( hFile );
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}
	_llseek( hFile, 1l, 0 );
	_hread( hFile, Buf, len );
	_lclose( hFile );
	c = Buf;
	while( len > 0 ) {
		while( len-- > 0  &&  *( ++c ) < 32 )
			;
		for( i = 0;  c[i] >= 32;  i++ ) {
			sBuf[i] = c[i];
		}
		c[i] = 0;
		sBuf[i] = 0;
		// DOS: Toll! sscanf geht NICHT ueber Segmentgrenzen, auch im Modell HUGE nicht!!!
		// Abhilfe: Umkopieren ...
		c += i;
		len -= i;
		if( sBuf[0] < '0'  ||  sBuf[0] > '9'  ||  sscanf( sBuf, "%ld %ld %ld %ld", &x, &y, &topo, &lumi ) < 4 ) {
			continue;
		}
		y--;
		if( x > 1024  ||  y > 1024  ||  x < 0  ||  y < 0 ) {
			continue;
		}
		if( x >= w ) {
			w = x+1;
		}
		if( y >= h ) {
			h = y+1;
		}
		pTopo[x+y*1024] = topo;
		pLumi[x+y*1024] = lumi;
	}
	GlobalFreePtr( Buf );
	pSnom->w = w;
	pSnom->h = h;
	for( y = 0;  y < h;  y++ ) {
		for( x = 0;  x < w;  x++ ) {
			pTopo[x+y*w] = pTopo[x+y*1024]+0x7FFFu;
			pLumi[x+y*w] = pLumi[x+y*1024]+0x7FFFu;
		}
	}
	GlobalReAllocPtr( pTopo, 2l*w*h, 0 );
	GlobalReAllocPtr( pLumi, 2l*w*h, 0 );

	lstrcpy( pSnom->Topo.Titel, "Topography" );
	lstrcpy( pSnom->Lumi.Titel, "Luminescence" );

	pSnom->Topo.Daten = pTopo;
	pSnom->Topo.Typ = TOPO;
	pSnom->Topo.MaxDaten = 65535u;
	pSnom->Topo.Pseudo3D = FALSE;
	pSnom->Topo.ModuloKonturen = FALSE;
	pSnom->Topo.Konturen = FALSE;
	pSnom->Topo.Farben[0] = 0x0l;
	pSnom->Topo.Farben[1] = 0x7f7f7f7ful;
	pSnom->Topo.Farben[2] = 0xFFFFFFFFul;
	pSnom->Topo.Verlauf3Farben = TRUE;
	pSnom->Topo.Start = 0.0;
	pSnom->Topo.Ende = 100.0;
	pBmp->Links = TOPO;
	MaxBild( &( pSnom->Topo ), w, h );

	pSnom->Lumi.Daten = pLumi;
	pSnom->Lumi.Typ = LUMI;
	pSnom->Lumi.MaxDaten = 65535u;
	pSnom->Lumi.Pseudo3D = FALSE;
	pSnom->Lumi.ModuloKonturen = FALSE;
	pSnom->Lumi.Konturen = FALSE;
	pSnom->Lumi.Farben[0] = 0l;
	pSnom->Lumi.Farben[1] = 0x7f7f7f7ful;
	pSnom->Lumi.Farben[2] = 0xFFFFFFFFul;
	pSnom->Lumi.Verlauf3Farben = TRUE;
	pSnom->Lumi.Start = 0.0;
	pSnom->Lumi.Ende = 100.0;
	pBmp->Rechts = LUMI;
	MaxBild( &( pSnom->Lumi ), w, h );

	return ( TRUE );
}


// 6.1.98
#endif


/**************	Ab hier HDF-Datei lesen (diese Routine verzweigt dann evt.)  *****************/

// Findet ein Tag mit bestimmter Referenznummer
LONG SearchTagRef( DD_TAG HUGE *dd, UWORD tag, UWORD ref )
{
	UWORD i = 0;

	while( dd[i].tag != 0 )	{
		if( dd[i].tag == tag  &&  dd[i].ref == ref ) {
			return ( i );
		}
		i++;
	}
	return ( -1 );
}


// 11.8.97


// Findet ein Tag und gibt die Referenznummer zurück
LONG SearchTag( DD_TAG HUGE *dd, UWORD tag, LPUWORD ref )
{
	UWORD i = 0;

	while( dd[i].tag != 0 )	{
		if( dd[i].tag == tag ) {
			*ref = dd[i].ref;
			return ( i );
		}
		i++;
	}
	return ( -1 );
}


// 10.8.97


//  Liest eine Datei in beliebigem Format ein
//	Die Routine selber interpretiert nur das HDF-Format
BOOLEAN	ReadAll( LPCSTR datei, LPBMPDATA pBmp )
{
	DD_TAG HUGE *lpDD;
	LONG i, j, k, lMaxDD;
	UWORD uRef;
	LPUCHAR	pPuffer = NULL;
	BOOLEAN	bResult;

	HFILE hFile;
	unsigned char str[4];
	LONG lNextTags;

	ASSERT( pBmp != NULL  );

	// Erst einmal davon ausgehen, dass die Datei nix sinnvolles enthült
	pBmp->pSnom[0].Topo.puDaten = NULL;
	pBmp->pSnom[0].Topo.Typ = NONE;
	pBmp->pSnom[0].Error.puDaten = NULL;
	pBmp->pSnom[0].Error.Typ = NONE;
	pBmp->pSnom[0].Lumi.puDaten = NULL;
	pBmp->pSnom[0].Lumi.Typ = NONE;

	pBmp->pPsi.fXOff0 = 0.0;
	pBmp->pPsi.fYOff0 = 0.0;

	if( ( hFile = _lopen( datei, OF_READ|OF_SHARE_DENY_WRITE ) ) == -1 ) {
		FehlerRsc( E_FILE );
		return ( FALSE );
	}

	WarteMaus();
#ifdef BIT32
	{
		FILETIME ft;
		SYSTEMTIME st;
		GetFileTime( (HANDLE)hFile, &ft, NULL, NULL );
		FileTimeToLocalFileTime( &ft, &ft );
		FileTimeToSystemTime( &ft, &st );
		pBmp->iJahr = st.wYear;
		pBmp->iMonat = st.wMonth;
		pBmp->iTag = st.wDay;
		pBmp->iStunde = st.wHour;
		pBmp->iMinute = st.wMinute;
		pBmp->iSekunde = st.wSecond;
	}
#endif
	_hread( hFile, str, 4l );

	// Zuerst einmal herausfinden, was es für eine Datei ist ...
	if( str[0] == '!'  &&  str[1] == 'A' ) {
		// old SNOM ascii ASCII ...
		bResult = ReadAscii( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( str[0] == 'T'  &&  str[1] == 'E' ) {
		// Total energy data from Jena ...
		bResult = ReadJena( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( *(long*)str == *(long*)"STiM" ) {
		// seem like RHK ...
		bResult = ReadRHK( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( str[0] == ';' ) {
		// probably Omicron ...
		bResult = ReadOmicron( datei, hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( *(long*)str == *(long*)"SPIZ" ) {
		// seem like Seiko ...
		bResult = ReadSeiko( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	// ECS Header seems to be not very special ...
	if( str[0] == 0xA0  &&  str[1] == 0 ) {
		bResult = ReadECS( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	// Hitachi has really no unique Header ...
	if( str[0] == 0x00  &&  str[1] == 0x01 ) {
		bResult = ReadHitachi( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( *(long*)str == WSxM_MAGIC )	{
		bResult = ReadWSxM( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

#if 0
	if( str[0] == 7 ) {
		StatusLineRsc( I_ASCII );
		result = ReadRawSnom( hFile, pBmp );
		SetCursor( OldCursor );
		ClearStatusLine();
		_lclose( hFile );
		return ( result );
	}
#endif

	if( str[0] == '\\' ) {
		// War wohl eine Digital-Datei
		StatusLineRsc( I_DI );
		bResult = ReadDigital( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if(  str[0] == 'G'   &&  str[1]=='W'  &&  str[2]=='Y'  ) {
		// Gwyddion
		StatusLineRsc( I_GWY );
		bResult = ReadGwyddion( hFile, pBmp );
		NormalMaus();
		return ( bResult );
	}

	if( *(long*)str != *(long*)HDFMAGIC ) {
#if 0
		// Kein HDF? Dann kann es eigentlich nur eine Bitmap sein ...
		BITMAPINFO huge *pDib;

		StatusLineRsc( I_BMP );
		_lclose( hFile );
		if( ( pDib = ReadBitmap( datei ) ) != NULL ) {
			ConvertDib2Snom( &( pBmp->pSnom[0] ), pBmp, pDib, MessageBox( pBmp->hwnd, "Sind dies Topografiedaten?", datei, MB_ICONQUESTION|MB_YESNO ) == IDYES );
			MemFree( pDib );
			SetCursor( OldCursor );
			ClearStatusLine();
			*pPuffer = 0;
			return ( TRUE );
		}
		else
#endif
		{
			NormalMaus();
			lstrcpyn( pBmp->szName, datei, 256 );
			return ( DialogBoxParam( hInst, "RohDialog", pBmp->hwnd, RohImportDialog, (LPARAM)pBmp ) );
		}
	}

	// Ab hier HDF-Datei lesen
	StatusLineRsc( I_HDF );
	pPuffer = (LPUCHAR)pMalloc( 0x8000 );                                   // Sollte reichen ...
	lpDD = (DD_TAG HUGE*)pMalloc( sizeof( DD_TAG )*256 );    // Sollte ebenfalls genug sein
	// Wenn nicht, dann war die Datei zu esoterisch!
	if( pPuffer == NULL  ||  lpDD == NULL )	{
		if( pPuffer ) {
			MemFree( pPuffer );
		}
		if( lpDD ) {
			MemFree( lpDD );
		}
		_lclose( hFile );
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	_llseek( hFile, 0l, 0 );
	_hread( hFile, pPuffer, 0x08000ul );

	lNextTags = 4;
	lMaxDD = 0l;
	// Tags laden; dabei werden weniger als 256 Tags angenommen, die sich bitteschoen in den
	// ersten 16000 Bytes versammeln sollten; ansonsten war es mit Sicherheit keine SNOM oder
	// AFM-Datei
	do {
		i = Big2Little( *(LPUWORD)( pPuffer+lNextTags ) );                        // Anzahl (dh.count)
		j = LongBig2Little( *(LPLONG)( pPuffer+lNextTags+2 ) );   // naechstes Tag (dh.next)
		// Selbst gesteckte Grenzen überschritten; geht natürlich eleganter (und viel langsamer ...)
		if( i+lMaxDD >= 256  ||  lNextTags > 0x8000-sizeof( DD_HEADER ) ) {
			MemFree( lpDD );
			MemFree( pPuffer );
			_lclose( hFile );
			// eigentlich E_MANY_TAGS "Unerwartetes HDF-Format!"
			// Aber von uns kann SOO eine Datei eh nicht sein ...
			NormalMaus();
			FehlerRsc( E_FILE_CORRUPT );
			return ( FALSE );
		}
		// Tags einfach kopieren
		/* ACHTUNG: sizeof(DD_HEADER)==8 mit MS Visual C++!!! */
		MemMove( &lpDD[lMaxDD], (LPVOID)( &pPuffer[lNextTags+6] ), sizeof( DD_TAG )*i  );
		lNextTags = j;
		lMaxDD += i;
	} while( lNextTags > 0l );
	lpDD[lMaxDD].tag = 0;   // Endemarkierung

	// Big-Endian nach Little-Endian konvertieren
	for( i = 0;  i < lMaxDD;  i++ )	{
		lpDD[i].tag = Big2Little( lpDD[i].tag );
		lpDD[i].ref = Big2Little( lpDD[i].ref );
		lpDD[i].offset = LongBig2Little( lpDD[i].offset );
		lpDD[i].len = LongBig2Little( lpDD[i].len );
	}

	// check HDF typ
	pBmp->Typ = HDF;
	i = (short)SearchTag( lpDD, DFTAG_SNOMPUTZ, &uRef );
	if( i >= 0 ) {
		// Scanparameter für snomputz
		pBmp->lExtraLen = 0;
		pBmp->pExtra = pMalloc( sizeof( SNOMPUTZ_SCANDATA ) );
		if( pBmp->pExtra != NULL ) {
			pBmp->lExtraLen = sizeof( SNOMPUTZ_SCANDATA );
			MemMove( pBmp->pExtra, pPuffer+lpDD[i].offset, lpDD[i].len );
			pBmp->Typ = SNOMPUTZ;
		}
	}

	// Evt. Kommentar lesen
	i = (short)SearchTag( lpDD, DFTAG_FD, &uRef );
	if( i >= 0 ) {
		LPBYTE date;
		if( lpDD[i].len < 1024 ) {
			MemMove( (LPSTR)pBmp->pKommentar, pPuffer+lpDD[i].offset, lpDD[i].len );
		}
		else {
			// Kommentar zu lang!
			MemMove( (LPSTR)pBmp->pKommentar, pPuffer+lpDD[i].offset, 1024 );
			StatusLineRsc( W_COMMENT );
		}
		date = strstr( pBmp->pKommentar, "Measured = " );
		if( date != NULL ) {
			sscanf( date+10, "%hd.%hd.%hd %02hd:%02hd:%02hd", (LPUWORD)&pBmp->iTag, (LPUWORD)&pBmp->iMonat, (LPUWORD)&pBmp->iJahr, (LPUWORD)&pBmp->iStunde, (LPUWORD)&pBmp->iMinute, (LPUWORD)&pBmp->iSekunde );
		}
	}


	// Nun sind alle Tags geladen, jetzt heisst es suchen!
	// Oder vielleicht doch eine switch-Anweisung durchlaufen?
	i = SearchTag( lpDD, DFTAG_NDG, &uRef );
	pBmp->pPsi.fW = 0.0;    // Zeigt: Noch keinen gültigen PsiHdr gelesen
	do {
		if( i >= 0 ) {
			LPUWORD	puWord, puPos;
			WORKMODE ThisMode = FALSE;
			UWORD CompressionType = 0;
			BILD Bild;
			LONG w = 0, h = 0;
			LONG lNtData = 0;
			LONG lDataStart = 0, lDatatLen = 0;

			// Besser alles auf Null setzen
			MemSet( &Bild, 0, sizeof( BILD ) );
			Bild.fSkal = 1.0;

			// NDG-Tag bearbeiten, d.h. alle Informationen über das Array herausfinden
			puWord = (LPUWORD)( pPuffer+lpDD[i].offset );
			for( j = 0;  j < lpDD[i].len/2;  j += 2 ) {
				k = SearchTagRef( lpDD, Big2Little( puWord[j] ), Big2Little( puWord[j+1] ) );
				ASSERT(  k < lMaxDD  );
				if( k == -1 ) {
					continue;       // Sollte nicht vorkommen: Kein Tag obwohl eines Angekündigt
				}
				puPos = (LPUWORD)( pPuffer+lpDD[k].offset );
				switch(	Big2Little( puWord[j] ) ) {
					// Typ herausfinden
					case DFTAG_SDL:
						if( ReadLong( puPos ) == ReadLong( "Lumi" ) ) {
							ThisMode = LUMI;
						}
						else if( ReadLong( puPos ) == ReadLong( "Topo" ) ) {
							ThisMode = TOPO;
						}
						else {
							// Unbekannt ist Fehlersignal ...
							ThisMode = ERRO;
						}
						lstrcpy( (LPSTR)Bild.strTitel, (LPSTR)puPos );
						break;

					// Dimension und Format herausfinden
					case DFTAG_SDD:
						if( Big2Little( *puPos ) != 2 )	{
							// Unbekannte Daten mit mehr als 2 Dimensionen
							// Künnten mal Spektrale Daten werden ...
							ThisMode = FALSE;
							j = lpDD[i].len;
							break;
						}
						puPos++;
						w = LongBig2Little( ReadLong( puPos ) );
						puPos += 2;
						h = LongBig2Little( ReadLong( puPos ) );
						puPos += 2;
						k = SearchTagRef( lpDD, Big2Little( *puPos ), Big2Little( *( puPos+1 ) ) );
						lNtData = LongBig2Little( ReadLong( pPuffer+lpDD[k].offset ) );
						break;

					case DFTAG_SDU: // Einheiten feststellen:  (um oder üm) bzw. nm für Topo, sonst Lumi
						if( lstrcmpi( (LPSTR)puPos, "nm" ) > 0 ) {
							pBmp->pPsi.fW *= 1000.0;
						}
						(LPSTR)puPos += 3;
						if( lstrcmpi( (LPSTR)puPos, "nm" ) > 0 ) {
							pBmp->pPsi.fH *= 1000.0;
						}
						(LPSTR)puPos += 3;
						if( lstrcmpi( (LPSTR)puPos, "nm" ) > 0 ) {                                                                      // nW würe kleiner Null ...
							Bild.fSkal *= 1000.0;
						}
						break;

					case DFTAG_CAL:
					{                       // Einige Klimmzuege um double von Mototrola nach INTEL zu konvertieren ...
						// Lohn ist die Skalierung der z-Achse in um (=üm) bwz nm (muss nachskaliert werden!)
						long hilf[2];
						ASSERT( sizeof( long )*2 == sizeof( double ) );
						hilf[1] = LongBig2Little( ReadLong( puPos ) );
						hilf[0] = LongBig2Little( ReadLong( puPos+2 ) );                                                                // puPos = WORD *!
						Bild.fSkal *= *(double*)hilf;
						break;
					}

					case DFTAG_SDM:
						pBmp->pPsi.iMax = Big2Little( *puPos++ );               // Als Word!
						pBmp->pPsi.iMin = Big2Little( *puPos++ );
						break;

					case DFTAG_SD:
						lDataStart = lpDD[k].offset;
						lDatatLen = lpDD[k].len;
						break;

					case DFTAG_IMCOMP:      // Daten LZW-komprimiert
					case DFTAG_COMPRESSED: // oder sonstwie komprimiert
						CompressionType = lpDD[k].offset;
						if( CompressionType == 0 ) {
							CompressionType = OLD_LZW;
						}
						break;

					case DFTAG_PSIHD:       // Auch Snomputz nutzt den binüren Header
#if LEN_OK
						MemMove( &( pBmp->pPsi ), puPos, sizeof( PSI_HEADER ) );
#else
#ifdef BIT32
						MemMove( &( pBmp->pPsi ), puPos, 66 );
						MemMove( &( pBmp->pPsi.fW ), puPos+33, 64 );                                                            // 130-66=64 Da puPos *WORD!!! => 33
#else
						{
							// Da MS-C ums verrecken nicht packt eben per Hand ...
							LPBYTE pPtr = (LPBYTE)puPos;

							pPtr += 4;                                                              // u1
							MemMove( pBmp->pPsi.cTitle, pPtr, 32 );                                                         // Titel
							pPtr += 32;
							MemMove( pBmp->pPsi.cInstrument, pPtr, 8 );
							pPtr += 8;
							pBmp->pPsi.x_dir = *( (LPSWORD)pPtr )++;
							pBmp->pPsi.y_dir = *( (LPSWORD)pPtr )++;
							pBmp->pPsi.cShowOffset = *pPtr++;
							pBmp->pPsi.cNoUnits = *pPtr++;
							pBmp->pPsi.iCol = *( (LPSWORD)pPtr )++;
							pBmp->pPsi.iRow = *( (LPSWORD)pPtr )++;
							pPtr += 12;                                                             // u5
							pBmp->pPsi.fW = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fH = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fXOff0 = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fYOff0 = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.u6 = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fLinePerSec = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fSetpoint = *( (LPFLOAT)pPtr )++;
							MemMove( pBmp->pPsi.cSetPointUnit, pPtr, 8 );
							pPtr += 8;
							pBmp->pPsi.fSampleBias = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fTipBias = *( (LPFLOAT)pPtr )++;
							pBmp->pPsi.fZGain = *( (LPFLOAT)pPtr )++;
							MemMove( pBmp->pPsi.cZGainUnit, pPtr, 8 );
							pPtr += 8;
							pBmp->pPsi.iMin = *( (LPSWORD)pPtr )++;
							pBmp->pPsi.iMax = *( (LPSWORD)pPtr )++;
						}
#endif
#endif
						break;

					case DFTAG_BILDHD: // Snomputzheader!
#ifdef BIT32
						MemMove( &Bild, puPos, 62 );
						MemMove( &( Bild.Farben[0] ), puPos+31, 52 );                                               // 114-62=52 Da puPos *WORD => 31
#else
						{
							// Da MS-C ums verrecken nicht packt eben per Hand ...
							LPUCHAR	pPtr = (LPUCHAR)puPos;

							pPtr += 6;                                                      // puDaten und uMaxDaten überspringen
							Bild.Typ = *( (LPUWORD)pPtr )++;
							MemMove( Bild.strTitel, pPtr, 32 );
							pPtr += 32;
							Bild.fSkal = *( (LPDOUBLE)pPtr )++;
							Bild.uKontur = *( (LPUWORD)pPtr )++;
							Bild.uKonturToleranz = *( (LPUWORD)pPtr )++;
							Bild.uModulo = *( (LPUWORD)pPtr )++;
							Bild.bVerlauf3Farben = (BOOLEAN)*( (LPUWORD)pPtr )++;
							Bild.bPseudo3D = (BOOLEAN)*( (LPUWORD)pPtr )++;
							Bild.bKonturen = (BOOLEAN)*( (LPUWORD)pPtr )++;
							Bild.bModuloKonturen = (BOOLEAN)*( (LPUWORD)pPtr )++;
							Bild.Farben[0] = *( (LPLONG)pPtr )++;
							Bild.Farben[1] = *( (LPLONG)pPtr )++;
							Bild.Farben[2] = *( (LPLONG)pPtr )++;
							Bild.fStart = *( (LPFLOAT)pPtr )++;
							Bild.fEnde = *( (LPFLOAT)pPtr )++;
							Bild.fXYWinkel3D = *( (LPFLOAT)pPtr )++;
							Bild.fZWinkel3D = *( (LPFLOAT)pPtr )++;
							Bild.fZSkal3D = *( (LPFLOAT)pPtr )++;
							Bild.bSpecialZUnit = (BOOLEAN)*( (LPUWORD)pPtr )++;
							Bild.bShowNoZ = (BOOLEAN)*( (LPUWORD)pPtr )++;
							MemMove( Bild.strZUnit, pPtr, 16 );
							Bild.iNumColors = 0;
						}
#endif
						if( lpDD[k].len > 114 )	{                                               // contains Colortable
							LPUCHAR	pPtr = ( (LPUCHAR)puPos )+114;
							WORD iCols = 0;

							Bild.iNumColors = *( (LPUWORD)pPtr )++;
							if( Bild.iNumColors > 256 ) {
								Bild.iNumColors = 0;
							}
							while( iCols < Bild.iNumColors ) {
								Bild.LUT[iCols++] = RGB( pPtr[0], pPtr[1], pPtr[2] );
								pPtr += 3;
							}
						}
						// Wenn das Datenlangwort == 1, Linkes Bild, ==2, rechtes Bild
						if( *(LPLONG)puPos == 1 ) {
							pBmp->Links = Bild.Typ;
						}
						if( *(LPLONG)puPos == 2 ) {
							pBmp->Rechts = Bild.Typ;
						}
						Bild.puDaten = NULL;
						break;

#if 0
					default:
					{
						BYTE str[256];
						sprintf( str, "$%X (%d) is an unknown tag!", Big2Little( puWord[j] ), Big2Little( puWord[j] ) );
						MessageBox( NULL, str, "Warning", MB_OK );
						break;
					}
#endif
				} // End Switch
			}

			// Alle müglichen Tags wurden durchlaufen, jetzt wird geladen!
			if( ThisMode&( TOPO|ERRO|LUMI ) ) {
				LONG lBildLen = w*h*sizeof( WORD );
				LPBYTE pPtr;
				LPBYTE pHilf = NULL;
				int BitsPerData = 16;

				if( lNtData != 0x01161004l  &&  lNtData != 0x01171004l  &&  lNtData != 0x01151004l ) {
					_lclose( hFile );
					MemFree( lpDD );
					MemFree( pPuffer );
					FehlerRsc( E_FILE_CORRUPT );
					return ( FALSE );
				}

				// Laden
				if( lNtData == 0x01151004l ) {
					BitsPerData = 8;
				}

				// Viel zu wenig Speicher?
				pPtr = pMalloc( lBildLen );
				if( pPtr == NULL ) {
					_lclose( hFile );
					MemFree( lpDD );
					MemFree( pPuffer );
					NormalMaus();
					FehlerRsc( E_MEMORY );
					return ( FALSE );
				}

				if( ThisMode&TOPO ) {
					MemMove( &( pBmp->pSnom[0].Topo ), &Bild, sizeof( BILD ) );
				}
				else if( ThisMode&ERRO ) {
					MemMove( &( pBmp->pSnom[0].Error ), &Bild, sizeof( BILD ) );
				}
				else if( ThisMode&LUMI ) {
					MemMove( &( pBmp->pSnom[0].Lumi ), &Bild, sizeof( BILD ) );
				}
				else {
					_lclose( hFile );
					MemFree( lpDD );
					MemFree( pPuffer );
					NormalMaus();
					FehlerRsc( E_FILE_CORRUPT );
					return ( FALSE );
				}

				_llseek( hFile, lDataStart, 0 );
				k = _hread( hFile, pPtr, lDatatLen	);

				// wir berücktsichtigen nun auch *GEPACKTE* Dateien
				if( CompressionType  &&  ( pHilf = pMalloc( lBildLen ) ) != NULL ) {
					// Daten entpacken

					// Altes Format (schlicht nur LZW)
					if( CompressionType&OLD_LZW ) {
						if( pPtr[1] == 0 ) {
							pPtr[0] = INIT_BITS;
							LZWDecodeBlock( pHilf, lBildLen, pPtr, lDatatLen );
							MemMove( pPtr, pHilf, lBildLen );
						}
					}

					// Zuerst LZW-Verpackung entfernen
					if( CompressionType&LZW  &&  LZWDecodeBlock( pHilf, lBildLen, pPtr, lDatatLen ) > 0 ) {
						MemMove( pPtr, pHilf, lBildLen );
					}
					// Dann Huffmankodierung weg
					if( CompressionType&HUFFMAN  &&  HuffmanDecodeBlock( (LPWORD)pHilf, lBildLen, (LPUCHAR)pPtr, CompressionType&DELTA ) > 0 ) {
						MemMove( pPtr, pHilf, lBildLen );
					}
					// Differenzen zu Absolutwerte
					if( CompressionType&DELTA  &&  DeltaDecodeBlock( (LPUWORD)pHilf, lBildLen, pPtr ) > 0 ) {
						MemMove( pPtr, pHilf, lBildLen );
					}

					// Sind die Bits gepackt?
					if( CompressionType&NBITS ) {
						BitsPerData = pPtr[0];
						MemMove( pPtr, pPtr+1, lBildLen-1 );
					}
					MemFree( pHilf );
					CompressionType = NONE;
				}

				// Nun sind die Daten unkomprimiert
				//  lNtData==0x01161004l
				if( ( lNtData&0x00010000l ) == 0 ) {
					// Signed data!
					LadeBlock( pBmp, pPtr, w, w, h, BitsPerData, ThisMode, 0x8000u, Bild.Typ == NONE );
				}
				else {
					// Unsigned data
					LadeBlock( pBmp, pPtr, w, w, h, BitsPerData, ThisMode, 0, Bild.Typ == NONE );
				}
				MemFree( pPtr );

				pBmp->pSnom[0].fX = pBmp->pPsi.fW/(float)w;
				pBmp->pSnom[0].fY = pBmp->pPsi.fH/(float)h;
				pBmp->pSnom[0].fXOff = pBmp->pPsi.fXOff0;
				pBmp->pSnom[0].fYOff = pBmp->pPsi.fYOff0;
			}
			// Also endlich geladen, nun weiter ...
			ThisMode = FALSE;
		}
		j = SearchTag( lpDD+i+1, DFTAG_NDG, &uRef );
		i += j+1;
	} while( (short)j != -1 );

	_lclose( hFile );
	MemFree( lpDD );
	MemFree( pPuffer );
	NormalMaus();
	return ( TRUE );
}


// 10.8.97


/*********************************************************************************************
 ****	Ab hier nur Schreiberoutinen ...
 **********************************************************************************************/

// Speichert SNOM-Bild;
// orientiert sich dabei am Format von Park Scientific (HDF-Format)


// Speichert ein Bild; kann bis zu 3 mal aufgerufen werden! (TOPO, ERRO, LUMI)
// plStart points to a pointer where the actual start adress can be entered
WORD CreateBildHDF( LPUCHAR hdf, LPLONG lPosition, DD_TAG HUGE **dd_tags, WORD ref, LPBMPDATA pBmp, LONG w, LONG h, LPBILD pBild, LPLONG *plStart, LONG lLen, WORD CompressionType )
{
	LONG lPos = *lPosition;
	LPUWORD	pWord;
	LPLONG pLong;
	DD_TAG HUGE *dd = *dd_tags;
	WORD ndg_tags[10*2];
	WORD *ndg = ndg_tags;
	WORD wTagNr = 0;
	WORD iAkt = pBmp->iAktuell;
	int i;

	// Number-Types: Zuerst die Daten (#1)
	dd->tag = DFTAG_NT;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = 4l;
	if( CompressionType > 0 ) {
		MemMove( hdf+lPos, "\x01\x17\x10\x04", 4l );    // unsigned int!
	}
	else {
		MemMove( hdf+lPos, "\x01\x16\x10\x04", 4l );    // signed int!
	}
	dd++;
	lPos += 4l;
	*ndg++ = DFTAG_NT;
	*ndg++ = ref;
	wTagNr++;

	// PSI-Header (#2)
	dd->tag = DFTAG_PSIHD;
	dd->ref = ref;
	dd->offset = lPos;
#if LEN_OK
	dd->len = sizeof( PSI_HEADER );
#else
	dd->len = 130;
#endif
	lstrcpy( pBmp->pPsi.cTitle, pBild->strTitel );
	pBmp->pPsi.iCol = (WORD)w;
	pBmp->pPsi.iRow = (WORD)h;
	pBmp->pPsi.fW = pBmp->pSnom[iAkt].fX*(float)w/1000.0;
	pBmp->pPsi.fH = pBmp->pSnom[iAkt].fY*(float)h/1000.0;
	pBmp->pPsi.fXOff0 = pBmp->pSnom[iAkt].fXOff;
	pBmp->pPsi.fYOff0 = pBmp->pSnom[iAkt].fYOff;
	pBmp->pPsi.fZGain = pBild->fSkal;
	if( pBild->Typ == TOPO ) {
		lstrcpy( pBmp->pPsi.cZGainUnit, "nm" );
	}
	else {
		lstrcpy( pBmp->pPsi.cZGainUnit, "nW" );
	}
	if( CompressionType  ||  pBild->uMaxDaten <= 0x7fff ) {
		pBmp->pPsi.iMin = 0;
		pBmp->pPsi.iMax = pBild->uMaxDaten;
	}
	else {
		pBmp->pPsi.iMin = 0x8000u;
		pBmp->pPsi.iMax = (short)( (WORD)pBild->uMaxDaten-0x8000 );
	}
	MemMove( hdf+lPos, &( pBmp->pPsi ), dd->len );
#if !LEN_OK
#ifdef BIT32
	MemMove( hdf+lPos+66, &( pBmp->pPsi.fW ), 64 );
#endif
#endif
	lPos += dd->len;
	dd++;
	*ndg++ = DFTAG_PSIHD;
	*ndg++ = ref;
	wTagNr++;

	// Scientific Data Record (#3)
	dd->tag = DFTAG_SDD;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = 22;
	pWord = (LPUWORD)( hdf+lPos );
	*pWord++ = Big2Little( 2 );       // 2 Dimensionen
	*( (LPLONG)pWord )++ = LongBig2Little( w );
	*( (LPLONG)pWord )++ = LongBig2Little( h );
	*pWord++ = Big2Little( DFTAG_NT );
	*pWord++ = Big2Little( ref );
	*pWord++ = Big2Little( DFTAG_NT );
	*pWord++ = Big2Little( 0x101 );
	*pWord++ = Big2Little( DFTAG_NT );
	*pWord++ = Big2Little( 0x102 );
	lPos += 22;
	dd++;
	*ndg++ = DFTAG_SDD;
	*ndg++ = ref;
	wTagNr++;

	// Scientific Data (#4)
	dd->tag = DFTAG_SD;
	dd->ref = ref;
	( *plStart ) = &( dd->offset );
	dd->len = lLen;
	dd++;
	*ndg++ = DFTAG_SD;
	*ndg++ = ref;
	wTagNr++;

	// Scientific Data Label (#5)
	// Daran wird der Typ erkannt!
	dd->tag = DFTAG_SDL;
	dd->ref = ref;
	dd->offset = lPos;
	switch( pBild->Typ ) {
		case TOPO:
			dd->len = 15;
			MemMove( hdf+lPos, "Topography\0X\0Y", 15 );
			break;

		case LUMI:
			dd->len = 16;
			MemMove( hdf+lPos, "Luminecence\0X\0Y", 16 );
			break;

		case ERRO:
			dd->len = 10;
			MemMove( hdf+lPos, "Error\0X\0Y", 10 );
			break;
	}
	lPos += dd->len;
	dd++;
	*ndg++ = DFTAG_SDL;
	*ndg++ = ref;
	wTagNr++;

	// Scientific Data Unit (#6)
	dd->tag = DFTAG_SDU;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = 9;
	if( pBild->Typ != LUMI ) {
		MemMove( hdf+lPos, "um\0um\0nm", 9 );
	}
	else {
		MemMove( hdf+lPos, "um\0um\0nW", 9 );
	}
	lPos += dd->len;
	dd++;
	*ndg++ = DFTAG_SDU;
	*ndg++ = ref;
	wTagNr++;

	// Kalibrierung (#7)
	dd->tag = DFTAG_CAL;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = 36;
	pLong = (LPLONG)( hdf+lPos );
	{       // Einige Klimmzuege um double von Mototrola nach INTEL zu konvertieren ...
		// Lohn ist die Skalierung der z-Achse in um (=üm)
		long hilf[2];
		*(double*)hilf = pBild->fSkal;
		*pLong++ = LongBig2Little( hilf[1] );
		*pLong++ = LongBig2Little( hilf[0] );
	}
	*pLong++ = 0;
	*pLong++ = 0;
	*pLong++ = 0;
	*pLong++ = 0;
	*pLong++ = 0;
	*pLong++ = 0;
	*pLong++ = DFNT_FLOAT64;
	lPos += dd->len;
	dd++;
	*ndg++ = DFTAG_CAL;
	*ndg++ = ref;
	wTagNr++;

	// Eigendefinition zur Darstellungsbeschreibung (#8)
	dd->tag = DFTAG_BILDHD;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = 114;
	MemMove( hdf+lPos, pBild, 62 );
	MemMove( hdf+( lPos+62 ), &( pBild->Farben[0] ), 52 );
	// Save Palette if needed
	if( pBild->iNumColors > 0  &&  !pBild->bNoLUT ) {
		dd->len +=  2+( 3*pBild->iNumColors );
	}
	if( pBild->iNumColors > 0  &&  !pBild->bNoLUT )	{
		LPUCHAR	pPtr = hdf+lPos+114;
		WORD iCols = 0;

		*(LPWORD)pPtr = pBild->iNumColors;
		pPtr += 2;
		while( iCols < pBild->iNumColors ) {
			*pPtr++ = GetRValue( pBild->LUT[iCols] );
			*pPtr++ = GetGValue( pBild->LUT[iCols] );
			*pPtr++ = GetBValue( pBild->LUT[iCols] );
			iCols++;
		}
	}
	*(LPLONG)( hdf+lPos ) = 0l;
	if( pBmp->Links == pBild->Typ ) {
		*(LPLONG)( hdf+lPos ) = 1l;
	}
	else if( pBmp->Rechts == pBild->Typ ) {
		*(LPLONG)( hdf+lPos ) = 2l;
	}
	lPos += dd->len;
	dd++;
	*ndg++ = DFTAG_BILDHD; // 7
	*ndg++ = ref;
	wTagNr++;

	// Anzeigen, dass komprimiert
	if( CompressionType ) {
		// Flag, dass Dtaen komprimiert
		dd->tag = DFTAG_COMPRESSED;     // (#9)
		dd->ref = ref;
		dd->offset = CompressionType;   // NBIT+LZW
		dd->len = 0;
		dd++;
		*ndg++ = DFTAG_COMPRESSED;      //
		*ndg++ = ref;
		wTagNr++;
	}

	// Last is Numeric Data Group (#9 oder 10)
	dd->tag = DFTAG_NDG;
	dd->ref = ref;
	dd->offset = lPos;
	dd->len = wTagNr*2*sizeof( WORD );
	pWord = (LPUWORD)( hdf+lPos );
	for( i = 0;  i < 2*wTagNr;  i++ ) {
		pWord[i] = Big2Little( ndg_tags[i] );
	}
	lPos += dd->len;
	dd++;
	wTagNr++;

	// Alle Zahlen nach Motorola wandeln ...
	while( *dd_tags < dd ) {
		( *dd_tags )->tag = Big2Little( ( *dd_tags )->tag );
		( *dd_tags )->ref = Big2Little( ( *dd_tags )->ref );
		( *dd_tags )->offset = LongBig2Little( ( *dd_tags )->offset );
		( *dd_tags )->len = LongBig2Little( ( *dd_tags )->len );
		( *dd_tags )++;
	}
	*lPosition = lPos;
	return ( wTagNr );
}


// 30.11.97


// Komprimiert (so müglich) die Daten und gibt die neue Lünge zurück
// Eingabe: Zeiger auf Ausgangsdaten, Lünge (in Worten!), Flag, ob nach Bytes gewandelt werden soll
// Ausgabe: Lünge der komprimierten Daten (0=erfolglos)
//					pData zeigt dann entweder auf komprimierte Daten (return>0) oder Ausgangsdaten
LONG CompressData( LPUWORD *puData, LONG lLenSrc, WORD uMaxDaten, WORD *method )
{
	LPUWORD	pSrc = *puData;
	LPUCHAR	pHilf, pHilf1;
	LONG lNewLen, lLen, i;
	UWORD nBits = 1;

	// Anzahl Bits für Kodierung feststellen
	while( uMaxDaten > ( 1ul<<nBits )  &&  nBits < 16 )
		nBits++;
	lLen = ( lLenSrc*nBits+7l )/8l;   // So lang würe es maximal mit nBit-Decoding
	if( ( pHilf = (LPUCHAR)pMalloc( lLen+2 ) ) == NULL ) {
		return ( 0 );
	}

	if( ( pHilf1 = (LPUCHAR)pMalloc( lLen+2 ) ) == NULL ) {
		MemFree( pHilf );
		StatusLineRsc( E_MEMORY );
		return ( 0 );
	}

	// Erst einmal eine simple, billige Deltakompression versuchen
	// (danach sollten die Werte um Null streuen (sind signed char ...)
	//  also sollte eine Huffmankodierung besonders gut gehen)
	if( nBits > 8 )	{
		LPUCHAR	pDest = pHilf1;
		LONG j, lTemp, lLast = 0;

		for( j = i = 0;  i < lLenSrc  &&  j < lLen;  i++ ) {
			lTemp = ( (LONG)(ULONG)pSrc[i] )-lLast;
			lLast = pSrc[i];
			if( lTemp < -127l  ||  lTemp > 127 ) {
				pDest[j++] = 0x80u;
				pDest[j++] = (UCHAR)( lLast>>8 );
				pDest[j++] = (UCHAR)lLast;
			}
			else {
				pDest[j++] = (UCHAR)lTemp;
			}
		}
		if( j < lLen  &&  i == lLenSrc ) {
			// erfolgreich!
			lLen = j;
			uMaxDaten = 256;
			*method |= DELTA;
			// pHilf1 zeigt auf die eben konvertierten Daten
		}
	}


#if 1
	// Huffman-Kodierung
	if( uMaxDaten <= 8196u ) {
		LPULONG	pulFreq = (LPULONG)pMalloc( sizeof( long )*(ULONG)uMaxDaten+1ul );
		DWORD BitPos = 0;
		HUFFCODE hcode;

		// besonders erfolgversprechend bei Deltakompression
		if( *method&DELTA ) {
			// Byteweise arbeiten!
			for( i = 0;  i < lLen;  i++ ) {
				pulFreq[pHilf1[i]]++;
			}
			MemMove( pHilf+2, pulFreq, sizeof( long )*(ULONG)uMaxDaten );
			i = huffinit( uMaxDaten, pulFreq, &hcode );
			MemFree( pulFreq );
			if( i )	{
				// Huffinit war Ok
				BitPos = (DWORD)uMaxDaten*32ul+16ul;
				*(LPUWORD)pHilf = uMaxDaten;
				for( i = 0;  i < lLen  &&  ( BitPos/8 ) < (ULONG)( lLen-1 );  i++ ) {
					huffenc( pHilf1[i], pHilf, &BitPos, &hcode );
				}
				huffexit( &hcode );
				if( i == lLen )	{
					*method |= HUFFMAN;
					MemFree( pHilf1 );
					*puData = (LPUWORD)pHilf;
					return ( ( BitPos+7ul )/8ul );
				}
			}
		}
		else {
			// Hier sind die Daten noch im Wortformat!
			for( i = 0;  i < lLenSrc;  i++ ) {
				pulFreq[pSrc[i]]++;
			}
			MemMove( pHilf+2, pulFreq, sizeof( long )*uMaxDaten );
			i = huffinit( uMaxDaten, pulFreq, &hcode );
			MemFree( pulFreq );
			if( i )	{
				// Huffinit war Ok
				BitPos = (DWORD)uMaxDaten*32ul+16ul;
				*(LPUWORD)pHilf = uMaxDaten;
				for( i = 0;  i < lLenSrc  &&  BitPos/8 < (ULONG)( lLen-1 );  i++ ) {
					huffenc( ( *puData )[i], pHilf, &BitPos, &hcode );
				}
				huffexit( &hcode );
				if( i == lLenSrc ) {
					*method |= HUFFMAN;
					MemFree( pHilf1 );
					*puData = (LPUWORD)pHilf;
					return ( ( BitPos+7ul )>>3ul );
				}
			}
		}
	}
#endif

	if( ( *method&DELTA ) == 0 ) {
		// Es lüüt sich noch Platz durch Zusammenschieben gewinnen
		// Auüerdem steht im Array pHilf noch nix sinvolles ...
		if( nBits&7 ) {
			// Bits packen
			LPUCHAR	pPtr = pHilf1;
			ULONG ulTemp;
			WORD j;

			// 16 Bits nach n-Bits ...
			*pPtr++ = (UCHAR)nBits;
			for( ulTemp = j = i = 0;  i < lLenSrc;  i++ ) {
				ulTemp |= (ULONG)pSrc[i];
				j += nBits;
				if( j > 16 ) {
					*pPtr++ = (UCHAR)( ( ulTemp>>( j&15 ) )>>8 );
					*pPtr++ = (UCHAR)( ( ulTemp>>( j&15 ) ) );
					j -= 16;
				}
				ulTemp <<= nBits;
			}
			*method |= NBITS;
			lLen++;
		}
		else if( nBits == 8 ) {
			LPUCHAR	pPtr = pHilf1;

			*pPtr++ = nBits;
			lLen++;         // Bittiefe berücktsichtigen
			for( i = 0;  i < lLenSrc;  i++ ) {
				*pPtr++ = (BYTE)pSrc[i];
			}
			*method |= NBITS;
		}
		else if( nBits == 16 ) {
			// Nix packen
			// (beachte: Immer noch ist lLen == lLenSrc*2, sehr praktisch!)
			ASSERT(	lLen == lLenSrc*2 );
			MemMove( pHilf1, pSrc, lLen );
		}
	}

	// Test LZW-Compression!
	lNewLen = LZWCompressBlock( INIT_BITS, pHilf, lLen, pHilf1, lLen );
	if( lNewLen < 0 ) {
		// Daten wurden lünger ... war wohl nix!
		MemFree( pHilf );
		if( nBits == 16  &&  *method == 0 ) {
			// Lieüen sich nicht packen!
			MemFree( pHilf1 );
			return ( 0 );
		}
		// Also nur Bitgepackt gespeichert
		*puData = (LPUWORD)pHilf1;
		return ( lLen+1 );
	}

	// Sonst LZW-gepackt speichern
	MemFree( pHilf1 );
	*method |= LZW;
	*puData = (LPUWORD)pHilf;
	return ( lNewLen );
}


// 22.9.98


#define SNOM_HD	"SNOM Binary Display Informations"
#define SNOM_HD_LEN	33

// Anzahl im Header
#define MAX_HD	10

// Schreibt Header und eigentliche Daten
BOOLEAN	WriteHDF( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei, BOOLEAN DoCompress )
{
	LPSNOMDATA pSnom = &( pBmp->pSnom[iAktuell] );
	DD_TAG HUGE *dd, HUGE *dd_tags; // Sollte wirklich reichen ...
#if LEN_OK
	DD_HEADER HUGE *dh;
#endif
	LPUWORD	pTopo, pError, pLumi;
	LONG lTopoLen = 0, lErrorLen = 0, lLumiLen = 0;
	LPLONG pTopoStart, pErrorStart, pLumiStart;
	WORD TopoCompress = NONE, ErrorCompress = NONE, LumiCompress = NONE;
	LPUCHAR	hdf;
	BOOL neuen_header = FALSE;                      // Zeigt an, dass schon ein gültiger Header geschrieben wurde!

	OFSTRUCT of;
	HFILE hFile;
	LONG lPos, i;
	LPLONG next_pos;
	LPWORD count_pos;
	WORD tag_nr = 0;

	if( pBmp == NULL  ||  szDatei == NULL ) {
		return ( FALSE );
	}

	if( pSnom->Topo.puDaten == NULL ) {
		what &= ~TOPO;
	}
	if( pSnom->Error.puDaten == NULL ) {
		what &= ~ERRO;
	}
	if( pSnom->Lumi.puDaten == NULL ) {
		what &= ~LUMI;
	}
	if( !what ) {
		return ( FALSE );   // Nix auszugeben!
	}
	if( HFILE_ERROR == ( hFile = OpenFile( szDatei, &of, OF_WRITE | OF_CREATE ) ) )	{
		// FEHLERMELDUNG!
		return ( FALSE );
	}

	// Erst mal Platz für Header schaffen
	hdf = (LPUCHAR)pMalloc( 0x8000l );      // sollte für jeden Header reichen
	if( hdf == NULL ) {
		StatusLineRsc( E_MEMORY );
		return ( FALSE );
	}

	// Falls Kompression gewünscht, Daten versuchen zu komprimieren
	// kann natürlich an zuwenig Speicher scheitern ...
	if( what&TOPO )	{
		// Zeiger auf gültige Daten holen
		pTopo = pSnom->Topo.puDaten;
		lTopoLen = pSnom->w*pSnom->h*2l;
		if( (LONG)pTopo < 256L ) {      // Indirekt adressiert?
			pTopo = pBmp->pSnom[(LONG)pTopo-1].Topo.puDaten;
		}
		// komprimieren?
		if( DoCompress ) {
			lTopoLen = CompressData( &pTopo, pSnom->w*pSnom->h, pSnom->Topo.uMaxDaten, &TopoCompress );
			if( !TopoCompress ) {
				lTopoLen = pSnom->w*pSnom->h*2l;
			}
		}
	}

	// gleiches für Fehler
	if( what&ERRO )	{
		// Zeiger auf gültige Daten holen
		pError = pSnom->Error.puDaten;
		lErrorLen = pSnom->w*pSnom->h*2l;
		if( (LONG)pError < 256L ) {     // Indirekt adressiert?
			pError = pBmp->pSnom[(LONG)pError-1].Error.puDaten;
		}
		// Evt. komprimieren
		if( DoCompress ) {
			lErrorLen = CompressData( &pError, pSnom->w*pSnom->h, pSnom->Error.uMaxDaten, &ErrorCompress );
			if( !ErrorCompress ) {
				lErrorLen = pSnom->w*pSnom->h*2l;
			}
		}
	}

	// und Lumineszens
	if( what&LUMI )	{
		// Zeiger auf gültige Daten holen
		pLumi = pSnom->Lumi.puDaten;
		lLumiLen = pSnom->w*pSnom->h*2l;
		if( (LONG)pLumi < 256L ) {      // Indirekt adressiert?
			pLumi = pBmp->pSnom[(LONG)pLumi-1].Lumi.puDaten;
		}
		// Evt. komprimieren
		if( DoCompress ) {
			lLumiLen = CompressData( &pLumi, pSnom->w*pSnom->h, pSnom->Lumi.uMaxDaten, &LumiCompress );
			if( !LumiCompress ) {
				lLumiLen = pSnom->w*pSnom->h*2l;
			}
		}
	}
	// Nun haben pTopo, lTopoLen, pError, lErrorLen, pLumi und lLumiLen gültige Werte!

	// und nun HDF-Header zusammenbauen
	MemMove( (LPSTR)hdf, (LPSTR)HDFMAGIC, 4 );      // Magic-Number

	// Aktuelle Position berechnen (plus etwas Toleranz)
	lPos = MAGICLEN+sizeof( DD_HEADER )*3+40*sizeof( DD_TAG );

	// Header schreiben, zwei Header für zwei Bitmaps
	// (damit werden die Daten vor dem PSI-Programm versteckt ...)
#if LEN_OK
	dh = (DD_HEADER HUGE*)( hdf+MAGICLEN );
	count_pos = &( dh->count );       // Merken für dynamische Berechnung
	next_pos = &( dh->next );
	// Tags werden gleich in den Speicher geschrieben ...
	dd_tags = dd = (DD_TAG HUGE*)( hdf+sizeof( DD_HEADER )+MAGICLEN );
#else
	count_pos = (LPLONG)( hdf+MAGICLEN );
	next_pos = (LPLONG)( hdf+2+MAGICLEN );
	// Tags werden gleich in den Speicher geschrieben ...
	dd_tags = dd = (DD_TAG HUGE*)( hdf+6+MAGICLEN );
#endif
	// Maschinentag (#1)
	dd->tag = DFTAG_MT;
	dd->ref = DFMT_PC;
	dd->offset = 0L;
	dd->len = 0L;
	dd++;
	tag_nr++;

	// Versionstag (3.2 compatible) (#2)
	dd->tag = DFTAG_VERSION;
	dd->ref = 1;
	dd->offset = lPos;
	dd->len = VERSIONLEN;
	MemMove( hdf+lPos, VERSION3_2, VERSIONLEN );
	dd++;
	tag_nr++;
	lPos += VERSIONLEN;

	// PSI-Tag-Identifier (#3)
	dd->tag = DFTAG_TID;
	dd->ref = DFTAG_PSI;
	dd->offset = lPos;
	dd->len = PSI_ID_LEN;
	MemMove( hdf+lPos, PSI_ID, PSI_ID_LEN );
	dd++;
	tag_nr++;
	lPos += PSI_ID_LEN;

	// PSI-Tag-Identifier (#4)
	dd->tag = DFTAG_TID;
	dd->ref = DFTAG_PSIHD;
	dd->offset = lPos;
	dd->len = PSI_HD_LEN;
	MemMove( hdf+lPos, PSI_HD, PSI_HD_LEN );
	dd++;
	tag_nr++;
	lPos += PSI_HD_LEN;

	// PSI-Tag-Identifier (#5)
	dd->tag = DFTAG_TID;
	dd->ref = DFTAG_BILDHD;
	dd->offset = lPos;
	dd->len = PSI_HD_LEN;
	MemMove( hdf+lPos, SNOM_HD, SNOM_HD_LEN );
	dd++;
	tag_nr++;
	lPos += SNOM_HD_LEN;

	// PSI-Tag-Identifier (#6)
	dd->tag = DFTAG_PSI;
	dd->ref = 1;
	dd->offset = lPos;
	dd->len = 4;
	MemMove( hdf+lPos, "\04\00\00\02", 4 );
	dd++;
	tag_nr++;
	lPos += 4;

	// weglassen PSI_SPEC
	// weglassen DFTAG_ID

	// Scanparameter für snomputz (if there #7)
	if( pBmp->Typ == SNOMPUTZ ) {
		dd->tag = DFTAG_SNOMPUTZ;
		dd->ref = 1;
		dd->offset = lPos;
		dd->len = sizeof( SNOMPUTZ_SCANDATA );
		MemMove( hdf+lPos, pBmp->pExtra, sizeof( SNOMPUTZ_SCANDATA ) );
		lPos += dd->len;
		dd++;
		tag_nr++;
	}

	// Number-Types: Achse 1 (#7)
	dd->tag = DFTAG_NT;
	dd->ref = 0x101;
	dd->offset = lPos;
	dd->len = 4l;
	MemMove( hdf+lPos, "\x01\x05\x20\x04", 4l );    // Intel 32 Bit float
	// Als Long würe es nicht portabel!!!
	dd++;
	tag_nr++;
	lPos += 4l;

	// Number-Types: Achse 2 (#8)
	dd->tag = DFTAG_NT;
	dd->ref = 0x102;
	dd->offset = lPos;
	dd->len = 4l;
	MemMove( hdf+lPos, "\x01\x05\x20\x04", 4l );    // Intel float
	dd++;
	tag_nr++;
	lPos += 4l;

	// File-identifier (#9)
	dd->tag = DFTAG_FID;
	dd->ref = 1;
	dd->offset = lPos;
	dd->len = lstrlen( szDatei )+1;   // Selbst leere Felder sollten min. 1 Byte lang sein ...
	lstrcpy( (LPSTR)( hdf+lPos ), szDatei );
	lPos += dd->len;
	dd++;
	tag_nr++;

	// Kommentar (#10)
	/* Zuerst evt. Datum einfügen */
	i = lstrlen( pBmp->pKommentar );
	if( i == 0  ||  strstr( pBmp->pKommentar, "Measured = " ) == NULL ) {
		if( i < 970 ) {
			wsprintf( hdf+lPos, "Measured = %hd.%hd.%hd %02hd:%02hd:%02hd\n", pBmp->iTag, pBmp->iMonat, pBmp->iJahr, pBmp->iStunde, pBmp->iMinute, pBmp->iSekunde );
			lstrcat( hdf+lPos, pBmp->pKommentar );
			lstrcpy( pBmp->pKommentar, hdf+lPos );
		}
	}
	dd->tag = DFTAG_FD;
	dd->ref = 1;
	dd->offset = lPos;
	dd->len = lstrlen( pBmp->pKommentar )+1;  // Selbst leere Felder sollten min. 1 Byte lang sein ...
	lstrcpy( (LPSTR)( hdf+lPos ), pBmp->pKommentar );
	lPos += dd->len;
	dd++;
	tag_nr++;

	// Alle Zahlen nach Motorola wandeln ...
	dd = dd_tags;
	for( i = 0;  i < tag_nr;  i++ )	{
		dd->tag = Big2Little( dd->tag );
		dd->ref = Big2Little( dd->ref );
		dd->offset = LongBig2Little( dd->offset );
		dd->len = LongBig2Little( dd->len );
		dd++;
	}
	dd_tags = dd;

	// Alles in den ersten Header für die Topografie!
	if( what&TOPO )	{
		neuen_header = CreateBildHDF( hdf, &lPos, &dd_tags, 2, pBmp, pSnom->w, pSnom->h, &( pSnom->Topo ), &pTopoStart, lTopoLen, TopoCompress );
		*count_pos = Big2Little( neuen_header+tag_nr );
	}

	// zweiter Header für Fehlersignal
	if( what&ERRO )	{
		if( neuen_header ) {
			// Neuen (2.) Header (nur wenn schon einer "verbraucht" wurde)
#if LEN_OK
			dh = (DD_HEADER HUGE*)dd_tags;
#if 0
			dh->count = 9;
			if( ErrorCompress > 0 ) {
				dh->count++;
			}
			dh->count = Big2Little( dh->count );
#endif
			// Zeiger auf diesen Header setzen!
			*next_pos = LongBig2Little( (LONG)dd_tags-(LONG)hdf );
			count_pos = &( dh->count );
			next_pos = &( dh->next );
			dh++;
			dd_tags = (DD_TAG HUGE*)dh;
#else
			LPUCHAR pHilf = (LPUCHAR)dd_tags;

			if( ErrorCompress > 0 ) {
				*(LPUWORD)pHilf = Big2Little( 10 );
			}
			else {
				*(LPUWORD)pHilf = Big2Little( 9 );
			}
			// Zeiger auf diesen Header setzen!
			*next_pos = LongBig2Little( (LONG)pHilf-(LONG)hdf );
			count_pos = pHilf;
			next_pos = (LPLONG)( pHilf+2 );
			dd_tags = (DD_TAG HUGE*)( pHilf+6 );
#endif
		}
		neuen_header = CreateBildHDF( hdf, &lPos, &dd_tags, 4, pBmp, pSnom->w, pSnom->h, &( pSnom->Error ), &pErrorStart, lErrorLen, ErrorCompress );
		*count_pos = Big2Little( neuen_header );
	}

	// Gleiches für die Lumineszens ausführen
	if( what&LUMI )	{
		if( neuen_header ) {
			// Neuen (2.) Header (nur wenn schon einer "verbraucht" wurde)
#if LEN_OK
			dh = (DD_HEADER HUGE*)dd_tags;
			dh->count = 9;
			if( LumiCompress > 0 ) {
				dh->count++;
			}
			dh->count = Big2Little( dh->count );
			// Zeiger auf diesen Header setzen!
			*next_pos = LongBig2Little( (LONG)dd_tags-(LONG)hdf );
			next_pos = &( dh->next );
			dh++;
			dd_tags = (DD_TAG HUGE*)dh;
#else
			LPUCHAR pHilf = (LPUCHAR)dd_tags;

			if( LumiCompress > 0 ) {
				*(LPUWORD)pHilf = Big2Little( 10 );
			}
			else {
				*(LPUWORD)pHilf = Big2Little( 9 );
			}
			// Zeiger auf diesen Header setzen!
			*next_pos = LongBig2Little( (LONG)pHilf-(LONG)hdf );
			next_pos = (LPLONG)( pHilf+2 );
			dd_tags = (DD_TAG HUGE*)( pHilf+6 );
#endif
		}
		neuen_header = CreateBildHDF( hdf, &lPos, &dd_tags, 8, pBmp, pSnom->w, pSnom->h, &( pSnom->Lumi ), &pLumiStart, lLumiLen, LumiCompress );
	}
	*next_pos = 0l;

	// before save Header, get the pointer right
	if( what&TOPO ) {
		*pTopoStart = LongBig2Little( lPos );
	}
	if( what&ERRO ) {
		*pErrorStart = LongBig2Little( lPos+lTopoLen );
	}
	if( what&LUMI ) {
		*pLumiStart = LongBig2Little( lPos+lErrorLen+lTopoLen );
	}
	_hwrite( hFile, hdf, lPos );
	MemFree( hdf );

	// Zuerst die Topografie-Daten
	if( what&TOPO )	{
		LONG lWritten;

		if( !TopoCompress  &&  pSnom->Topo.uMaxDaten > 0x7fffu ) {
			// Evt. in Signed verwandeln ...
			for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
				pTopo[i] -= 0x8000u;
			}
		}
		lWritten = _hwrite( hFile, (LPCSTR)pTopo, lTopoLen );
		if( !TopoCompress  &&  pSnom->Topo.uMaxDaten > 0x7fffu ) {
			// und wieder unsigned daraus machen ...
			if( pSnom->Topo.uMaxDaten > 0x7fffu ) {
				for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
					pTopo[i] += 0x8000u;
				}
			}
		}
		if( TopoCompress ) {
			MemFree( pTopo );
		}
		if( lWritten != lTopoLen ) {
			// Konnte nicht alles schreiben!
			_lclose( hFile );
			FehlerRsc( E_HARDDISK );
			return ( FALSE );
		}
	}

	// Dann (so vorhanden) die Fehler
	if( what&ERRO )	{
		LONG lWritten;

		// Signed needed?
		if( !ErrorCompress  &&  pSnom->Error.uMaxDaten > 0x7fffu ) {
			for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
				pError[i] -= 0x8000u;
			}
		}
		lWritten = _hwrite( hFile, (LPCSTR)pError, lErrorLen );
		// und wieder unsigned daraus machen ...
		if( !ErrorCompress  &&  pSnom->Error.uMaxDaten > 0x7fffu ) {
			for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
				pError[i] += 0x8000u;
			}
		}
		if( ErrorCompress ) {
			MemFree( pError );
		}

		if( lWritten != lErrorLen ) {
			// Konnte nicht alles schreiben!
			_lclose( hFile );
			FehlerRsc( E_HARDDISK );
			return ( FALSE );
		}
		lPos += lWritten;
	}

	// finnaly (if present) Lumineszence
	if( what&LUMI )	{
		LONG lWritten;

		// Evt. in Signed verwandeln ...
		if( !LumiCompress  &&  pSnom->Lumi.uMaxDaten > 0x7fffu ) {
			for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
				pLumi[i] -= 0x8000u;
			}
		}
		lWritten = _hwrite( hFile, (LPCSTR)pLumi, lLumiLen );
		// und wieder unsigned daraus machen ...
		if( !LumiCompress  &&  pSnom->Lumi.uMaxDaten > 0x7fffu ) {
			for( i = 0;  i < pSnom->w*pSnom->h;  i++ ) {
				pLumi[i] += 0x8000u;
			}
		}
		if( LumiCompress ) {
			MemFree( pLumi );
		}

		if( lWritten != lLumiLen ) {
			// Konnte nicht alles schreiben!
			_lclose( hFile );
			FehlerRsc( E_HARDDISK );
			return ( FALSE );
		}
	}

	_lclose( hFile );
	return ( TRUE );
}
// 27.7.97


