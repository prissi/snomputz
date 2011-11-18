/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

// this is all you need for a rudimental hdf-support ...

#include "hntdefs.h"
#include "htags.h"

//////////////////////////////////////////////////////////////////////////////
// HFILE.H

/* Magic cookie for HDF data files */
#define MAGICLEN 4  /* length */
#define HDFMAGIC "\016\003\023\001"     /* ^N^C^S^A */

// Standard Version Header ...
#define VERSIONLEN	0x2c
#define VERSION3_2	"\0\0\0\3\0\0\0\2\0\0\0\2NCSA HDF Version 3.2 compatible"


/* ----------------------- Internal Data Structures ----------------------- */
/* The internal structure used to keep track of the files opened: an
	 array of filerec_t structures, each has a linked list of ddblock_t.
	 Each ddblock_t struct points to an array of dd_t structs.

	 File Header(4 bytes)
	 ===================
   <--- 32 bits ----->
   ------------------
   |HDF magic number |
   ------------------

   HDF magic number - 0x0e031301 (Hexadecimal)

   Data Descriptor(DD - 12 bytes)
   ==============================
	 <-  16 bits -> <- 16 bits -> <- 32 bits -> <- 32 bits ->
	 --------------------------------------------------------
	 |    Tag      | reference   |  Offset     |  Length    |
   |             | number      |             |            |
   --------------------------------------------------------
        \____________/
               V
        tag/ref (unique data indentifier in file)
   
   Tag  -- identifies the type of data, 16 bit unsigned integer whose
           value ranges from 1 - 65535. Tags are assigned by NCSA.
           Current tag assingments are:
           00001 - 32767  - reserved for NCSA use
					 32768 - 64999  - user definable
           65000 - 65535  - reserved for expansion of format

   Refererence number - 16 bit unsigned integer whose assignment is not
          gauranteed to be consecutive. Provides a way to distinguish 
					elements with the same tag in the file.

   Offset - Specifies the byte offset of the data element from the 
            beginning of the file - 32 bit unsigned integer

	 Length - Indicates the byte length of the data element
						32 bit unsigned integer

   Data Descriptor Header(DDH - 6 bytes)
   ====================================
   <-  16 bits -> <- 32 bits ->
   -----------------------------
   | Block Size  | Next Block  |
   -----------------------------

   Block Size - Indicates the number of DD's in the DD Block,
                16 bit unsigned integer value
   Next Block - Gives the offset of the next DD Block. The last DD Block has
								a ZERO(0) in the "Next Block" field in the DDH.
                32 bit unsigned integer value

   Data Descriptor Block(DDB - variable size)
   ==========================================
	 <- DD Header -> <--------------- DD's --------------------->
   --------------------------------------------------------...-
   |Block | Next  | DD | DD | DD | DD | DD | DD | DD | DD |...|
   |Size  | Block |    |    |    |    |    |    |    |    |   |
   --------------------------------------------------------...-
	 <-------------------------- DD Block ---------------------->

	 Note: default number of DD's in a DD Block is 16

   HDF file layout
   =============================
   (one example)
   ---------------------------------------------------------------------
   | FH | DD Block | Data | DD Block | Data | DD Block | Data | .....
   ---------------------------------------------------------------------
 
*/

typedef unsigned short uint16;

#pragma pack(2)

/* data decsriptor header - preceed any dd's (this have to be big endien values!) */
typedef struct
{
	uint16	count;        /* number of tags to follow */
	long		next;	       /* offset of the next dh */
} DD_HEADER;


#pragma pack(2)

/* record of each data decsriptor (this have to be big endien values!) */
typedef struct
{
	uint16	tag;          /* Tag number of element i.e. type of data */
	uint16	ref;          /* Reference number of element */
	long		offset;       /* byte offset of data element from */
	long		len;					/* length of data element */
} DD_TAG;

#define ReadLong(i)	(*((LPLONG)(i)))
#define ReadWord(i)	(*((LPUWORD)(i)))

//////////////////////////////////////////////////////////////////////////////
// HDFI.H

/*--------------------------------------------------------------------------*/
/*                              MT/NT constants                             */
/*  four MT nibbles represent double, float, int, uchar (from most          */
/*  significant to least significant).                                      */
/*  The values for each nibble are:                                         */
/*      1 - Big Endian                                                      */
/*      2 - VAX                                                             */
/*      3 - Cray                                                            */
/*      4 - Little Endian                                                   */
/*      5 - Convex                                                          */
/*      6 - Fujitsu VP                                                      */
/*--------------------------------------------------------------------------*/
#define     DFMT_SUN            0x1111
#define     DFMT_ALLIANT        0x1111
#define     DFMT_IRIX           0x1111
#define     DFMT_APOLLO         0x1111
#define     DFMT_IBM6000        0x1111
#define     DFMT_HP9000         0x1111
#define     DFMT_CONVEXNATIVE   0x5511
#define     DFMT_CONVEX         0x1111
#define     DFMT_UNICOS         0x3331
#define     DFMT_CTSS           0x3331
#define     DFMT_VAX            0x2221
#define     DFMT_MIPSEL         0x4441
#define     DFMT_PC             0x4441
#define     DFMT_MAC            0x1111
#define     DFMT_SUN386         0x4441
#define     DFMT_NEXT           0x1111
#define     DFMT_MOTOROLA       0x1111
#define     DFMT_ALPHA          0x4441
#define     DFMT_VP             0x6611
#define     DFMT_I860           0x4441
#define     DFMT_CRAYMPP        0x1171


