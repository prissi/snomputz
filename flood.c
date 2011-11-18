// Attention, dirty coding here
// beter example can be found at http://www.drawit.co.nz/Developers.htm
// to save stack size, we are using lots of global and inlines!!!

static LPUBYTE	pFillArea;
static LONG		iFillAreaWW, iFillW, iFillH;
static LPUWORD	pFillData;
static UWORD	iFillLevel, FillHigher;

inline BOOLEAN GetPixel( LONG x, LONG y )
{
	return (pFillArea[(iFillH-1-y)*iFillAreaWW+(x>>3)] & (BYTE)(0x080>>(x&7)))!=0;
}

inline void SetPixel( UBYTE *pArea, LONG bw, LONG w, LONG h, LONG x, LONG y )
{
	pFillArea[(iFillH-1-y)*iFillAreaWW+(x>>3)] |= (BYTE)(0x080>>(x&7));
}

VOID	FloodArea( WORD	x, WORD y )
{
	if(  !GetPixel(x,y)  &&  FillHigher^(iFillLevel<=pFillData[iFillW*(LONG)y+x]  )
	{
		FloodArea( x-1, y );
		FloodArea( x+1, y );
		FloodArea( x, y-1 );
		FloodArea( x, y+1 );
	}
}


// Mark all neighbouring points higher (lower) as the startpoint in the bitmap pArea (iAreaWW wide)
BOOLEAN	MarkUpperArea( LPUWORD puData, LONG w, LONG h, UBYTE *pArea, 
					   LONG iAreaWW, LONG iStartX, LONG iStartY, BOOLEAN higher )
{
ASSERT( pBild!=NULL  &&  pBild->puDaten!=NULL  &&  pArea!=0  &&  iStartX<=w  &&  iStartY<=h  );

	pFillData = puData;
	iFillLevel = pFillData[w*iStartY+iStartX];
	if(  !higher   )
		iFillLevel++;
	iFillW = w;
	iFillH = h;
	iFillArea = pArea;
	iFillAreaWW = iAreaWW;
	FloodArea( iStartX, iStartY );
}
// End of dirty coding alert