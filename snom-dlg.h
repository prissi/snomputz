
// Gibt Rechenzeit an andere ab
VOID	YieldApp( BOOL Wait );

/* Dialog-Callback aus SNOM-DLG.C */
BOOL WINAPI InfoDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI StdDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI StringDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OverrunDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI HandleBrowseDialog(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI RohImportDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI FarbenDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI KonturDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI Ansicht3DDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI MatheDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI Mittel3DDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI MittelDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI UnitDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI FractalDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI SpikeDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI MedianDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI WinkelDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI GrossDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI TutorDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI ProfilDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI FFTDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI VolumeDialog (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);



// helper functions

// Sucht Namen aller Bitmaps ... (z.B. für die Mathematik)
LPBMPDATA MakeHwndName( HWND hwnd );

//**** Callback für das Neuzeichnen der Fenster
BOOL CALLBACK EnumSetComboNames( HWND hwnd, LPARAM lparm );
