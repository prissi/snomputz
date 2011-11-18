


// Oszi-"Control"
LONG WINAPI OsziWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// PID-Dialog
BOOL WINAPI MessPID(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


// Seriell-Dialogroutine
LRESULT CALLBACK MessSeriellWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Routine, die die einzelnen Unterroutinen aufruft
LRESULT CALLBACK MessWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

