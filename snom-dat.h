/*****************************************************************************************
****	SNOM-DAT: Routinen zum Laden und Speicher (Packroutinen siehe SNOM-PAC)
*****************************************************************************************/

/* Konvertiert einen Datenblock in ein Snombild (Topografie, Fehler oder Lumineszens)
 * Wird zuletzt immer dann aufgerufen, wenn Daten geladen sind.
 * Die Daten werden als erstes Element (pBmp->pSnom[0]) eingefügt
 * ww beziechnet die Breite einer Zeile in Bytes; dies kann mehr sein, als die Weite w, der Rest
 * wird ignoriert.
 * uBits ist die Breite in Bits 1..16, uAdd ein Offset, der aufaddiert werden soll (meist 0 oder 0x8000)
 * für signed/unsigned.
 */
BOOLEAN	LadeBlock( LPBMPDATA pBmp, LPVOID pvPtr, LONG w, LONG ww, LONG h, int uBits, WORKMODE Mode, UWORD uAdd, BOOLEAN InitRest );
// 26.7.97

//  Liest eine Datei im Digital-Format ein
BOOL	ReadDigital( HFILE hFile, LPBMPDATA pBmp );
// 6.8.97

//  Liest eine Datei in beliebigem Format ein
//	Die Routine selber interpretiert nur das HDF-Format
BOOLEAN	ReadAll( LPSTR datei, LPBMPDATA pBmp );
// 10.8.97

//**** Schreibt RHK-Datei (Format: siehe in snom-dat.c) ****
BOOL	WriteRHK( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei );
// 1.4.00

//**** Schreibt Digital-Datei (Format: siehe in snom-dat.c) ****
BOOL	WriteDigital( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei );
// 5.4.00

// Schreibt Header und eigentliche Daten
BOOLEAN	WriteHDF( LPBMPDATA pBmp, WORD iAktuell, WORKMODE what, LPSTR szDatei, BOOLEAN DoCompress );
// 27.7.97




