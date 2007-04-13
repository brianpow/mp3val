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

#include "scandir.h"
#include "commands.h"
#include <commctrl.h>

extern HWND hEdit;
extern int iViewMode;

DWORD lasttime;

DWORD WINAPI ScanDirWrapper(LPVOID p) {
	ScanDirPars *pars;
	int res;
	
	pars=(ScanDirPars *)p;
	lasttime=0;
	
	res=ScanDirForFiles(pars->pl,pars->szDirName);
	SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"Scanning completed. Refreshing list...");
	RefreshView(iViewMode);
	SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"Operation completed successfully.");
	return res;
}

int ScanDirForFiles(CFileList *pl,char *szDirName) {
	char buf[MAX_PATH+1];
	char pathbuf[MAX_PATH+1];
	char *filepart;
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
	DWORD curtime;
	
	GetCurrentDirectory(MAX_PATH+1,buf);
	
	if(!SetCurrentDirectory(szDirName)) return 0;
	
	curtime=GetTickCount();
	if(curtime-lasttime>500) {
		GetFullPathName(szDirName,MAX_PATH+1,pathbuf,&filepart);
		SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"");
		SendMessage(hEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"Scanning directory: ");
		SendMessage(hEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)pathbuf);
		lasttime=curtime;
	}
	
	hFind=FindFirstFile("*",&wfd);
	if(hFind!=INVALID_HANDLE_VALUE) {
		do {
			if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				if(lstrcmp(wfd.cFileName,".")&&lstrcmp(wfd.cFileName,"..")) ScanDirForFiles(pl,wfd.cFileName);
			}
			if(!lstrcmp(".mp3",&wfd.cFileName[lstrlen(wfd.cFileName)-4])) {
				GetFullPathName(wfd.cFileName,MAX_PATH+1,pathbuf,&filepart);
				pl->addfile(pathbuf);
			}
		} while(FindNextFile(hFind,&wfd));
		FindClose(hFind);
	}
	
	SetCurrentDirectory(buf);
	
	return 0;
}
