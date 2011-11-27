/**************************************************************************************
****	Mathematische und verwandte Routinen
**************************************************************************************/

#include <stdio.h>
#include <math.h>

extern "C" {
#include "snomputz.h"
#include "snom-typ.h"
#include "snom-mem.h"
};

int CompareXYZ_COORD( const void *a, const void *b )
{
	return ( (int)( ( *(XYZ_COORD*)a ).hgt ) - (int)( ( *(XYZ_COORD*)b ).hgt ) );
}


/** the following constants are used to set bits corresponding to pixel types */
typedef enum {
	MAXIMUM = 1,            // marks local maxima (irrespective of noise tolerance)
	LISTED = 2,             // marks points currently in the list
	PROCESSED = 4,          // marks points processed previously
	MAX_AREA = 8,           // marks areas near a maximum, within the tolerance
	EQUAL = 16,             // marks contigous maximum points of equal level
	MAX_POINT = 32,         // marks a single point standing for a maximum
	ELIMINATED = 64        // marks maxima that have been eliminated before watershed
};


static int next_neighbour_x[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };
static int next_neighbour_y[8] = { -1, 0, 1, 1, 1, 0, -1, -1 };


extern "C" UWORD ListOfMaxima( LPUWORD puData, LONG width, LONG height, LONG ww, UWORD uMaxData, LONG tolerance, XYZ_COORD **pOutputKoord );


UWORD ListOfMaxima( LPUWORD puData, LONG width, LONG height, LONG ww, UWORD uMaxData, LONG tolerance, XYZ_COORD **pOutputKoord )
{
	char *maxmap = (char*)pMalloc( width*height );

	int maxSortingError = 0;

	int nMax = 0;
	int nResult = 0;
	XYZ_COORD *maxpts = (XYZ_COORD*)pMalloc( ( width*height )*sizeof( XYZ_COORD ) );
	XYZ_COORD *pResult = 0;
	XYZ_COORD *pList = 0;

	bool excludeEdges = true;

	assert( maxmap );

	for( int y = 1; y < height-1; y++ ) {         // find local maxima now

		for( int x = 1; x < width-1; x++ ) {      // for better performance with rois, restrict search to roi

			UWORD vTrue = puData[( y*ww )+x];
			boolean isMax = true;

			/* check wheter we have a local maximum.
			   Note: For an EDM, we need all maxima: those of the EDM-corrected values
			   (needed by findMaxima) and those of the raw values (needed by cleanupMaxima) */
			for( int d = 0; d < 8; d++ ) {
				int check_x = x+next_neighbour_x[d];
				int check_y = y+next_neighbour_y[d];
				// compare with the 8 neighbor pixels
//				if(  check_x>=0  &&  width>check_x  &&  check_y>=0  &&  height>check_y  ) {
				{
					UWORD vNeighborTrue = puData[( check_y*ww )+check_x];
					if( vNeighborTrue > vTrue ) {
						isMax = false;
						break;
					}
				}
			}
			if( isMax ) {
				maxmap[( y*width )+x] = MAXIMUM;
				maxpts[nMax].x = x;
				maxpts[nMax].y = y;
				maxpts[nMax].hgt = vTrue;
				nMax++;
			}
		} // for x
	} // for y

	// now we have a list of all local maxima 0> sort
	qsort( maxpts, nMax, sizeof( *maxpts ), CompareXYZ_COORD );

	pList = (XYZ_COORD*)pMalloc( width*height*sizeof( XY_COORD ) );  // temporary
	pResult = (XYZ_COORD*)pMalloc( (nMax+1)*sizeof( XYZ_COORD ) );     // final ... );

	// now process all maximas, heighest first
	for( int iMax = nMax-1;  iMax >= 0;  iMax-- ) {
		int x0 = maxpts[iMax].x;
		int y0 = maxpts[iMax].y;
		long offset0 = x0 + y0*width;
		LONG v0 = maxpts[iMax].hgt;
		boolean sortingError;

		if( maxmap[offset0] & PROCESSED ) {
			// already processed
			continue;
		}
		//we create a list of connected points and start the list at the current maximum
		do {                            //repeat if we have encountered a sortingError
			int listLen = 1;        //number of elements in the list
			int listI = 0;          //index of current element in the list
			int isEdgeMaximum = ( x0 == 0 || x0 == width-1 || y0 == 0 || y0 == height-1 );
			boolean maxPossible = true; //it may be a true maximum
			double xEqual = x0; //for creating a single point: determine average over the
			double yEqual = y0; //  coordinates of contiguous equal-height points
			int nEqual = 1; //counts xEqual/yEqual points that we use for averaging
			sortingError = false; //if sorting was inaccurate: a higher maximum was not handled so far

			pList[0] = maxpts[iMax];
			maxmap[ offset0 ] |= ( EQUAL|LISTED ); //mark first point as equal height (to itself) and listed

			do {
				//while neigbor list is not fully processed (to listLen)
				int x = pList[listI].x;
				int y = pList[listI].y;

				for( int d = 0; d < 8; d++ ) {
					int x2 = x+next_neighbour_x[d];
					int y2 = y+next_neighbour_y[d];
					long offset2 = ( y2*width )+x2;
					// compare with the 8 neighbor pixels
					if( ( x2 >= 0  &&  x2 < width  &&  y2 >= 0  &&  y2 < width )  &&  ( maxmap[offset2]&LISTED ) == 0 ) {
						if( ( maxmap[offset2]&PROCESSED ) != 0 ) {
							maxPossible = false; //we have reached a point processed previously, thus it is no maximum now
							//if(x0<25&&y0<20)IJ.write("x0,y0="+x0+","+y0+":stop at processed neighbor from x,y="+x+","+y+", dir="+d);
							break;
						}
						LONG v2 = puData[x2 + ( ww*y2 )];
						if( v2 > v0 + maxSortingError ) {
							maxPossible = false; //we have reached a higher point, thus it is no maximum
							//if(x0<25&&y0<20)IJ.write("x0,y0="+x0+","+y0+":stop at higher neighbor from x,y="+x+","+y+", dir="+d+",value,value2,v2-v="+v0+","+v2+","+(v2-v0));
							break;
						}
						else if( v2 >= v0-tolerance ) {
							if( v2 > v0 ) { //maybe this point should have been treated earlier
								sortingError = true;
								offset0 = offset2;
								v0 = v2;
								x0 = x2;
								y0 = y2;
							}
							pList[listLen].x = x2;
							pList[listLen].y = y2;
							listLen++; //we have found a new point within the tolerance
							maxmap[offset2] |= LISTED;
							if( x2 == 0 || x2 == width-1 || y2 == 0 || y2 == height-1 ) {
								isEdgeMaximum = true;
								if( excludeEdges ) {
									maxPossible = false;
									break; //we have an edge maximum;
								}
							}
							if( v2 == v0 ) { //prepare finding center of equal points (in case single point needed)
								maxmap[offset2] |= EQUAL;
								xEqual += x2;
								yEqual += y2;
								nEqual++;
							}
						}
					} // if isWithin & not LISTED
				} // for directions d
				listI++;
			} while( listI < listLen );

			assert(  !sortingError  );      //if x0,y0 was not the true maximum but we have reached a higher one

			{
				int resetMask = ~( maxPossible ? LISTED : ( LISTED|EQUAL ) );
				xEqual /= nEqual;
				yEqual /= nEqual;
				double minDist2 = 1e20;
				int nearestI = 0;
				for( listI = 0; listI < listLen; listI++ ) {
					int x = pList[listI].x;
					int y = pList[listI].y;
					long offset = y*ww + x;
					maxmap[offset] &= resetMask;            //reset attributes no longer needed
					maxmap[offset] |= PROCESSED;            //mark as processed
					if( maxPossible ) {
						maxmap[offset] |= MAX_AREA;
						if( ( maxmap[offset]&EQUAL ) != 0 ) {
							double dist2 = ( xEqual-x )*(double)( xEqual-x ) + ( yEqual-y )*(double)( yEqual-y );
							if( dist2 < minDist2 ) {
								minDist2 = dist2;       //this could be the best "single maximum" point
								nearestI = listI;
							}
						}
					}
				} // for listI
				if( maxPossible ) {
					// add them to list
					int x = pList[listI].x;
					int y = pList[listI].y;
					long offset = y*ww + x;
					maxmap[offset] |= MAX_POINT;
					if( !( excludeEdges && isEdgeMaximum ) ) {
						pList[nearestI].hgt = puData[ x + y*ww ];
						pResult[nResult++] = pList[nearestI];
					}
				}
			} //if !sortingError
		} while( sortingError );                         //redo if we have encountered a higher maximum: handle it now.
	} // for all maxima iMax

	MemFree( maxmap );
	MemFree( pList );
	if( pOutputKoord == NULL ) {
		MemFree( pResult );
	}
	else {
		*pOutputKoord = pResult;
	}
	return ( nResult );
}


