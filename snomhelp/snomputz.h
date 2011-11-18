
#ifndef IDHELP
#define IDHELP 9
#endif

#ifndef VK_BACK
#define VK_BACK   0x08
#endif

// Allgemeines
#define IDD_ZAHL	101
#define HILFETEXT	7799
#define INFO_TEXT	100
#define TOOLBARBMP 100

#define OVERRUN_OVERFLOW	102
#define OVERRUN_UNDERFLOW	103

// Stringtable
#define W_OVERFLOW	2
#define E_MEMORY	3
#define W_NIX	4
#define I_ZEILENMITTEL	5
#define I_DESPIKE	6
#define I_GROSS	7
#define I_FREE_MEMORY	8
#define I_REDRAW	9
#define I_IMPORT	10
#define I_SAVE_WND	11
#define I_EXPORT	12
#define I_PRINTING	13
#define I_UNDO	14
#define W_STATISTIK	16
#define I_Y_DIFF	17
#define I_GLAETTUNG	18
#define I_INVERT	19
#define I_TURN	20
#define I_WINKEL	21
#define I_KONTUR	22
#define I_COPY	23
#define E_NO_BITMAP	24
#define I_ASCII	26
#define I_DI	27
#define I_HDF	28
#define I_BMP	29
#define E_UNKNOWN_FILE	30
#define W_COMMENT	31
#define I_DIFFERENTIAL	41
#define E_DSP_DLL	39
#define I_SAVE_FILE	40
#define E_NOT_FITTED	33
#define DO_NO_OVERFLOW	34
#define E_FILE_CORRUPT	35
#define E_TOO_SMALL	36
#define E_HARDDISK	38
#define I_NULL	37
#define E_FILE	25
#define I_X_DIFF	15

#define GROSS_W	102
#define GROSS_H	103
#define GROSS_X_Y	104
#define GROSS_FAKTOR_OK	105
#define GROSS_FAKTOR	106

#define HUB_LUMI	119
#define HUB_Y_OFF	121
#define HUB_X_OFF	120

#define SPIKE_X	101
#define SPIKE_Y	102
#define SPIKE_LOW_OK	103
#define SPIKE_UP_OK	105
#define SPIKE_UP	106
#define SPIKE_INTERPOL	107
#define SPIKE_WEITE	108
#define SPIKE_LUMI	112
#define SPIKE_ERROR	111
#define SPIKE_TOPO	110
#define SPIKE_LOW	104

#define MEDIAN_7ZEILEN	115
#define MEDIAN_5X5	118
#define MEDIAN_3X3	117
#define MEDIAN_2X2	116
#define MEDIAN_5ZEILEN	114
#define MEDIAN_3ZEILEN	113

#define ALPHA_2D	103
#define ALPHA_WINKEL	105
#define ALPHA_ENDE	102
#define ALPHA_START	101

#define CAPTURE_BITMAP	23

// Zeilenmitteldialog
#define MITTEL_0	101
#define MITTEL_1	102
#define MITTEL_2	103
#define MITTEL_FIT	104
#define MITTEL_N	105

#define MESS_OSZI	555
#define MESS_ZURUECK	198
#define MESS_VOR	199

#define MESS_PID_FREQ	100
#define MESS_PID_PROP	101
#define MESS_PID_INT	102
#define MESS_PID_DIFF	103
#define MESS_PID_SHOW_SOLL	104
#define MESS_PID_SOLL	105
#define MESS_PID_ABORT	106
#define MESS_PID_SEND	107

#define MESS_SCAN_W	108
#define MESS_SCAN_XPTS	109
#define MESS_SCAN_FREQ	110
#define MESS_SCAN_VERSATZ    111
#define MESS_SCAN_SEND	112

#define MESS_CAPTURE	103

#define MESS_HILBERT	114
#define MESS_X_SCROLL	115
#define MESS_Y_SCROLL	106
#define MESS_RECHTSUNTEN	111
#define MESS_LINKSUNTEN	110
#define MESS_RECHTSOBEN	105
#define MESS_LINKSOBEN	109
#define MESS_LINESCAN	199
#define MESS_XPOS	107
#define MESS_YPOS	104

#define MESS_PORT	101
#define MESS_TERMINAL	102

#define MESS_FEIN_SETPOINT	102
#define MESS_GROB_SCROLL	106
#define MESS_GROB_POS	107
#define MESS_GROB_SETZEN	108
#define MESS_FEIN_START	101
#define MESS_FEIN_STOP	103

#define WINKEL_LOGARITHMUS	102
#define GLATT_PUNKTE	101

// Rohdatendialog ...
#define ROH_OFFSET	101
#define ROH_WEITE	102
#define ROH_HOEHE	103
#define ROH_8BIT	108
#define ROH_10BIT	110
#define ROH_12BIT	112
#define ROH_16BIT	116
#define ROH_INTEL	104

// Farbendialog
#define FARB_WECHSELE	7979
#define FARB_PREVIEW	118
#define FARB_SPEZ_Z	122
#define FARB_WEITE_ZAHL	125
#define FARB_WEITE_EINHEIT	126
#define FARB_START_EINHEIT	124
#define FARB_START_ZAHL	123
#define FARB_WEITE	109
#define FARB_Z_UNIT	121
#define FARB_Z_ACHSE	119
#define FARB_MOD_EINHEIT	113
#define FARB_KONT_EINHEIT	120
#define FARB_TITEL	117
#define FARB_MOD	115
#define FARB_MOD_DIST	116
#define FARB_KONT_TOL	114
#define FARB_KONT_DIST	111
#define FARB_3D	112
#define FARB_3FARBEN	101
#define FARB_3	104
#define FARB_2	103
#define FARB_1	102
#define FARB_START	108
#define FARB_KONT	110
#define FARB_SHOW3	107
#define FARB_SHOW2	106
#define FARB_SHOW1	105

#define HUB_X	101
#define HUB_Y	102
#define HUB_Z	103
#define HUB_AA	104
#define HUB_NM	105
#define HUB_MKM	106
#define HUB_LADEN	118
#define HUB_SHOW_OFF	110
#define HUB_KEINE	107
#define HUB_Z_UNIT	112
#define HUB_LUMI_UNIT	114
#define HUB_KOMM	109
#define HUB_INFO	108

#define MITTEL3D_XRASTER	104
#define MITTEL3D_YRASTER	105
#define MITTEL3D_KONST	106
#define MITTEL3D_LIN	107
#define MITTEL3D_QUAD	108
#define MITTEL3D_HORIZONTAL	109
#define MITTEL3D_VERTIKAL	110


#define ANSICHT_LINKS	108
#define ANSICHT_RECHTS	109
#define ANSICHT_DATEN	110
#define ANSICHT_MARK_RECHTS	113
#define ANSICHT_MARK_LINKS	212
#define ANSICHT_SHOW4	112
#define ANSICHT_HINTEN	111
#define ANSICHT_SHOW3	107
#define ANSICHT_SHOW2	106
#define ANSICHT_SHOW1	105
#define ANSICHT_VORNE	206
#define ANSICHT_PREVIEW	205
#define ANSICHT_ZW	203
#define ANSICHT_ZWINKEL	202
#define ANSICHT_XYWINKEL	201
#define ANSICHT_HOEHE_EDIT	210
#define ANSICHT_Z_EDIT	209
#define ANSICHT_XY_EDIT	208
#define ANSICHT_DRAFT	204
#define ANSICHT_ZEICHENSATZ	211


#define MATHE_FEST	219
#define MATHE_WERT	220
#define MATHE_QUELLE	221
#define MATHE_ERROR	772
#define MATHE_LUMI	774
#define MATHE_TOPO	771
#define MATHE_OVERFLOW	222

#define MATHE_HOCH 39
#define MATHE_AND	38
#define MATHE_XOR	94
#define MATHE_OR	124
#define MATHE_PLUS	43
#define MATHE_MINUS	45
#define MATHE_MAL	42
#define MATHE_GETEILT	47
#define MATHE_MODULO 37
#define MATHE_LOG 108


#define IDM_MESS	60
#define IDM_OPEN			61
#define IDM_IMPORT_LUMI	62
#define IDM_IMPORT_TOPO	63
#define IDM_SAVE				64
#define IDM_EXPORT_ANSICHT 65
#define IDM_EXPORT_LUMI	66
#define IDM_EXPORT_TOPO	67
#define IDM_EXPORT_DI	1
#define IDM_CLOSE 			68
#define IDM_DRUCKEN			69
#define IDM_DRUCKEREINSTELLEN	70
#define IDM_SEITENLAYOUT	71
#define IDM_RECENT1			72
#define IDM_RECENT2			73
#define IDM_RECENT3			74
#define IDM_RECENT4			75
#define IDM_EXIT				76

#define IDM_UNDO	30
#define IDM_REDO	31
#define IDM_COPY	32
#define IDM_PASTE	33
#define IDM_NUR_TOPO	34
#define IDM_NUR_ERROR	(IDM_NUR_TOPO+1)
#define IDM_NUR_LUMI	(IDM_NUR_TOPO+3)


#define IDM_DREHEN		40
#define IDM_ZEILENMITTEL	41
#define IDM_NULL	42
#define IDM_DIFF_NULL	43
#define IDM_3DMITTEL		45
#define IDM_GLEITEND_MITTEL	46
#define IDM_FFT_MITTEL	47
#define IDM_DESPIKE	  	48
#define IDM_MEDIAN	    49
#define IDM_GROESSE			50
#define IDM_NEGATIV			51
#define IDM_MATHE	    	52
#define IDM_FFT					53
#define IDM_DIFFERENTIAL	54

#define IDM_INVERT_MASKE  80
#define IDM_COPY_MASKE		81
#define IDM_LOESCHE_MASKE	82
#define IDM_MASKE_VERTIKAL_MITTELN		83
#define IDM_MASKE_HORIZONTAL_MITTELN	84

#define IDM_RMS					20
#define IDM_HISTOGRAMM	21
#define IDM_ALPHA				22
#define IDM_ORIENTIERUNG 23
#define IDM_KONTOUR			24
#define IDM_SCANLINE		25

#define IDM_FALSCHFARBEN	90
#define IDM_3DANSICHT			91
#define IDM_MASZE					92
#define IDM_REDRAW				93
#define IDM_FIT_TO_WINDOW	94
#define IDM_1ZU1	IDM_FIT_TO_WINDOW+1
#define IDM_1ZU2	IDM_FIT_TO_WINDOW+2
#define IDM_1ZU3	IDM_FIT_TO_WINDOW+3
#define IDM_1ZU4	IDM_FIT_TO_WINDOW+4
#define	IDM_SHOW				100
#define IDM_SHOW_TOPO		IDM_SHOW+1
#define IDM_SHOW_ERROR	IDM_SHOW+2
#define IDM_SHOW_LUMI		IDM_SHOW+4

#define IDM_TILE     	110
#define IDM_CASCADE   111
#define IDM_CLOSEALL  112
#define IDM_UMBENNEN  113
#define IDM_ARRANGE   114

#define IDM_ABOUT       120
#define IDM_HILFEINHALT	121
#define IDM_HILFESUCHEN	122
#define IDM_HILFETUTOR	123

