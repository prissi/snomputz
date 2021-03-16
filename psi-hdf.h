// Was man sich so zusammenreimen kann: Der PSI-HDF-Header (TAG 8009)


#ifndef __PSI_HDF_HEADER
#define __PSI_HDF_HEADER

#define DFTAG_PSI			0x8001
#define	DFTAG_PSIHD		0x8009
#define	DFTAG_PSISPEC 0x800A

#define	PSI_ID			"PSI TAG for ProScanVersion"
#define PSI_ID_LEN  28L
#define	PSI_HD			"PSI TAG for binary header block"
#define	PSI_HD_LEN	0x20
#define PSI_SPEC		"PSI TAG for Spectroscopy data"

#pragma pack(2)

typedef struct
{
	long		u1;							// Bedeutung der Parameter mit u? sind unbekannt
	char		cTitle[32];			// Topography o.ä.
	char		cInstrument[8];	// Benütztes Instument (ist evt. auch nur eine 3 Byte-Kennung)
	short		x_dir, y_dir;		// Scanrichtung (0/ right to left / top to bottom)
	char		cShowOffset, cNoUnits; // Werden nun einfach für Offsets missbraucht ...
	short		iCol, iRow;			// Zeilen und Spalten
	char		u5[12];
	float		fW, fH;					// Breite und Hoehe des Bildes
	float		fXOff0, fYOff0;   // X und Y Offset
	float		fRot;						// Rotation
	float		u6;
	float		fLinePerSec;		// Scangeschwindigkeit
	float		fSetpoint;
	char		cSetPointUnit[8];
	float		fSampleBias,	fTipBias;	// können ebensogut falsch sein; lässt man am besten = 0!
	float		fZGain;
	char		cZGainUnit[8];
	short	iMin, iMax;
} PSI_HEADER;

#endif

#define DFTAG_BILDHD	0x80AA
#define DFTAG_SNOMPUTZ			0x80AB


