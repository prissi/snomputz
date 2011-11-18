/**************************************************************************************
****	Programmier- und History-Routinen
**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <windows.h>
#include <windowsx.h>
//#include <ctl3d.h>
#include <commctrl.h>
#include <commdlg.h>
#if !defined(_WIN32) && !defined(__WIN32__)
#include <print.h>
#endif

#include "filebox.h"

#include "snomputz.h"
#include "snom-typ.h"
#include "snom-win.h"
#include "snom-mem.h"
#include "snom-dlg.h"
#include "snom-dsp.h"
#include "snom-dat.h"
#include "snom-wrk.h"
#include "snom-mat.h"

#define SNOM_PROGRAMM
#include "snom-prg.h"

/********************************************************************************************/
/* Wenn EXECUTE==FALSE, dann nur Zeile ansehen, nicht auswerten! */

/* if-Anweisung:
 * PROG_BEGIN_BLOCK = TRUE;
 * Stackpointer erhöhen
 *	EXECUTE = je nach if-Anweisung
 *	CONDITION = je nach if-Anweisung
 *	CONT_ON_RETURN = TRUE;	// Nach dem Ende einfach Fortsetzten
 *
 * else-Anweisung:
 * wie eine if-Anweisung mit dem Ergebnis !CONDITION
 */

/* while-Anweisung
 * PROG_BEGIN_BLOCK = TRUE;
 * Stackpointer erhöhen
 *	EXECUTE = je nach while-Anweisung
 *	CONDITION = je nach while-Anweisung
 *	CONT_ON_RETURN = entgegengesetz zur while-Anweisung;
 */

/* Wenn "{" und falls BEGIN_BLOCK = FALSE;
 *	Stack erhöhen
 *	CONDITION = 2; // War gar keine
 *	CONT_ON_RETURN = TRUE;	// Nach dem Ende einfach Fortsetzten
 */
 
/* Wenn "}" Stackpointer > 0
 * falls CONT_ON_RETURN
 * neue Zeilennummer = aktuelle Zeile + 1
 * sonst neue Zeilennummer gleich letzte Zeilenummer
 */


// erzeugt den nächsten Variablennamen
// pVar muss auf eine gültige Struktur zeigen, wenn bReadOnly: Nur Kopie gespeichert
// Zeigt danach entweder auf Original oder Kopie
BOOLEAN	GetVariable( LPPROGPTR pPrg, LPSTR *ppStr, VARIABLEN **pVar, BOOLEAN bReadOnly )
{
	int		i, iNeu=-1;
	BYTE	str[256];

	switch(  **ppStr  )
	{

		// Schwierigste zuerst: Ein Handle
		case '@':
		{
			WORKMODE	wm;
			// Gibt es einen Untertyp?
			if( (*ppStr)[1]=='(' )
			{
				while(  **ppStr!=')'  &&  **ppStr>0  )
				{
					if( tolower(**ppStr)=='t' )
						wm |= TOPO;
					else if( tolower(**ppStr)=='e' )
						wm |= ERRO;
					else if( tolower(**ppStr)=='l' )
						wm |= LUMI;
					else
						// War nicht T|E|L ...
						goto FalscherUntertyp;
					(*ppStr)++;
				}
			}
			else
				// Kein Untertyp
				wm = TOPO|ERRO|LUMI;

			// Namen herausfinden
			for(  i=0;  i<256  &&  isalnum(**ppStr);  i++  )
			{
				str[i] = **ppStr;
				(*ppStr)++;
			}
			str[i] = 0;
			if(  i==0  &&  i==256  )
				goto KeinName;

			// Gibt es die Variable schon?
			for(  i=0;  i<PROG_MAX_HANDLES;  i++  )
			{
				if(  iNeu==-1  &&  pPrg->Handles[i].Name[0]==0  )
					iNeu=i;
				else if(  lstrcmp(str, pPrg->Handles[i].Name )==0  )
					break;
			}

			// Event. neue Anlegen ...
			if(  i==PROG_MAX_HANDLES  )
			{
				if(  bReadOnly  )
					goto UndefinierteVariable;
				if(  iNeu==-1  )
					goto ZuvieleVariablen;
				i = iNeu;
				lstrcpy( pPrg->Handles[i].Name, str );
				pPrg->Handles[i].Bmp.pBmp = NULL;
				pPrg->Handles[i].Bmp.bModus = TOPO|ERRO|LUMI;
				pPrg->Handles[i].Typ = TYP_HANDLE;
			}

			// Werte in lokale Variable kopieren
			if(  bReadOnly  )
				MemMove( (*pVar), &(pPrg->Handles[i]), sizeof(VARIABLEN) );
			else
				*pVar = &(pPrg->Handles[i]);
			(*pVar)->Bmp.bModus = wm;
			if(  (wm==TOPO  ||  wm==ERRO  ||  wm==LUMI)  &&  bReadOnly  )
				(*pVar)->Typ = TYP_BILD;
		}
		return TRUE;


		// Neues Bild
		case '#':
		{
			(*ppStr)++;
			// Namen herausfinden
			for(  i=0;  i<256  &&  isalnum(**ppStr);  i++  )
			{
				str[i] = **ppStr;
				(*ppStr)++;
			}
			str[i] = 0;
			if(  i==0  &&  i==256  )
				goto KeinName;

			// Gibt es die Variable schon?
			for(  i=0;  i<PROG_MAX_BILDER;  i++  )
			{
				if(  iNeu==-1  &&  pPrg->Bilder[i].Name[0]==0  )
					iNeu=i;
				else if(  lstrcmp(str, pPrg->Bilder[i].Name )==0  )
					break;
			}

			// Event. neue Anlegen ...
			if(  i==PROG_MAX_BILDER  )
			{
				if(  bReadOnly  )
					goto UndefinierteVariable;
				if(  iNeu==-1  )
					goto ZuvieleVariablen;
				i = iNeu;
				lstrcpy( pPrg->Bilder[i].Name, str );
				pPrg->Bilder[i].Bmp.pBmp = NULL;
				pPrg->Bilder[i].Bmp.bModus = 0;
				pPrg->Bilder[i].Typ = TYP_BILD;
			}

			// Werte in lokale Variable kopieren
			if(  bReadOnly  )
				MemMove( (*pVar), &(pPrg->Bilder[i]), sizeof(VARIABLEN) );
			else
				(*pVar) = &(pPrg->Bilder[i]);
		}
		return TRUE;

		// Neuer String
		case '$':
		{
			(*ppStr)++;
			// Namen herausfinden
			for(  i=0;  i<256  &&  isalnum(**ppStr);  i++  )
			{
				str[i] = **ppStr;
				(*ppStr)++;
			}
			str[i] = 0;
			if(  i==0  &&  i==256  )
				goto KeinName;

			// Gibt es die Variable schon?
			for(  i=0;  i<PROG_MAX_STRINGS;  i++  )
			{
				if(  iNeu==-1  &&  pPrg->Strings[i].Name[0]==0  )
					iNeu=i;
				else if(  lstrcmp(str, pPrg->Strings[i].Name )==0  )
					break;
			}

			// Event. neue Anlegen ...
			if(  i==PROG_MAX_STRINGS  )
			{
				if(  bReadOnly  )
					goto UndefinierteVariable;
				if(  iNeu==-1  )
					goto ZuvieleVariablen;
				i = iNeu;
				lstrcpy( pPrg->Strings[i].Name, str );
				pPrg->Strings[i].str = pMalloc( 1024 );
				pPrg->Strings[i].Typ = TYP_STRING;
			}

			// Werte in lokale Variable kopieren
			if(  bReadOnly  )
				MemMove( *pVar, &(pPrg->Strings[i]), sizeof(VARIABLEN) );
			else
				*pVar = &(pPrg->Strings[i]);
		}
		return TRUE;

		// Neue Zahl
		case '!':
		{
			(*ppStr)++;
			// Namen herausfinden
			for(  i=0;  i<256  &&  isalnum(**ppStr);  i++  )
			{
				str[i] = **ppStr;
				(*ppStr)++;
			}
			str[i] = 0;
			if(  i==0  &&  i==256  )
				goto KeinName;

			// Gibt es die Variable schon?
			for(  i=0;  i<PROG_MAX_ZAHLEN;  i++  )
			{
				if(  iNeu==-1  &&  pPrg->Zahlen[i].Name[0]==0  )
					iNeu=i;
				else if(  lstrcmp(str, pPrg->Zahlen[i].Name )==0  )
					break;
			}

			// Event. neue Anlegen ...
			if(  i==PROG_MAX_ZAHLEN  )
			{
				if(  bReadOnly  )
					goto UndefinierteVariable;
				if(  iNeu==-1  )
					goto ZuvieleVariablen;
				i = iNeu;
				lstrcpy( pPrg->Zahlen[i].Name, str );
				pPrg->Zahlen[i].fWert = 0.0;
				pPrg->Zahlen[i].Typ = TYP_ZAHL;
			}

			// Werte in lokale Variable kopieren
			if(  bReadOnly  )
				MemMove( *pVar, &(pPrg->Zahlen[i]), sizeof(VARIABLEN) );
			else
				*pVar = &(pPrg->Zahlen[i]);
		}
		return TRUE;

	}
	return TRUE;

FalscherUntertyp:
	// Hier nur bei Fehler "Falscher Untertyp"
	FehlerRsc( PROG_VAR_SUBTYPE );
	return FALSE;
	
KeinName:
	// Nach einem Identifier kam keine Variable
	FehlerRsc( PROG_VAR_NULLNAME );
	return FALSE;

ZuvieleVariablen:
	// Der Platz für Variablen ging aus ... (Vielleicht sollte man mit Snomputz keine linearen Gleichungssysteme normieren ...)
	FehlerRsc( PROG_VAR_OUT_OF_NAMES );
	return FALSE;

UndefinierteVariable:
	// Auf der rechten Seite einer Gleichung sollten keine Variablen definiert werden ...
	FehlerRsc( PROG_VAR_UNDEF );
	return FALSE;
}
// 12.1.99




PROGRAMMSTATUS	DoOperation( VARIABLEN **pVar1, VARIABLEN *pVar2, int iOperation )
{
	// Kein Casting nötig
	if(  iOperation==0  )
	{
		// Zuweisung
		if(  (*pVar1)->Typ!=NONE  &&  pVar2->Typ!=(*pVar1)->Typ)  
			goto Typkonflikt;
		MemMove( &((*pVar1)->fWert), &(pVar2->fWert), sizeof(VARIABLEN)-256 );
		return PROG_EXECUTE;
	}

	// Nur Zuweisung auf Handles
	if(  (*pVar1)->Typ==TYP_HANDLE  ||  pVar2->Typ==TYP_HANDLE  )
		goto UnzulaessigerOperator;

	// Nur aneinanderhängen erlaubt
	if(  (*pVar1)->Typ==TYP_STRING  &&  iOperation!='+'  )
		goto UnzulaessigerOperator;
	
	// Nur Zuweisung auf Handles
	if(  pVar2->Typ==TYP_BILD  &&  (*pVar1)->Typ!=TYP_BILD  )
		goto UnzulaessigerOperator;

	switch(iOperation)
	{
		case '+':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert += pVar2->fWert;
			if(  (*pVar1)->Typ==TYP_STRING  )
			{
				if(  pVar2->Typ==TYP_STRING  )
					lstrcat( (*pVar1)->str, pVar2->str );
				else if(  pVar2->Typ==TYP_ZAHL  )
					sprintf( (*pVar1)->str+lstrlen((*pVar1)->str), "%10.3f", pVar2->fWert );
				else
				goto UnzulaessigerOperator;
			}
			if(  (*pVar1)->Typ==TYP_BILD  )
			{
				// Bilder addieren ...
			}
			return PROG_EXECUTE;

		case '-':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert -= pVar2->fWert;
			return PROG_EXECUTE;

		case '*':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert *= pVar2->fWert;
			return PROG_EXECUTE;

		case '/':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert /= pVar2->fWert;
			return PROG_EXECUTE;

		case '%':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert = fmod( (*pVar1)->fWert, pVar2->fWert );
			return PROG_EXECUTE;

		case '&':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert = ((long)(*pVar1)->fWert) & ((long)pVar2->fWert);
			return PROG_EXECUTE;

		case '|':
			if(  (*pVar1)->Typ==TYP_ZAHL  )
				(*pVar1)->fWert *= pVar2->fWert;
			return PROG_EXECUTE;
	}
	return PROG_EXECUTE;

Typkonflikt:
	FehlerRsc( PROG_EVAL_TYPES );
	return PROG_ABORT;

UnzulaessigerOperator:
	FehlerRsc( PROG_OP_ILLEGAL );
	return PROG_ABORT;

}
// 13.1.99




// Arbeitet einen Ausdruck solange durch, bis er einen einzigen Wert hat
// ppStr zeigt danach auf die Stelle, an der abgebrochen wurde
// Ruft sich evt. selbst auf ..
// Rechnet immer nur mit zwei Ausdrücken, ansonsten: Klammern ...
PROGRAMMSTATUS	EvaluateExpression( LPPROGPTR pPrg, LPSTR *ppStr, VARIABLEN **pErgebnis, BOOLEAN bKomma )
{
	VARIABLEN	Var1, Var2;
	VARIABLEN	*pVar1=&Var1, *pVar2=&Var2;
	int				iOperator=NONE;
	BOOLEAN		b;

	Var1.Typ = Var2.Typ = NONE;
	while(1)
	{
		// Leerzeichen, Tabs, ... entfernen
		while(  **ppStr<=32  &&  **ppStr>0  )
			(*ppStr)++;

		switch( **ppStr )
		{
			case '}':
			case '{':
					goto UnerlaubtesZeichen;

			case ',':	// Testen, ob Funktionsaufruf?
				if(  !bKomma  )
					goto UnerlaubtesZeichen;
				goto ZeilenEnde;

				// Zeilenende
			case '/':
				if(  (*ppStr)[1]!='/' )
					goto IsOperator;
				// Kommentar => Zeilenende ...
				goto ZeilenEnde;

			case 0:
			case ';':
ZeilenEnde:
				if(  Var1.Typ==NONE  ||  (Var2.Typ==NONE  &&  iOperator!=NONE)  )
					goto UnerwartetesZeilenende;
				// Einfach Zuweisung ausführen
				return DoOperation( pErgebnis, &Var1, 0 );

				// Zahlenkonstanten
			case '-':
			case '+':
				if(  Var1.Typ!=NONE  ||  iOperator==NONE  )
					goto	IsOperator;
			case '.':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if(  Var1.Typ==NONE  )
				{
					Var1.Typ = TYP_ZAHL;
					Var1.fWert = strtod( *ppStr, ppStr );
				}
				else if(  Var2.Typ==NONE  )
				{
					Var2.Typ = TYP_ZAHL;
					Var2.fWert = strtod( *ppStr, ppStr );
				}
				else
{ASSERT("Interner Fehler");}
				b = PROG_EXECUTE;	// Klappt immer ...
				goto Auswertung;
				break;

				// Zeichenketten
			case '\"':
			{
				VARIABLEN	*pVar=NULL;

				if(  Var1.Typ==NONE  )
					pVar = &Var1;
				else if(  Var2.Typ==NONE  )
					pVar = &Var2;
				else
{ASSERT("Interner Fehler");}

				pVar->Typ = TYP_STRING;
				pVar->str = ++(*ppStr);
				while(  **ppStr!='\"'  )
				{
					if(  **ppStr==0  )
						goto	OffenerString;
					if(  *(*ppStr)++=='\\'  )
						switch(  **ppStr  )
						{
							case '\"':	*ppStr[-1] = '\"';
													lstrcpy( (*ppStr)-1, *ppStr );
												break;
							case '\\':	*ppStr[-1] = '\"';
													lstrcpy( (*ppStr)-1, *ppStr );
												break;
							case 'n':		*ppStr[-1] = 13;
													*ppStr[0] = 10;
												break;
						}
	
				}
				**ppStr = 0;
			}
			b = PROG_EXECUTE;
			goto Auswertung;
			break;

				// Auf der rechten Seite und in Funktionen muss es 
				// eine definierte Variable sein => keine neuen anlegen!
			case '@':
			case '#':
			case '$':
			case '!':
				if(  Var1.Typ==NONE  )
					b = GetVariable( pPrg, ppStr, &pVar1, TRUE  );
				else if(  Var2.Typ==NONE  )
					b = GetVariable( pPrg, ppStr, &pVar2, TRUE  );
				else
{ASSERT( "Interner Fehler!" );}
				goto	Auswertung;

			// Klammerung => neuer Unterausdruck
			case '(':
			{
				LPSTR	c=strchr( *ppStr, ')' );

				if(  c==NULL  )
					goto	KlammerFehler;
				if(  Var1.Typ  &&  iOperator==NONE  )
					goto	KeinOperator;
				*c = 0;
				(*ppStr)++;
				if(  Var1.Typ==NONE  )
					b = EvaluateExpression( pPrg, ppStr, &pVar1, bKomma );
				else if(  Var2.Typ==NONE  )
					b = EvaluateExpression( pPrg, ppStr, &pVar2, bKomma );
				else
{ASSERT( "Interner Fehler!" );}
				*ppStr = c+1;	// Zeichen nach schließender Klammer
			}
Auswertung:
			if(  !b  )
				return b;
			// Hier passierts ...
			if(  Var2.Typ!=NONE  )
			{
				if(  iOperator==NONE  )	// Zuweisung hier nicht mehr erlaubt!
					goto	KeinOperator;
				b = DoOperation( &pVar1, &Var2, iOperator );
				if(  !b  )
					return b;
			}
			break;

			case '=':
				if(  (*ppStr)[1]!='='  )
					goto IllegaleZuweisung;
				(*ppStr)++;
				goto IsOperator;

			case '*':
			case '%':
			case '&':
			case '|':
IsOperator:
				iOperator = **ppStr;
				(*ppStr)++;
				break;

			default:
				goto	UnerlaubtesZeichen;
		}
	}
	return PROG_EXECUTE;

OffenerString:
	// Zeichenkette nicht geschlossen
	FehlerRsc( PROG_EVAL_STRING );
	return FALSE;

IllegaleZuweisung:
	FehlerRsc( PROG_EVAL_ASSIGN );
	return PROG_ABORT;

KeinOperator:
	FehlerRsc( PROG_EVAL_OPERATOR );
	return PROG_ABORT;

UnerlaubtesZeichen:
	FehlerRsc( PROG_EVAL_CHAR );
	return PROG_ABORT;

KlammerFehler:
	FehlerRsc( PROG_PARSE_BRACKETS );
	return PROG_ABORT;

UnerwartetesZeilenende:
	// Hier nur bei Fehler: Zeile zu früh zu Ende
	FehlerRsc( PROG_PARSE_EOL );
	return PROG_ABORT;
}
// 13.1.99




// Führt eine Programmzeile aus
PROGRAMMSTATUS	ParseLine( LPPROGPTR pPrg, LPSTR pStr )
{
	VARIABLEN		*pVar, InitVar;
	BOOLEAN			bKeineNeuen=FALSE, b;

	int				iVar=0;

	InitVar.Typ = NONE;
	pVar = &InitVar;
	while(1)
	{
		// Leerzeichen, Tabs, ... entfernen
		while(  *pStr<=32  &&  *pStr>0  )
			pStr++;

		switch( *pStr )
		{
				// Zeilenende
			case 0:
			case ';':
				pPrg->Stack[pPrg->iStackPtr].lZeilenNummer ++;
				return TRUE;

			case '@':
			case '#':
			case '$':
			case '!':
				if(  iVar!=0  )
					goto UnerwarteteEingabe;
				GetVariable( pPrg, &pStr, &pVar, FALSE );
				iVar = 1;
				break;

			case '=':
				if(  iVar!=0  &&  pStr[1]!='='  )
				{
					// Eine ganz normale Zuweisung ...
					pStr ++;
					while(  *pStr<=32  &&  *pStr>0  )
						pStr++;
					if(  isalpha(*pStr)  )
					{
						b = PROG_EXECUTE;//Funktion aufrufen
						pPrg->Stack[pPrg->iStackPtr].lZeilenNummer ++;
						return b;
					}
					else
					{
						b = EvaluateExpression( pPrg, &pStr, &pVar, FALSE );
						pPrg->Stack[pPrg->iStackPtr].lZeilenNummer ++;
						return b;
					}
				}
				// Operatoren
			case '/':
				if(  pStr[1]=='/' )
				{
					// Kommentar => Zeilenende ...
					*pStr=0;
					break;
				}
			case '*':
			case '+':
			case '-':
			case '&':
			case '|':
					goto UnerwarteteEingabe;
				break;

			// Block beginnen
			case '{':
				if(  !pPrg->Stack[pPrg->iStackPtr].BEGIN_BLOCK  )
				{
					if(  pPrg->iStackPtr>=255  )
						goto StackUeberlauf;
					pPrg->iStackPtr ++;
					pPrg->Stack[pPrg->iStackPtr].EXECUTE = TRUE;
					pPrg->Stack[pPrg->iStackPtr].CONDITION = UNDEF_CONDITION;
					pPrg->Stack[pPrg->iStackPtr].CONT_ON_RETURN = TRUE;
				}
				pPrg->Stack[pPrg->iStackPtr].lZeilenNummer = pPrg->Stack[pPrg->iStackPtr-1].lZeilenNummer;
				pPrg->Stack[pPrg->iStackPtr].BEGIN_BLOCK = FALSE;
				*pStr = 0;	// Also Zeile ausführen
				break;

			// Block enden lassen
			case '}':
				if(  pPrg->iStackPtr<=0  )
					goto StackUnterlauf;
				// Evt. Stacknummer retten
				if(  pPrg->Stack[pPrg->iStackPtr].CONT_ON_RETURN  )
					pPrg->Stack[pPrg->iStackPtr-1].lZeilenNummer = pPrg->Stack[pPrg->iStackPtr].lZeilenNummer;
				pPrg->iStackPtr --;
				pPrg->Stack[pPrg->iStackPtr].EXECUTE = TRUE;
				pPrg->Stack[pPrg->iStackPtr].CONDITION = UNDEF_CONDITION;
				pPrg->Stack[pPrg->iStackPtr].CONT_ON_RETURN = TRUE;
				*pStr = 0;
				break;

			default:
				if(  isalpha(*pStr)  )
					goto UnerwarteteEingabe;
				return PROG_EXECUTE;//Prozedur aufrufen
		}
	}

StackUnterlauf:
	FehlerRsc( PROG_PARSE_STACK_UNDER );
	return FALSE;

StackUeberlauf:
	FehlerRsc( PROG_PARSE_STACK_OVER );
	return FALSE;

UnerwarteteEingabe:
	FehlerRsc( PROG_PARSE_WRONG );
	return PROG_ABORT;

UnerwartetesZeilenende:
	// Hier nur bei Fehler: Zeile zu früh zu Ende
	FehlerRsc( PROG_PARSE_EOL );
	return FALSE;
}
// 12.1.99



// "Erkennt" eine Zeile
PROGRAMMSTATUS	GetPrgLine( LPSTR pSrc, LONG lLineNr, LPSTR pDest, LONG *plLen, LONG lLen )
{
	LONG	l;

	// Zuerst die richtige Zeile finden
	while(  lLineNr>0  )
	{
		if(  *pSrc==13  )
		{
			pSrc ++;
			if(  *pSrc==10  )
				pSrc ++;
			lLineNr--;
		}
		else if(  *pSrc==10  )
		{
			pSrc ++;
			lLineNr--;
		}
		else if(  *pSrc++==0  )
			return PROG_EOF;
	}

	// Dann diese kopieren
	for(  l=0;  l<lLen  &&  (*pSrc!=13 && *pSrc!=10 && *pSrc!=0);  l++  )
		*pDest++ = *pSrc++;
	*pDest++ = 0;
	*plLen = lLen;

	// Und noch etwas Fehlerbehandlung ...
	if(  l==lLen  )
		return PROG_ABORT;	// String zu lang
	return PROG_EXECUTE;
}
// 12.1.99




// Bereitet Programmablauf vor
BOOLEAN	RunProgram( LPSTR pPrg )
{
	LPPROGPTR				pProgPtr=NULL;
	PROGRAMMSTATUS	iError;
	BYTE						str[1024];
	long						lLen, i, lZeile=0;

	// Und initialisieren => Wir können mehrere Programme gleichzeitig ausführen
	pProgPtr = pMalloc( sizeof(PROGPTR) );
	for(  i=0;  i<PROG_MAX_HANDLES;  i++  )
		pProgPtr->Handles[0].Name[0] = 0;
	for(  i=0;  i<PROG_MAX_BILDER;  i++  )
		pProgPtr->Bilder[0].Name[0] = 0;
	for(  i=0;  i<PROG_MAX_ZAHLEN;  i++  )
		pProgPtr->Zahlen[0].Name[0] = 0;
	for(  i=0;  i<PROG_MAX_STRINGS;  i++  )
		pProgPtr->Strings[0].Name[0] = 0;
	pProgPtr->Stack[0].EXECUTE = TRUE;
	pProgPtr->Stack[0].CONDITION = 2;
	pProgPtr->iStackPtr = 0;
	
	do
	{
		iError = GetPrgLine( pPrg, pProgPtr->Stack[pProgPtr->iStackPtr].lZeilenNummer, str, &lLen, 1024 );
		if(  iError==PROG_EXECUTE  )
			iError = ParseLine( pProgPtr, str );
	}
	while(  iError==PROG_EXECUTE  );
	MemFree( pProgPtr );
	return iError==PROG_EOF;
}
// 12.1.99

/********************************************************************************************/

WNDPROC pOldProgEditProc;
HWND		hwndEdit;

/********************************************************************************************/

// Verwaltung Programmierfenster 
LRESULT WINAPI	ProgWndProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
	switch (message)
	{
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDM_PROG_RUN:
				{
					LPSTR	prg;
					LONG	len = GetWindowTextLength( hwnd );
					prg = pMalloc( len );
					GetWindowText( hwnd, prg, len );
					RunProgram( prg );
				}
			}
		}
		break;

		case WM_DESTROY:
			hwndEdit = NULL;
			break;
	}
	return CallWindowProc( pOldProgEditProc, hwnd, message, wParam, lParam);
} // 12.1.99