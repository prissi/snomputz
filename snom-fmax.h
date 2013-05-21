/**************************************************************************************
****	Mathematische und verwandte Routinen
**************************************************************************************/


UWORD ListOfMaxima( LPUWORD puData, LONG width, LONG height, UWORD uMaxData, LONG tolerance, XYZ_COORD **pOutputKoord );

void CalcDotRadius( LPUWORD puData, const LONG w, const LONG h, const UWORD zero_level, int iAveBefore, UWORD iDotNr, XYZ_COORD *pDots, const WORD quantisation, const BOOL recenter, const int maxradius );

//default smoothing level
#define DOT_AVERAGE (1)

