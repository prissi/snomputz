				case	IDM_AUTOKORRELATION:
				if(  modus&TOPO  &&  pBmp->pSnom[pBmp->iAktuell].Topo.puDaten  )
				{
					// for progress bar
					HWND	hProgress, hwndPB;
					RECT	rcClient;
					int		cyVScroll;
					// 2D Autokorrekation
					LPSNOMDATA	pNewSnom, pSnom=&(pBmp->pSnom[pBmp->iAktuell]);
					LPUWORD			puDaten = pSnom->Topo.puDaten;
					LONG				dx, dy, i, j;
					LONG				x, y, w, h, yoff, dyoff;
					LPLONG			pf1;
					LPFLOAT			pf2;
					float				fMax=0.0, fMin=0.0, fMittel;
					double			fTemp=0;

					if(  (LONG)puDaten<256  )
						puDaten = pBmp->pSnom[(WORD)pBmp->pSnom[pBmp->iAktuell].Topo.puDaten-1].Topo.puDaten;
					WarteMaus();

					if(  (pNewSnom=pAllocNewSnom( pBmp, modus ))!=NULL  )
					{
						pf1 = pMalloc( sizeof(float)*pSnom->w*pSnom->h );
						pf2 = pMalloc( sizeof(float)*pSnom->w*pSnom->h );
#if 1
						/*** Die Autokorrelation ist folgendermaßen definiert:
						 *** G(l,m) := 1/((H-m)*(W-l)) \sum_{j=1}^{H-m} \sum_{i=1}^{W-l} z_{i,j}*z_{i+l,j+m}
						 *** ACHTUNG: Die Einheit ist Länge^2!
						 *** ACHTUNG: <z_i> := 0, sonst abziehen!
						 ***/

							w = pSnom->w;
							h = pSnom->h;

							InitCommonControls(); 
							hProgress = CreateDialog( hInst, "Show_Progress", pBmp->hwnd, StdDialog );
					    GetClientRect(hProgress, &rcClient); 
					    cyVScroll = GetSystemMetrics(SM_CYVSCROLL); 
					    hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPSTR) NULL, WS_CHILD | WS_VISIBLE, rcClient.left, rcClient.bottom - cyVScroll, rcClient.right, cyVScroll, hProgress, (HMENU) 0, hInst, NULL); 
					    // Set the range and increment of the progress bar. 
							SetWindowText( hProgress, "Calculating 2D autocorrelation ..." );
					    SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, h)); 
					    SendMessage(hwndPB, PBM_SETSTEP, (WPARAM) 1, 0); 

							// substract mean value before starting
									//	pSnom->Topo.fSkal lassen wir noch unter den Tisch fallen!
							for(  i=0;  i<w*h;  i++  )
								fTemp += (double)(pf1[i] = puDaten[i]);
							fMittel = fTemp/(w*h);	// mittelwert für Autokorrelation
							for(  i=0;  i<w*h;  i++  )
								pf1[i] -= fMittel;

							for(  dyoff=dy=0;  dy<h;  dy++,dyoff+=w  )
							{
				        SendMessage(hwndPB, PBM_STEPIT, 0, 0); 
								Sleep(0);
								for(  dx=0;  dx<w;  dx++  )
								{
									// Calculate now autocorrelation
									fTemp = 0;
									for(  yoff=y=0;  y<h-dy;  y++,yoff+=w  )
									{
										for(  x=0;  x<w-dx;  x++  )
											fTemp += pf1[x+yoff]*pf1[dx+x+dyoff+dy]; // multiply outside the loop
									}
									pf2[dx+dyoff] = fTemp/(double)((h-dy)*(w-dx));
								}
							}
							DestroyWindow( hwndPB );
							DestroyWindow( hProgress );
#else
#if 0
						// Does not work -- phase is lost!!!

						// do use of the autocorrelation funktion!
						w = pSnom->w;
						h = pSnom->h;

						// substract mean value before starting
						for(  i=0;  i<w*h;  i++  )
							fTemp += pf2[i] = puDaten[i]*pSnom->Topo.fSkal;
						fMittel = fTemp/(w*h);	// mittelwert für Autokorrelation
						for(  i=0;  i<w*h;  i++  )
							pf2[i] -= fMittel;

						// do autocorrelation in x axis
						for(  y=0;  y<h;  y++  )
							Autokorrelation( pf1+(y*w), pf2+(y*w), w, TRUE );
						// turn right
						for(  y=0;  y<w;  y++  )
						{
							for(  x=0;  x<h;  x++  )
								pf2[x+y*h] = pf1[(h-x-1)*w+y];
						}
						// beware now w is heigth and h is width!
						// so x ist y ...
						for(  x=0;  x<w;  x++  )
							Autokorrelation( pf1+(x*h), pf2+(x*h), h, TRUE );
						// And now we need to turn this back ...
						for(  y=0;  y<h;  y++  )
						{
							for(  x=0;  x<w;  x++  )
								pf2[x+y*w] = pf1[h*x+y];
						}
						// result is now in pf2!
#endif
						// Substract mean value before starting
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
							fTemp += (pf1[i] = puDaten[i]*pSnom->Topo.fSkal);
						fMittel = fTemp/(pSnom->w*pSnom->h);	// mittelwert für Autokorrelation
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
							pf1[i] -= fMittel;

						// do everything by our own (BEWARE: there is a bug in this routine?!)
						for(  dy=0;  dy<pSnom->h;  dy++  )
						{
							sprintf( str, "%s %i", "Zeile", dy+1 );
							StatusLine( str );
							Sleep(0);
							for(  dx=0;  dx<pSnom->w;  dx++  )
							{
								fTemp = 0.0;
								for(  i=0;  i<pSnom->h-dy;  i++  )
									for(  j=0;  j<pSnom->w-dx;  j++  )
										fTemp += pf1[i*pSnom->w+j]*pf1[(i+dy)*pSnom->w+(j+dx)];
								fTemp /= (double)i*j;// pSnom->w*pSnom->h; // ACHTUNG: Korrekt wäre "i*j"!
								pf2[dx+dy*pSnom->w] = (float)fTemp;
							}
						}
#endif
						// Neu Skalieren
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
						{
							if(  pf2[i]>fMax  )
								fMax = pf2[i];
							if(  pf2[i]<fMin  )
								fMin = pf2[i];
						}
						fMax -= fMin; // to prevent overflow

						// floats wieder nach WORD
						for(  i=0;  i<pSnom->w*pSnom->h;  i++  )
							pNewSnom->Topo.puDaten[i] = (pf2[i]-fMin)*65000.0/fMax;

						BildMax( &(pNewSnom->Topo), pSnom->w, pSnom->h );
						RecalcCache( pBmp, TRUE, TRUE );
						NormalMaus();
						InvalidateRect( hwnd, NULL, FALSE );
						MemFree( pf1 );
						MemFree( pf2 );
					}
				}
				break;


