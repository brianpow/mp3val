/*
 * MP3val-frontend - a frontend for MP3val program
 * Copyright (C) 2005-2006 Alexey Kuznetsov (ring0)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "scanlist.h"
#include "spawn.h"
#include "listman.h"
#include "commands.h"

#include <commctrl.h>

CSpawner MySpawner;

extern HANDLE hStdinWrite, hStdoutRead;
extern HWND hProgress,hListView,hEdit;
extern int iViewMode;
extern bool bClicked;
extern char *szStates[5];

DWORD WINAPI ScanListWrapper(LPVOID p) {
	ScanListPars *pars;
	pars=(ScanListPars *)p;
	return ScanList(pars->hListView,pars->plist,pars->selected,pars->fix);
}

/* ATTENTION: it is not currently possible to display
 * multiple types of files unless you are using
 * VM_EVERYTHING mode
 */

int ScanList(HWND hListView, CFileList* plist, bool selected, bool fix) {
	int res;
	int i;
	int state;
	LVITEM lvItem;
	int pos_cor;
	int iProcessed=0;
	
	if(!fix) res=MySpawner.SpawnProcess("mp3val.exe -p");
	else res=MySpawner.SpawnProcess("mp3val.exe -f -p");
	
	if(res) {
		MessageBox(NULL,"Cannot launch mp3val.exe","Error",MB_OK|MB_ICONERROR);
		return 1;
	}
	
	PostMessage(hProgress,PBM_SETPOS,(WPARAM)0,0);
	if(selected) PostMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,ListView_GetSelectedCount(hListView)));
	else PostMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,ListView_GetItemCount(hListView)));
	
	pos_cor=0;
	
	for(i=0;i<=ListView_GetItemCount(hListView)-1;i++) {
		if((ListView_GetItemState(hListView,i,LVIS_SELECTED)&LVIS_SELECTED)||!selected) {
			state=ScanFile(i-pos_cor,plist,&MySpawner,fix);
			plist->changestate(iViewMode,i-pos_cor,state);
			if(state!=iViewMode&&iViewMode) pos_cor++;
			SendMessage(hProgress,PBM_STEPIT,0,0);
			
			lvItem.mask=LVIF_TEXT;
			lvItem.iItem=i;
			lvItem.iSubItem=1;
			lvItem.pszText=szStates[state];
			SendMessage(hListView,LVM_SETITEM,0,(LPARAM)&lvItem);

			if(!bClicked) {
				ListView_EnsureVisible(hListView,i,TRUE);
				HandleSelectionChange(i);
			}
			iProcessed++;
		}
	}
	
	MySpawner.DespawnProcess();
	
	if(!bClicked&&iProcessed) SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"Scanning completed!");
	
	if(!iProcessed) MessageBox(GetParent(hListView),"Nothing to scan","MP3val-frontend",MB_OK|MB_ICONWARNING);
	else {
		if(!fix) MessageBox(GetParent(hListView),"Scan completed","MP3val-frontend",MB_OK|MB_ICONINFORMATION);
		else MessageBox(GetParent(hListView),"Scan and repair completed","MP3val-frontend",MB_OK|MB_ICONINFORMATION);
	}
	
	if(iViewMode) RefreshView(iViewMode);
	
	return 0;
}

int ScanFile(int i,CFileList *plist,CSpawner *pSpawner,bool fix) {
	FileInfo fi;
	char strbuf[2048];
	int newstate=ST_NORMAL;
	
	if(!(plist->getfileno(iViewMode,i,&fi))) return 0;
	
	if(fi.state!=ST_PROBLEM&&fi.state!=ST_NOT_SCANNED&&fix) return fi.state;
	
	pSpawner->writestr(fi.szFileName);
	
	fi.proot->cleanup();
	
	do {
		pSpawner->readstr(strbuf,2048);
		fi.proot->addnode(strbuf);
		if(!memcmp(strbuf,"WARNING",lstrlen("WARNING"))) newstate=ST_PROBLEM;
		else if(!memcmp(strbuf,"ERROR",lstrlen("ERROR"))) newstate=ST_PROBLEM;
		else if(!memcmp(strbuf,"FIXED",lstrlen("FIXED"))) newstate=ST_FIXED;
	} while(lstrcmp(strbuf,"Done!"));
	
	return newstate;
}
