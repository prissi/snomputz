#ifdef __main

#if defined(_WIN32)  ||  defined(__WIN32S__)
//#define STRCONST __declspec( dllexport ) const char *
#define STRCONST const char *
#else
//#define STRCONST __declspec( dllexport ) const char far *
#define STRCONST const char far *
#endif


// Immer gebraucht (Namen der Bilder)
STRCONST	STR_TOPO =						"Topografie";
STRCONST	STR_ERROR =						"Fehler";
STRCONST	STR_LUMI =						"Lumineszenz";
STRCONST	STR_UNBENANNT =				"Unbenannt_%i";

// Aus SNOM-BMP
STRCONST	STR_LOAD_IMPORT =			"Datei Importieren";
STRCONST	STR_SAVE_FILE =				"Datei speichern als";
STRCONST	STR_EXPORT_VIEW =			"Ansicht exportieren";
STRCONST	STR_EXPORT_DATA =			"Daten als Bitmap speichern";
STRCONST	STR_SAVE_PROFIL =			"Profilverlauf speichern";
STRCONST	STR_SAVE_HIST =				"Histogramm speichern";
STRCONST	STR_FILE_SAVE_NAMES =	"HDF komprimiert (*.hdz)\0*.hd?\0PSI-kompatibel (*.hdf)\0*.hdf\0Digital (*.0xy)\0*.0??\0RHK-kompatibel (*.sm2)\0*.sm2\0";
STRCONST	STR_FILE_BMP =				"Bitmap (*.bmp,*.dib)\0*.bmp;*.dib\0";
STRCONST	STR_FILE_EXPORT =			"Bitmap (*.bmp,*.dib)\0*.bmp;*.dib\0Metafile (*.emf)\0*.emf\0";
STRCONST	STR_FILE_ASCII =			"SNOM-Putz Datei (*.prf,*.hst,*.kor)\0*.prf;*.hst;*.kor\0ASCII-Files (*.dat)\0*.dat\0All Files\0*.*\0";

// Hilfestrings f�r die Dateiauswahl (MUSS mit den Strings in der HLP-Datei �bereinstimmen!)
STRCONST	STR_HFILE_OPEN = "Datei �ffnen";
STRCONST	STR_HFILE_SAVE = "Datei speichern als";
STRCONST	STR_HFILE_EXPORT = "Bild exportieren";
STRCONST	STR_HFILE_PROFIL = "H�henprofil";
STRCONST	STR_HFILE_HIST = "Histogramm";

// Aus Dsp-mes.c
STRCONST	STR_MITTEL_ZEIT = "Mittelwert %d, Verbleibende Zeit (min:ses) %d:%02d";

// Aus SNOMPUTZ.C
STRCONST	STR_HDF_SNOM =				"HDF f�r SNOM-Putz";
STRCONST	STR_OPEN_FILE =				"Datei laden";
STRCONST	STR_FILE_AFM =				"Alle Dateien\0*.*\0HDF-Dateien (*.hd?)\0*.hd?\0Omicron (*.par)\0*.par\0Digital (*.0xy)\0*.0??\0RHK (*.sm2)\0*.sm2\0ECS (*.img)\0*.img\0";
STRCONST	STR_SNOM_MESS =				"Snomputz: Messen";
STRCONST	STR_SNOM_PROG =				"Snomputz: Programmieren";
STRCONST	STR_TUTOR =						"Der SNOM-Putz Tutor wird helfen, ein Bild zu bearbeiten.\nBitte eine Datei laden.";
STRCONST	STR_HFILE_TUTOR =			"SNOM-Putz Tutor";
STRCONST	STR_T_NO_OPEN =				"Datei konnten nicht geladen werden, \nder Tutor wird beendet.";
STRCONST	STR_T_ERROR =					"Soll das Fehlerbild ausgeblendet werden?";
STRCONST	STR_T_TURN =					"Soll das Bild rotiert werden? (Rasterzeilen sind senkrecht)";

// A hier aus SNOM-DLG.C
STRCONST	STR_FILE_FRAC =		"Fraktale Dimensionsdaten speichern";
// Hilfestrings f�r Dialoge
STRCONST	STR_HELP_FARBEN = "Farbzuweisung";
STRCONST	STR_HELP_VIEW =		"Darstellung �ndern";
STRCONST	STR_HELP_MATH =		"Bildermathematik";
STRCONST	STR_HELP_MASZE =	"Ma�e festlegen";
STRCONST	STR_HELP_LINES =	"Zeilenmittelung";
STRCONST	STR_HELP_PLANES =	"3D-Mittelung";
STRCONST	STR_HELP_FRAC =		"Rauhigkeitsexponenten";
STRCONST	STR_HELP_SPIKE =	"Ausrei�er entfernen";
STRCONST	STR_HELP_FFT =		"FFT-Filterung";

// Buttonbeschriftungen
STRCONST	STR_F_UNIT =		"f / Hz";
STRCONST	STR_TOPO_UNIT =	"h / nm";
STRCONST	STR_TOPO_SUNIT =	"h / �m";
STRCONST	STR_X_UNIT =			"x / nm";
STRCONST	STR_X_SUNIT =			"x / �m";
STRCONST	STR_Y_UNIT =			"y / nm";
STRCONST	STR_Y_SUNIT =			"y / �m";
STRCONST	STR_LUMI_UNIT =		"I / mW";
STRCONST	STR_PROFIL =			"Profilverlauf";
STRCONST	STR_AUTOKORR =		"Autokorrelation";
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

// Hilfestrings f�r die Dateiauswahl (MUSS mit den Strings in der HLP-Datei �bereinstimmen!)
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
// Hilfestrings f�r Dialoge
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
