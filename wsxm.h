/***********************************************************************
*
*	File Name: Header.h
*
* Defines for WxSM-Header
*
***********************************************************************/

#define MAX_CHARS_IN_HEADER_LINE	1000

#define	WSxM_MAGIC	0x4D785357
// or in ASCII 'WSxM'

#define IMAGE_HEADER_VERSION        "1.0 (April 2000)"

#define IMAGE_HEADER_SIZE_TEXT      "Image header size: "
#define	IMAGE_HEADER_END_TEXT		"Header end"
#define TEXT_COPYRIGHT_NANOTEC      "WSxM file copyright Nanotec Electronica\n"
#define STM_IMAGE_FILE_ID           "SxM Image file\n"

#define IMAGE_HEADER_GENERAL_INFO               "General Info"
#define	IMAGE_HEADER_GENERAL_INFO_ACQUISITION		"Acquisition channel"
#define	IMAGE_DOUBLE_DATA												"Image Data Type: double"
#define IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS		"Number of columns"
#define IMAGE_HEADER_GENERAL_INFO_NUM_ROWS			"Number of rows"
#define IMAGE_HEADER_GENERAL_INFO_Z_AMPLITUDE		"Z Amplitude"

#define IMAGE_HEADER_MISC_INFO			     "Miscellaneous"
#define IMAGE_HEADER_MISC_INFO_MAXIMUM	     "Maximum"
#define IMAGE_HEADER_MISC_INFO_MINIMUM	     "Minimum"
#define IMAGE_HEADER_MISC_INFO_COMMENTS	     "Comments"
#define IMAGE_HEADER_MISC_INFO_VERSION	     "Version"

#define IMAGE_HEADER_CONTROL                "Control"
#define IMAGE_HEADER_CONTROL_X_AMPLITUDE	"X Amplitude"
#define IMAGE_HEADER_CONTROL_Y_AMPLITUDE	"Y Amplitude"

#define IMAGE_HEADER_HEADS	"Head Settings"

#define IMAGE_HEADER_HEADS_X_CALIBRATION	"X Calibration"
#define IMAGE_HEADER_HEADS_Z_CALIBRATION	"Z Calibration"


/* Most simple sample Header:

WSxM file copyright Nanotec Electronica
SxM Image file
Image header size: 417

[Control]

   Signal Gain: 1
  X Amplitude: 100 nm
  Y Amplitude: 100 nm
  Z Gain: 1

[General Info]

   Head type: STM
  Number of columns: 512
  Number of rows: 512
  Z Amplitude: 100 nm

[Head Settings]

   Preamp Gain: 1000 mV/nA
  X Calibration: 1 Ĺ/V
  Z Calibration: 1 Ĺ/V

[Header end]

	Height Units might be well Ĺ or µm

*/

/***********************************************************************
*
*	HEADER structure
*
*	This is the structure we will use to represent a header of a WSxM
*	It will have three strings representing each value in the header
*
*	- The title will indicate the group of values this value is included
*	in the header
*
*	- The label will precisate what is the value for
*
*	- The value will be an ASCII representation of the value
*
*	In the structure we can find too the total number of fields in the
*	structure
*
***********************************************************************/

