#include <windows.h>

// loads an image and create the connected window
LPBMPDATA OpenCreateWindow( LPCSTR datei );

// �ndert den Status der Men�eintr�ge von UNDO/REDO
void	EnableUndoRedo(BOOL Undo, BOOL Redo);

// String aus Resource
LPSTR	GetStringRsc( UINT h );

// H�ngt neue Datei an die letzten vier Dateien ...
void	UpdateRecent( LPSTR datei, LPBMPDATA pBmp );

// Statuszeile leeren
void	ClearStatusLine( void );

// str in der Statuszeile ausgeben 
void	StatusLine( LPSTR str );

// Schreibt neue Informationen aus der Stringressource h in die Statusleiste ...
void	StatusLineRsc( UINT h );

// Fehlermeldung aus String
void	Fehler( LPSTR str );

// Fehlermeldung aus der Stringressource h in die Statusleiste ...
void	FehlerRsc( UINT h );

// Warnung aus String
void	Warnung( LPSTR str );

// Warnung aus der Stringressource h in die Statusleiste ...
void	WarnungRsc( UINT h );

// Mauszeiger variieren
void	WarteMaus( void );
void	NormalMaus( void );
// 30.10.98
