/* Typdefinitionen für's Scannen */
/* Werden in SNOMPUTZ auch verwendet!!! */

#ifndef _PIDPAR
#define _PIDPAR

#ifndef TOPO
#define	TOPO (1)
#define ERRO (2)
#define LUMI (4)
#endif


/* Parameterdefinitionen */
#define SET_PID_PARAMETER ('p')
#define ABORT ('q')
#define EXIT ('e')
#define GET_VALUE ('a')
#define SET_SCAN_PARAMETER ('s')
#define SET_POSITION ('o')         
#define COARSE_FORWARD ('f')
#define COARSE_BACK ('b')
#define VOID_CMD ('v')
#define INVERT_PIEZO_CMD ('i')
#define EXPONENTIALLY_CMD ('x')
#define TUNNELVOLTAGE_CMD ('t')
#define GET_SPS_CMD ('g')


/* Für Definition Scanrichtung */
#define LINKS2RECHTS (1)
#define RECHTS2LINKS (2)

/* Rückgabewert: Abbruch des Scans */
#define ABORTSCAN	(0x80008000ul)
/* Rückgabewert: Hierkommen die Daten */
#define STARTSPS	(0x80000000ul)
/* Rückgabewert: Hier kommt eine Zeile */
#define STARTLINE	(0x00008000ul)


#pragma pack(4)
typedef struct							   
{
	long	iPts;							/* in Anzahl Schritten */
	long	lFreq;						/* Schritte pro Zyklus */
	long	iEndX, iEndY;			/* X - und Y-Offset in Punkten */
	long	iStartX, iStartY;	/* X - und Y-Offset in Punkten */
} GOPAR;

#pragma pack(4)
typedef struct							   
{
	long	iModus;			/* Was soll gescannt werden? */
	long	lFreq;			/* Linien pro Sekunde */
	long	iXPts, iYPts;	/* Punkte X- und Y-Richtung im Bild */
	long	iW, iH;			/* Bildweite/Höhe in Pixel */
	long	iXOff, iYOff;	/* X - und Y-Offset in Punkten */
	long	lWinkel;		/* Winkel im Bogenmaß */
	long	iDir;			/* 1=Links->Rechts, 2 = Rechts->Links, 3=beide */
	long	iOffset;		/* Offset für Messungen Rechts->Links */
} SCANPAR;



#endif


