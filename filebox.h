/* Common-File-Dialog (auf Deutsch!) */

extern UINT	WM_FSEL_HELP;

// Hängt neue Extension ran
void	ChangeExt( LPSTR strStr, LPSTR strExt );

// Aus HelpEx: Pfad zur Hilfedatei ermitteln
void MakeHelpFileName(HINSTANCE hInst, LPSTR strFileName, LPSTR strModulname);
// 5.4.97

/* Datei laden Auswahldialog */
DWORD CMUFileOpen( HWND hWnd, LPCSTR	strTitel, LPSTR strName, LPCSTR strTemp );
// Fileselctor for multiple files (s.a. for single files)
DWORD CMUFileOpenMultiImg( HWND hWnd, LPCSTR strTitel, LPSTR strName, LPCSTR strTemp );
/* Datei speichern Auswahldialog */
DWORD CMUFileSave( HWND hWnd, LPCSTR strTitel, LPSTR strName, LPCSTR strTemp, LPSTR wFilterIndex );

/* Get Directory Name */
BOOL CMUGetFolderName( const HWND hwndOwner, LPCSTR lpszTitle, LPSTR strSelFolder );
