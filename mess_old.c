

/* Schreibt dir zuletzt gelesenen Daten ins Oszi
 * (ist eine Thread-Funktion, läuft also Quasi im Hintergrund!)
 */
int	Read2Scanlines( LPARAM hDlg )
{
	static	iXPts;
	int			iDir, i;
	int			iX=0;

	iXPts = Scan.iXPts-1;
	if(  Scan.iDir&LINKS2RECHTS  )
		iDir = LINKS2RECHTS;
	else
		iDir = (Scan.iDir&RECHTS2LINKS);

	while(1)	// Forever
	{
		if(  bComm  )
		{
			// Falls gerade nicht frei => 5 ms Pause
			Sleep(10);
			continue;
		}

		bComm = TRUE;
		i = (*ReadMailbox)( 0, 0 );
		switch(  i  )
		{

			case ABORTSCAN: // Abbruch oder einfach fertig ...
				if(  iDir&LINKS2RECHTS  )
				{
					OsziMax[0] = iX;
					OsziMax[2] = iX;
				}
				else
					OsziMax[1] = iX;
				RedrawOszi( (HWND)hDlg );
				bComm = FALSE;
				return -1;

			case STARTLINE:	// Neue Zeile
				if(  iDir&LINKS2RECHTS  &&  iX!=0  )
				{
					OsziNum[0] = iX;
					OsziNum[2] = iX;
					iX = 0;
					iDir = RECHTS2LINKS;
					RedrawOszi( (HWND)hDlg );
				}
				else if(  iDir&RECHTS2LINKS  &&  iX!=0  )
				{
					OsziNum[1] = iX;
					iX = 0;
					iDir = LINKS2RECHTS;
					RedrawOszi( (HWND)hDlg );
				}
				break;

			default:
				if(  iDir&LINKS2RECHTS  )
				{
					OsziDaten[0][iX] = (i>>16);
					OsziDaten[2][iX] = (short)i;
					iX ++;
				}
				else
				{
					// Nun zurück
					OsziDaten[1][iXPts-iX] = (i>>16);
					iX ++;
				}
				break;
		}
	}
}
// 11.11.98



