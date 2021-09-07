#ifdef __main

#if defined(_WIN32)  ||  defined(__WIN32S__)
//#define STRCONST __declspec( dllexport ) const char *
#define STRCONST const char *
#else
//#define STRCONST __declspec( dllexport ) const char far *
#define STRCONST const char far *
#endif

// Name of Images (always needed ...)
STRCONST	STR_TOPO =						"Topography";
STRCONST	STR_ERROR =						"Error Signal";
STRCONST	STR_LUMI =						"Luminescence";
STRCONST	STR_UNBENANNT =				"Untitled_%i";

// Aus SNOM-BMP
STRCONST	STR_LOAD_IMPORT =			"Import File";
STRCONST	STR_SAVE_FILE =				"Save File as";
STRCONST	STR_EXPORT_VIEW =			"Export Window as";
STRCONST	STR_EXPORT_DATA =			"Save Data as BMP";
STRCONST	STR_SAVE_PROFIL =			"Save Profile";
STRCONST	STR_SAVE_HIST =				"Save Histogram";
STRCONST	STR_FILE_SAVE_NAMES =	"HDF compressed (*.hdz)\0*.hd?\0HDF (PSI-compatible)\0*.hdf\0";
STRCONST	STR_FILE_BMP =				"Bitmap (*.bmp,*.dib)\0*.bmp;*.dib\0";
STRCONST	STR_FILE_EXPORT =			"Bitmap (*.bmp,*.dib)\0*.bmp;*.dib\0Metafile (*.emf)\0*.emf\0";
STRCONST	STR_FILE_ASCII =			"SNOM-Putz Data Files (*.prf,*.hst,*.kor)\0*.prf;*.hst;*.kor\0ASCII-Files (*.dat)\0*.dat\0All Files\0*.*\0";
STRCONST	STR_FILE_LUT	 =			"Color LUT (*.lut)\0*.lut\0All files\0*.*\0";
STRCONST	STR_FOLDER =					"Select folder for file overview";
STRCONST	STR_NO_FILES =				"No SxM-Files in this folder.";
STRCONST	STR_FOUND_FILES =			"About %i SxM file(s) in this folder.";

// Hilfestrings für die Dateiauswahl (MUSS mit den Strings in der HLP-Datei übereinstimmen!)
STRCONST	STR_HFILE_OPEN = "Datei öffnen";
STRCONST	STR_HFILE_SAVE = "Datei speichern als";
STRCONST	STR_HFILE_EXPORT = "Bild exportieren";
STRCONST	STR_HFILE_PROFIL = "Höhenprofil";
STRCONST	STR_HFILE_HIST = "Histogramm";

// Aus Dsp-mes.c
STRCONST	STR_MITTEL_ZEIT = "Avarage height %d; Remaining time (min:ses) %d:%02d";

// Aus SNOMPUTZ.C
STRCONST	STR_HDF_SNOM =				"HDF for SNOM-Putz";
STRCONST	STR_OPEN_FILE =				"Open File";
STRCONST	STR_FILE_AFM =				"All Files\0*.*\0HDF-Files (*.hd?)\0*.hd?\0Omicron (*.par)\0*.par\0Digital-AFM (*.0??)\0*.0??\0Hitachi-AFM (*.afm)\0*.afm\0WSxM-files (*.stp)\0*.stp\0";
STRCONST	STR_SNOM_MESS =				"Snomputz: Measurements";
STRCONST	STR_SNOM_PROG =				"Snomputz: Programming";
STRCONST	STR_TUTOR =						"The SNOM-Putz Tutor will help you to prepare an image\nPlease choose a File!";
STRCONST	STR_HFILE_TUTOR =			"SNOM-Putz Tutor";
STRCONST	STR_T_NO_OPEN =				"The File was not loaded, the Tutor will finish.";
STRCONST	STR_T_ERROR =					"The File contains an error image. Would you like to exclude it from the screen?";
STRCONST	STR_T_TURN =					"Should the image be rotated by 90°?\n(Are the Scanline vertical?)";

// A hier aus SNOM-DLG.C
STRCONST	STR_FILE_FRAC =		"Save Fractal Exponent Analysis";
// Hilfestrings für Dialoge
STRCONST	STR_HELP_FARBEN = "Farbzuweisung";
STRCONST	STR_HELP_VIEW =		"Darstellung ändern";
STRCONST	STR_HELP_MATH =		"Bildermathematik";
STRCONST	STR_HELP_MASZE =	"Maße festlegen";
STRCONST	STR_HELP_LINES =	"Zeilenmittelung";
STRCONST	STR_HELP_PLANES =	"3D-Mittelung";
STRCONST	STR_HELP_FRAC =		"Rauhigkeitsexponenten";
STRCONST	STR_HELP_SPIKE =	"Ausreißer entfernen";
STRCONST	STR_HELP_FFT =		"FFT-Filterung";

// Buttonbeschriftungen
STRCONST	STR_F_UNIT =		"f / Hz";
STRCONST	STR_TOPO_UNIT =		"h / nm";
STRCONST	STR_TOPO_SUNIT =	"h / µm";
STRCONST	STR_X_UNIT =			"x / nm";
STRCONST	STR_X_SUNIT =			"x / \xB5m";
STRCONST	STR_Y_UNIT =			"y / nm";
STRCONST	STR_Y_SUNIT =			"y / \xB5m";
STRCONST	STR_LUMI_UNIT =		"I / mW";
STRCONST	STR_PROFIL =			"Profile";
STRCONST	STR_AUTOKORR =		"Autocorrelation";
STRCONST	STR_PSD =					"ln(PSD)";
STRCONST	STR_NIX =					"[nothing]";

#else

#if defined(_WIN32)  ||  defined(__WIN32S__)
#define STRCONST extern const char *
#else
#define STRCONST extern const char far *
#endif

// Immer gebraucht (Namen der Bilder)
STRCONST	STR_TOPO;
STRCONST	STR_ERROR;
STRCONST	STR_LUMI;
STRCONST	STR_UNBENANNT;

// Aus SNOM-BMP.C
STRCONST STR_LOAD_IMPORT;
STRCONST STR_SAVE_FILE;
STRCONST	STR_EXPORT_VIEW;
STRCONST	STR_EXPORT_DATA;
STRCONST	STR_SAVE_PROFIL;
STRCONST	STR_SAVE_HIST;
STRCONST	STR_FILE_SAVE_NAMES;
STRCONST	STR_FILE_BMP;
STRCONST	STR_FILE_EXPORT;
STRCONST	STR_FILE_ASCII;
STRCONST	STR_FILE_LUT;
STRCONST	STR_FOLDER;
STRCONST	STR_NO_FILES;
STRCONST	STR_FOUND_FILES;

// Hilfestrings für die Dateiauswahl (MUSS mit den Strings in der HLP-Datei übereinstimmen!)
STRCONST	STR_HFILE_OPEN;
STRCONST	STR_HFILE_SAVE;
STRCONST	STR_HFILE_EXPORT;
STRCONST	STR_HFILE_PROFIL;
STRCONST	STR_HFILE_HIST;

// Aus Dsp-mes.c
STRCONST	STR_MITTEL_ZEIT;

// Aus SNOMPUTZ.C
STRCONST	STR_HDF_SNOM;
STRCONST	STR_OPEN_FILE;
STRCONST	STR_FILE_AFM;
STRCONST	STR_SNOM_MESS;
STRCONST	STR_SNOM_PROG;
STRCONST	STR_TUTOR;
STRCONST	STR_HFILE_TUTOR;
STRCONST	STR_T_NO_OPEN;
STRCONST	STR_T_ERROR;
STRCONST	STR_T_TURN;

// A hier aus SNOM-DLG.C
STRCONST	STR_FILE_FRAC;
// Hilfestrings für Dialoge
STRCONST	STR_HELP_FARBEN;
STRCONST	STR_HELP_VIEW;
STRCONST	STR_HELP_MATH;
STRCONST	STR_HELP_MASZE;
STRCONST	STR_HELP_LINES;
STRCONST	STR_HELP_PLANES;
STRCONST	STR_HELP_FRAC;
STRCONST	STR_HELP_SPIKE;
STRCONST	STR_HELP_FFT;

// Einheiten
STRCONST	STR_F_UNIT;
STRCONST	STR_TOPO_UNIT;
STRCONST	STR_TOPO_SUNIT;
STRCONST	STR_X_UNIT;
STRCONST	STR_X_SUNIT;
STRCONST	STR_Y_UNIT;
STRCONST	STR_Y_SUNIT;
STRCONST	STR_LUMI_UNIT;

// Buttonbeschriftungen
STRCONST	STR_PROFIL;
STRCONST	STR_AUTOKORR;
STRCONST	STR_PSD;
STRCONST	STR_NIX;
#endif
