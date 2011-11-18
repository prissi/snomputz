
#include <assert.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include "filebox.h"
#include "snomputz.h"
#include "snomlang.h"

extern HINSTANCE hInst;


// Hängt neue Extension ran
void	ChangeExt( LPSTR strDatei, LPSTR strExt )
{
	int	iLen;

assert( strDatei!=NULL  &&  strExt!=NULL  );

	iLen = lstrlen(strDatei);
	while(  --iLen>0  )
	{
		if(  strDatei[iLen]=='\\'  )
			break;
		if(  strDatei[iLen]=='.'  )
			strDatei[iLen] = 0;
	}

assert(  iLen>=0  );

	// Gab es etwas zu ersetzen?
	if(  iLen>=0  &&  strDatei[iLen]==0  )
		lstrcat( strDatei, strExt );
}
// 20.5.98




// Aus HelpEx: Pfad zur Hilfedatei ermitteln
void MakeHelpFileName(HINSTANCE hInst, LPSTR strFileName, LPSTR strModulname)
{
	LPSTR		pcFileName;
	BYTE		strFile[256];
	int     iFileNameLen;

	iFileNameLen = GetModuleFileName(hInst,strFile,256);
	if (iFileNameLen==0)
	{
		// war nicht erfolgreich
		wsprintf(strFileName,"%s.hlp", (LPSTR)strModulname);
		return;
	}
	pcFileName = strFile + iFileNameLen-1;
	while (iFileNameLen-->0) {
		if (*pcFileName-- == '.')
			break;
	}
	pcFileName[2] = 0;
	wsprintf(strFileName,"%shlp", (LPSTR)strFile, (LPSTR)strModulname);
	return;
}
// 5.4.97



#include <windows.h>


/*********************************************************************
Using the OPENFILENAME structure and the Windows 3.1 API call
GetOpenFileName() eases the selection of files for the programmer and for
the user.  The WINHELP.EXE help file WIN31WH.HLP (found in the BORLANDC\BIN
directory) contains a detailed description of the function call and its
associated structure.  The Flags field of the structure is particularly
useful when custimization is required.
**********************************************************************/

BYTE	strLastOpenPath[256];

/****************************************************************************/
// Fileselctor aus "cmdlg.ide" von Borland
DWORD CMUFileOpen( HWND hWnd, LPCSTR	strTitel, LPSTR strName, LPCSTR strTemp )
{
	BYTE	strTempName[256];
	OPENFILENAME ofnTemp;
	DWORD Errval; // Error value
	BOOL  Suc=FALSE;
	BYTE	buf[50];  // Error buffer
	LPSTR Errstr="GetOpenFileName Error $%lx";
	LPSTR	c;

/*  char strTemp[] = "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0";

Note the initialization method of the above string.  The GetOpenFileName()
function expects to find a string in the OPENFILENAME structure that has
a '\0' terminator between strings and an extra '\0' that terminates the
entire filter data set.  Using the technique shown below will fail because
"X" is really 'X' '\0' '\0' '\0' in memory.  When the GetOpenFileName()
function scans strTemp it will stop after finding the extra trailing '\0'
characters.

	char strTemp[][4] = { "X", "*.*", "ABC", "*.*", "" };

The string should be "X\0*.*\0ABC\0*.*\0".

Remember that in C or C++ a quoted string is automatically terminated with
a '\0' character so   char "X\0";   would result in 'X' '\0' '\0' which
provides the extra '\0' that GetOpenFileName() needs to see in order to
terminate the scan of the string.  Just 'char ch "X";' would result in 'X'
'\0' and GetOpenFileName() would wander off in memory until it lucked into
a '\0' '\0' character sequence.
*/

/*
Some Windows structures require the size of themselves in an effort to
provide backward compatibility with future versions of Windows.  If the
lStructSize member is not set the call to GetOpenFileName() will fail.
*/
	c = (LPSTR)strName + lstrlen(strName);
	while(  c>(LPSTR)strName  )
	{
		c--;
		if(  *c<=' '  )
			*c = '_';
	}

	c = (LPSTR)strName + lstrlen(strName);
	while(  *c!=':'  &&  *c!='\\'  &&  c>(LPSTR)strName  )
		c--;

	if(  *c=='\\'  )
		*c++ = 0;
	else
		c = strName;
	lstrcpy( strTempName, c );
	if(  c>strName  )
		lstrcpy( strLastOpenPath, strName );
	
	ofnTemp.lStructSize = sizeof( OPENFILENAME );
	ofnTemp.hwndOwner = hWnd; // An invalid hWnd causes non-modality
	ofnTemp.hInstance = 0;
	ofnTemp.lpstrFilter = (LPSTR)strTemp;  // See previous note concerning string
	ofnTemp.lpstrCustomFilter = NULL;
	ofnTemp.nMaxCustFilter = 0;
	ofnTemp.nFilterIndex = 1;
	ofnTemp.lpstrFile = (LPSTR)strTempName;  // Stores the result in this variable
	ofnTemp.nMaxFile = 256; // Soviel sollte es schon sein!
	ofnTemp.lpstrFileTitle = NULL;
	ofnTemp.nMaxFileTitle = 0;
	ofnTemp.lpstrInitialDir = strLastOpenPath;
	ofnTemp.lpstrTitle = strTitel;  // Title for dialog
	ofnTemp.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	if(  hWnd!=NULL  )
		ofnTemp.Flags |= OFN_SHOWHELP;
	ofnTemp.nFileOffset = 0;
	ofnTemp.nFileExtension = 0;
	ofnTemp.lpstrDefExt = "*";
	ofnTemp.lCustData = 0;
	ofnTemp.lpfnHook = 0;
	ofnTemp.lpTemplateName = NULL;
/*
If the call to GetOpenFileName() fails you can call CommDlgExtendedError()
to retrieve the type of error that occured.
*/
	if(  (Suc=GetOpenFileName( &ofnTemp )) != TRUE  )
	{
		Errval = CommDlgExtendedError();
		if(  Errval!=0  ) // 0 value means user selected Cancel
		{
			wsprintf(buf,Errstr,Errval);
			MessageBox(hWnd,buf,NULL,MB_OK|MB_ICONSTOP);
		}
	}
	else
	{
		lstrcpy( (LPSTR)strName, (LPSTR)strTempName );

		// Pfad extrahieren
		c = (LPSTR)strName + lstrlen(strName);
		while(  *c!=':'  &&  *c!='\\'  &&  c>(LPSTR)strName  )
			c--;
		if(  c>strName  &&  *c=='\\'  )
		{
			*c = 0;
			lstrcpy( strLastOpenPath, strName );
			*c = '\\';
		}
	}

	InvalidateRect( hWnd, NULL, TRUE ); // Repaint to display the new name
	if(  Suc  )
		return ofnTemp.nFilterIndex;	// Ausgewählte Extension
	return 0;
}
// 6.6.97

// Show browsing Window
BOOL WINAPI HandleBrowseDialog(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);


/* Adds and handles the "browse" button ... */
BOOL WINAPI AddBrowseCap(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static	LPOPENFILENAME pOfnTemp;
	static	HWND			mw;
	static	WPARAM		hSetFont;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			WORD	x, y;
//			RECT	rect={6,146,50,14};
			RECT	rect={212,92,71,14}; //old
			pOfnTemp = (LPOPENFILENAME)lParam;
			MapDialogRect(hdlg,&rect);
			x = rect.left;
			y = rect.top;
			GetClientRect( GetDlgItem(hdlg,2), &rect );
			mw = CreateWindow( "button", "Browse", WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, x, y, rect.right, rect.bottom, hdlg, NULL, NULL, NULL );
// old			mw = CreateWindow( "button", "Browse", WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, x, y, rect.right, rect.bottom, hdlg, NULL, NULL, NULL );
			SendMessage( mw, WM_SETFONT, hSetFont, 0 );
		}
		break;
		case WM_SETFONT:
			hSetFont = wParam;
		break;
		case WM_SHOWWINDOW:
			ShowWindow( mw, SW_SHOW );
		break;
		case WM_CLOSE:
			DestroyWindow( mw );
		return FALSE;
		case WM_COMMAND:
		{
			if(  wParam==BN_CLICKED  )
			{
				pOfnTemp->lCustData = GetCurrentDirectory( pOfnTemp->nMaxFile, pOfnTemp->lpstrFile );
				DestroyWindow( mw );
				EndDialog( hdlg, 2 );
				return TRUE;
			}
		}
	}
	return FALSE;
}
// 24.3.00


/****************************************************************************/
// Fileselctor for multiple files (s.a. for single files)
DWORD CMUFileOpenMultiImg( HWND hWnd, LPCSTR strTitel, LPSTR strName, LPCSTR strTemp )
{
	BYTE	strTempName[256];
	OPENFILENAME ofnTemp;
	DWORD Errval; // Error value
	BOOL  Suc=FALSE;
	BYTE	buf[500];  // Error buffer
	LPSTR Errstr="GetOpenFileName Error $%lx";
	LPSTR	c;
	WORD	i, j;

	c = (LPSTR)strName + lstrlen(strName);
	while(  c>(LPSTR)strName  )
	{
		c--;
		if(  *c<=' '  )
			*c = '_';
	}
	c = (LPSTR)strName + lstrlen(strName);
	while(  *c!=':'  &&  *c!='\\'  &&  c>(LPSTR)strName  )
		c--;

	if(  *c=='\\'  )
		*c++ = 0;
	else
		c = strName;
	lstrcpy( strTempName, c );
	if(  c>strName  )
		lstrcpy( strLastOpenPath, strName );
	
	ofnTemp.lStructSize = sizeof( OPENFILENAME );
	ofnTemp.hwndOwner = hWnd; // An invalid hWnd causes non-modality
	ofnTemp.hInstance = 0;
	ofnTemp.lpstrFilter = (LPSTR)strTemp;  // See previous note concerning string
	ofnTemp.lpstrCustomFilter = NULL;
	ofnTemp.nMaxCustFilter = 0;
	ofnTemp.nFilterIndex = 1;
	ofnTemp.lpstrFile = (LPSTR)strTempName;  // Stores the result in this variable
	ofnTemp.nMaxFile = 256; // Soviel sollte es schon sein!
	ofnTemp.lpstrFileTitle = NULL;
	ofnTemp.nMaxFileTitle = 0;
	ofnTemp.lpstrInitialDir = strLastOpenPath;
	ofnTemp.lpstrTitle = strTitel;  // Title for dialog
	ofnTemp.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;// |  OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK;
	if(  hWnd!=NULL  )
		ofnTemp.Flags |= OFN_SHOWHELP;
	ofnTemp.nFileOffset = 0;
	ofnTemp.nFileExtension = 0;
	ofnTemp.lpstrDefExt = "*";
	ofnTemp.lCustData = 0;
	ofnTemp.lpfnHook = AddBrowseCap;
	ofnTemp.lpTemplateName = NULL;
/*
If the call to GetOpenFileName() fails you can call CommDlgExtendedError()
to retrieve the type of error that occured.
*/
	if(  (Suc=GetOpenFileName( &ofnTemp )) != TRUE  )
	{
		if(  ofnTemp.lCustData>0  )
		{
			// Here for browsing ...
			LONG	params[2];
			params[0] = (LONG)hWnd;
			params[1] = (LONG)strTempName;
			CreateDialogParam( hInst, "OverviewDlg", hWnd, HandleBrowseDialog, (LPARAM)params  );
		}
		else
		{
			Errval = CommDlgExtendedError();
			if(  Errval!=0  ) // 0 value means user selected Cancel
			{
				wsprintf(buf,Errstr,Errval);
				MessageBox(hWnd,buf,NULL,MB_OK|MB_ICONSTOP);
			}
		}
	}
	else
	{
		i=0;
		while(strTempName[i]>0  &&  strTempName[i]!=32)
			i++;
		if(  strTempName[i]==32  )
		{
			strTempName[i++] = 0;
			// Directory update
			lstrcpy( strLastOpenPath, strTempName );
		}
		else
		{
			// This was the only name
			OpenCreateWindow( strTempName );
//			SendMessage( hWnd, WM_COMMAND, IDM_OPEN_FROM_STRING, (LPARAM)strTempName );
			// Directory update
			for(  i=lstrlen(strTempName);  i>0  &&  strTempName[--i]!='\\';  )
				;
			if(  i>0  )
			{
				strTempName[i] = 0;
				lstrcpy( strLastOpenPath, strTempName );
			}
		}

		// Beware: there may be more than one name!
		while(  strTempName[i]>0  )
		{
			j = i;
			while(strTempName[i]>0  &&  strTempName[i]!=32)
				i++;
			if(  strTempName[i]==32  )
				strTempName[i++] = 0;
			if(  i!=j  )
			{
				lstrcpy( buf, strTempName );
				lstrcat( buf, "\\" );
				lstrcat( buf, strTempName+j );
				SendMessage( hWnd, WM_COMMAND, IDM_OPEN_FROM_STRING, (LPARAM)buf );
			}
		}
	}

	InvalidateRect( hWnd, NULL, TRUE ); // Repaint to display the new name
	if(  Suc  )
		return ofnTemp.nFilterIndex;	// Ausgewählte Extension
	return 0;
}
// 6.6.97




BYTE	strLastSavePath[256];

/****************************************************************************/
// Fileselector fürs Speichern
/****************************************************************************/
DWORD CMUFileSave( HWND hWnd, LPCSTR strTitel, LPSTR strName, LPCSTR strTemp, LPSTR wFilterIndex )
{
	OPENFILENAME	ofnTemp;
	LPSTR					c;
	char					strTempName[256];
	char 					buf[50], *Errstr="GetSaveFileName Error #%ld";
	DWORD					Errval; // Error value
	BOOL  				Suc=FALSE;

/*
Some Windows structures require the size of themselves in an effort to
provide backward compatibility with future versions of Windows.  If the
lStructSize member is not set the call to GetOpenFileName() will fail.
*/
#if 1
	c = (LPSTR)strName + lstrlen(strName);
	while(  c>(LPSTR)strName  )
	{
		c--;
		if(  *c<=' '  )
			*c = '_';
	}

	c = (LPSTR)strName + lstrlen(strName);
	while(  *c!=':'  &&  *c!='\\'  &&  c>(LPSTR)strName  )
		c--;

	if(  *c=='\\'  )
		*c++ = 0;
	else
		c = strName;
	lstrcpy( strTempName, c );
	if(  c>(LPSTR)strName  )
		lstrcpy( strLastSavePath, strName );
#else
	lstrcpy( strTempName, strName );
#endif
	ofnTemp.lStructSize = sizeof( OPENFILENAME );
	ofnTemp.hwndOwner = hWnd; // An invalid hWnd causes non-modality
	ofnTemp.hInstance = 0;
	ofnTemp.lpstrFilter = (LPSTR)strTemp;  // See previous note concerning string
	ofnTemp.lpstrCustomFilter = NULL;
	ofnTemp.nMaxCustFilter = 0;
	ofnTemp.nFilterIndex = 1;
	if(  wFilterIndex!=NULL  )
		ofnTemp.nFilterIndex = *wFilterIndex;
	ofnTemp.lpstrFile = (LPSTR)strTempName;  // Stores the result in this variable
	ofnTemp.nMaxFile = 256; // Soviel sollte es schon sein!
	ofnTemp.lpstrFileTitle = NULL;
	ofnTemp.nMaxFileTitle = 0;
	ofnTemp.lpstrInitialDir = strLastSavePath;
	ofnTemp.lpstrTitle = strTitel;  // Title for dialog
	ofnTemp.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	if(  hWnd!=NULL  )
		ofnTemp.Flags |= OFN_SHOWHELP;
	ofnTemp.nFileOffset = 0;
	ofnTemp.nFileExtension = 0;
	ofnTemp.lpstrDefExt = "";
	ofnTemp.lCustData = 0;
	ofnTemp.lpfnHook = NULL;
	ofnTemp.lpTemplateName = NULL;
/*
If the call to GetOpenFileName() fails you can call CommDlgExtendedError()
to retrieve the type of error that occured.
*/
	if((Suc=GetSaveFileName( &ofnTemp )) != TRUE)
	{
		Errval = CommDlgExtendedError();
		if(  Errval!=0  ) // 0 value means user selected Cancel
		{
			wsprintf(buf,Errstr,Errval);
			MessageBox(hWnd,Errstr,NULL,MB_OK|MB_ICONSTOP);
		}
	}
	else
	{
		lstrcpy( (LPSTR)strName, (LPSTR)strTempName );

		// Pfad extrahieren
		c = (LPSTR)strName + lstrlen(strName);
		while(  *c!=':'  &&  *c!='\\'  &&  c>(LPSTR)strName  )
			c--;
		if(  c>strName  &&  *c=='\\'  )
		{
			*c = 0;
			lstrcpy( strLastSavePath, strName );
			*c = '\\';
		}
	}

	InvalidateRect( hWnd, NULL, TRUE ); // Repaint to display the new name
	if(  wFilterIndex!=NULL  )
		*wFilterIndex = (BYTE)ofnTemp.nFilterIndex;
	if(  Suc  )
		return ofnTemp.nFilterIndex;	// Ausgewählte Extension
	return FALSE;
}



#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif

//*********************************************************************************
// Function name- CMUGetFolderName
// Description- Get a folder path
// In- lpszTitle - title for caption
//  hwndOwner - reference to parent window
//  strRootFolder - root folder	(NULL = default)
//  strSelFolder - current folder (empty = default)
// Flags (see names)
// Out    - strSelFolder - selected folder path by user
// Return- TRUE if user select OK, else FALSE.
//*********************************************************************************
BOOL WINAPI	CMUGetFolderCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

BOOL CMUGetFolderName( const HWND hwnd, LPCSTR lpszTitle, LPSTR strSelFolder )
{
	static char pszDisplayName[MAX_PATH];
	LPITEMIDLIST pIdl;
	BROWSEINFO	bi;
	OFSTRUCT		of;

	// if this is a filename, cut the filename part
	while(  strSelFolder!=NULL  &&  strSelFolder[0]!=0  )
	{
		LPSTR	*c=strrchr(strSelFolder,'\\');
		if(  SetCurrentDirectory( strSelFolder )  )
		{
			GetCurrentDirectory( MAX_PATH, pszDisplayName );
			break;
		}
		if(  c==NULL  )
			break;
		*c = 0;
	}
	// Init structure
	bi.hwndOwner = hwnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = pszDisplayName;
	bi.lpszTitle = lpszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT;
	bi.lpfn = CMUGetFolderCallbackProc;
	bi.lParam = (LPARAM)strSelFolder;
	if(  strSelFolder==NULL  )
		bi.lParam = (LPARAM)pszDisplayName;

	//OleInitialize(NULL);
	pIdl = SHBrowseForFolder(&bi);
	//OleUninitialize();
	if (pIdl != NULL)
	{
		int b = SHGetPathFromIDList(pIdl, pszDisplayName);
		if (b!=0)
		{
			if(  strSelFolder!=NULL  )
				strcpy(strSelFolder,pszDisplayName);
			return TRUE;
		}
	}
	return FALSE;
}
// from Codeguru


// Needed to set default folder
BOOL WINAPI	CMUGetFolderCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg)
	{
		case BFFM_INITIALIZED:
			SendMessage( hwnd, BFFM_SETSELECTION,TRUE,lpData );
		break;
		case BFFM_SELCHANGED:
		{
			HANDLE	hs;
			WIN32_FIND_DATA	fd;
			TCHAR	szDir[MAX_PATH], szCurr[MAX_PATH], *p;
			WORD	no_of_files=0;

			/* In Statusmessages are needed insert here ... */
			if (SHGetPathFromIDList((LPITEMIDLIST) lParam ,szDir))
			{
				GetCurrentDirectory( MAX_PATH, szCurr );
				SetCurrentDirectory( szDir );
				lstrcat( szDir, "\\*.*" );
				hs = FindFirstFile( szDir, &fd );
				while(  FindNextFile(hs,&fd)  &&  hs!=INVALID_HANDLE_VALUE  )
				{
					if(  fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY  )
						continue;
					// check extension
					p = fd.cFileName+lstrlen(fd.cFileName)-3;
					if(  p[-1]!='.'  )
						continue;
					// check extension for: par, hdf, hdz, xyz 000...999
					if(  !(*p=='0'  ||  atoi(p)>0)  &&  lstrcmpi(p,"par")  &&  lstrcmpi(p,"hdf")  &&  lstrcmpi(p,"hdz")  &&  lstrcmpi(p,"afm")  &&  lstrcmpi(p,"xqd")  )
						continue;
					// add file here!
					no_of_files ++;
				}
				FindClose(hs);
				SetCurrentDirectory( szCurr );
				if(  no_of_files==0  )
				{
					SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)STR_NO_FILES);
					SendMessage(hwnd,BFFM_ENABLEOK,0,0);
				}
				else
				{
					wsprintf( szDir, STR_FOUND_FILES, no_of_files );
					SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
					SendMessage(hwnd,BFFM_ENABLEOK,0,TRUE);
				}
			}
		}
		break;
	}
	return 0;
}
