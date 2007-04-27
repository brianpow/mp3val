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

#include "base.h"

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <ole2.h>

#include "commands.h"
#include "listman.h"
#include "scandir.h"
#include "scanlist.h"
#include "spawn.h"
#include "resource.h"
#include "settings.h"

char szBrowseForFolderTitle[]="Choose a folder";

char szStateNotApplicable[]="N/A";
char szStateNotScanned[]="NOT SCANNED";
char szStateOK[]="OK";
char szStateProblem[]="PROBLEM";
char szStateFixed[]="FIXED";

char *szStates[5]={szStateNotApplicable,szStateNotScanned,szStateOK,szStateProblem,szStateFixed};

char szDirBeingAdded[MAX_PATH+1];

extern HWND hWnd,hListView,hEdit,hProgress;
extern HMENU hViewMenu,hPopup;
extern bool bClicked;
extern CSpawner MySpawner;
extern CSettings options;
ScanListPars pars;
ScanDirPars scandirpars;

CFileList list;
bool bWorkingWithList=false;

char szOpenFileName[OPENFILENAME_BUFSIZE];
OPENFILENAME ofn;

int iViewMode=VM_EVERYTHING;

HANDLE hThread=NULL;
HANDLE hDirThread=NULL;

BOOL CALLBACK OptionsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

int InitCommands() {
	char strbuf[2048];
	char strbuf2[2048]="Using core: ";
	CSpawner MySpawner; 
	memset(szOpenFileName,'\0',OPENFILENAME_BUFSIZE);
	
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=hWnd;
	ofn.hInstance=NULL;
	ofn.lpstrFilter="MP3 files (*.mp3)\0*.mp3\0MP2 files (*.mp2)\0*.mp2\0MP1 files (*.mp1)\0*.mp1\0All MPEG audio files (*.mp3,*.mp2,*.mp1)\0*.mp3;*.mp2;*.mp1\0All files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter=NULL;
	ofn.nMaxCustFilter=0;
	ofn.nFilterIndex=1;
	ofn.lpstrFile=(char *)szOpenFileName;
	ofn.nMaxFile=OPENFILENAME_BUFSIZE;
	ofn.lpstrFileTitle=NULL;
	ofn.nMaxFileTitle=0;
	ofn.lpstrInitialDir=NULL;
	ofn.lpstrTitle="Select file(s) for validation";
	ofn.Flags=OFN_ALLOWMULTISELECT|OFN_EXPLORER|OFN_FILEMUSTEXIST;
	ofn.nFileOffset=0;
	ofn.nFileExtension=0;
	ofn.lCustData=0;
	ofn.lpfnHook=NULL;
	ofn.lpTemplateName=NULL;
	
	if(!MySpawner.SpawnProcess("mp3val.exe -v")) {
		MySpawner.readstr(strbuf,2048);
		lstrcat(strbuf2,strbuf);
		SendMessage(hEdit,WM_SETTEXT,0,(LPARAM)strbuf2);
	}
	else {
		MessageBox(hWnd,"Cannot find mp3val.exe","Error!",MB_OK|MB_ICONERROR);
		ExitProcess(0);
	}
	
	szDirBeingAdded[0]='\0';
	
	return 0;
}

int RefreshView(int iViewType) {
	LVITEM lvItem;
	FileInfo fi;
	int i;
	int iTopIndex;
	
	bWorkingWithList=true;
	
	iTopIndex=ListView_GetTopIndex(hListView);

	SendMessage(hListView,LVM_DELETEALLITEMS,0,0);

	for(i=0;;i++) {
		if(!list.getfileno(iViewType,i,&fi)) break;
		lvItem.mask=LVIF_TEXT;
		lvItem.iItem=i;
		lvItem.iSubItem=0;
		lvItem.pszText=fi.szFileName;
		SendMessage(hListView,LVM_INSERTITEM,0,(LPARAM)&lvItem);
		lvItem.iSubItem=1;
		lvItem.pszText=szStates[fi.state];
		SendMessage(hListView,LVM_SETITEM,0,(LPARAM)&lvItem);
	}
	
	ListView_EnsureVisible(hListView,iTopIndex,FALSE);
	
	bWorkingWithList=false;
	
	return 0;
}

int AddDir(char *szDirName) {
	DWORD dwThreadId;
	DWORD dwExitCode;

	scandirpars.pl=&list;
	lstrcpy(scandirpars.szDirName,szDirName);
	
	if(hDirThread) {
		GetExitCodeThread(hDirThread,&dwExitCode);
		if(dwExitCode!=STILL_ACTIVE) {
			CloseHandle(hDirThread);
			hDirThread=NULL;
		}
		else {
			MessageBox(hWnd,"Processing thread is still active","MP3val-frontend",MB_OK|MB_ICONWARNING);
			return 0;
		}
	}
	
	hDirThread=CreateThread(NULL,0,ScanDirWrapper,&scandirpars,0,&dwThreadId);
	
	return 0;
}

int DoDropFiles(HDROP hDrop) {
	char buf[MAX_PATH+1];
	DWORD dwExitCode;
	int i,count;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode==STILL_ACTIVE) {
			MessageBox(hWnd,"Impossible during scan","MP3val-frontend",MB_OK|MB_ICONERROR);
			return 0;
		}
	}
	
	count=DragQueryFile(hDrop,0xFFFFFFFF,buf,MAX_PATH+1);
	for(i=0;i<=count-1;i++) {
		DragQueryFile(hDrop,i,buf,MAX_PATH+1);
		if(GetFileAttributes(buf)&FILE_ATTRIBUTE_DIRECTORY) AddDir(buf);
		else {
			list.addfile(buf);
			RefreshView(iViewMode);
		}
	}
	
	return 0;
}

int DoFileAddFile() {
	DWORD dwAttr;
	DWORD dwExitCode;
	char filename[MAX_PATH+1];
	char dirname[MAX_PATH+1];
	char *p;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode==STILL_ACTIVE) {
			MessageBox(hWnd,"Impossible during scan","MP3val-frontend",MB_OK|MB_ICONERROR);
			return 0;
		}
	}
	
	if(!GetOpenFileName(&ofn)) return 1;
	dwAttr=GetFileAttributes(ofn.lpstrFile);
	if(dwAttr&FILE_ATTRIBUTE_DIRECTORY) {
		strcpy(dirname,ofn.lpstrFile);
		strcat(dirname,"\\");
		p=ofn.lpstrFile;
		for(p=p+strlen(p)+1;*p;p=p+strlen(p)+1) {
			strcpy(filename,dirname);
			strcat(filename,p);
			list.addfile(filename);
		}
	}
	else {
		list.addfile(ofn.lpstrFile);
	}
	
	RefreshView(iViewMode);

	return 0;
}

int DoFileAddDir() {
	BROWSEINFO bi;
	LPCITEMIDLIST pidl;
	char tmpbuf[MAX_PATH+1];
	DWORD dwExitCode;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode==STILL_ACTIVE) {
			MessageBox(hWnd,"Impossible during scan","MP3val-frontend",MB_OK|MB_ICONERROR);
			return 0;
		}
	}

	bi.hwndOwner=hWnd;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=tmpbuf;
	bi.lpszTitle=szBrowseForFolderTitle;
	bi.ulFlags=BIF_EDITBOX;
	bi.lpfn=BrowseCallbackProc;
	bi.lParam=(LPARAM)0;

	OleInitialize(NULL);
	pidl=SHBrowseForFolder(&bi);
	if(!pidl) {
		OleUninitialize();
		return 0;
	}
	
	SHGetPathFromIDList(pidl,szDirBeingAdded);
	
	CoTaskMemFree((void *)pidl);
	
	OleUninitialize();
	
	AddDir(szDirBeingAdded);
		
	return 0;
}

int DoFileQuit() {
	DestroyWindow(hWnd);
	return 0;
}

int DoActionsRemove(bool selection) {
	int i=-1;
	int oldi=-1;
	
	if(!selection) {
		ListView_DeleteAllItems(hListView);
		list.cleanup();
		return 0;
	}
	
	bWorkingWithList=true;
	
	for(;;) {
		i=SendMessage(hListView,LVM_GETNEXTITEM,(WPARAM)i,(LPARAM)MAKELPARAM(LVNI_SELECTED,0));
		if(i==-1) break;
		list.deletefileno(iViewMode,i);
		ListView_DeleteItem(hListView,i);
		oldi=i;
		i--;
	}
	if(oldi<=ListView_GetItemCount(hListView)-1&&oldi>=0) ListView_SetItemState(hListView,oldi,LVIS_SELECTED,LVIS_SELECTED);
	
	bWorkingWithList=false;

	return 0;
}

int DoActionsScan(bool selection,bool fix) {
	DWORD dwThreadId;
	DWORD dwExitCode;
	
	bClicked=false;
	
	pars.hListView=hListView;
	pars.plist=&list;
	pars.selected=selection;
	pars.fix=fix;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode!=STILL_ACTIVE) {
			CloseHandle(hThread);
			hThread=NULL;
		}
		else {
			MessageBox(hWnd,"Processing thread is still active","MP3val-frontend",MB_OK|MB_ICONWARNING);
			return 0;
		}
	}
	
	hThread=CreateThread(NULL,0,ScanListWrapper,&pars,0,&dwThreadId);
	return 0;
}

int DoActionsStopScan() {
	int res;
	DWORD dwExitCode;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode!=STILL_ACTIVE) {
			CloseHandle(hThread);
			hThread=NULL;
		}
	}
	
	if(hThread) {
		res=MessageBox(hWnd,"Abort operation?","MP3val-frontend",MB_YESNO|MB_ICONWARNING);
		if(res!=IDYES) return 0;
		TerminateThread(hThread,1);
		CloseHandle(hThread);
		MySpawner.DespawnProcess();
		PostMessage(hProgress,PBM_SETPOS,(WPARAM)0,0);
		MessageBox(hWnd,"Processing thread has been terminated by user","MP3val-frontend",MB_OK|MB_ICONERROR);
	}
	else MessageBox(hWnd,"Nothing to abort","MP3val-frontend",MB_OK|MB_ICONERROR);
	return 0;
}

int DoActionsOptions() {
	DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_OPTIONS),hWnd,OptionsDlgProc);
	return 0;
}

int DoViewSetMode(int mode) {
	int i;
	DWORD dwExitCode;
	
	if(hThread) {
		GetExitCodeThread(hThread,&dwExitCode);
		if(dwExitCode==STILL_ACTIVE) {
			MessageBox(hWnd,"Impossible during scan","MP3val-frontend",MB_OK|MB_ICONERROR);
			return 0;
		}
	}
	
	iViewMode=mode;
	for(i=0;i<=VM_COUNT-1;i++) {
		if(i==iViewMode) CheckMenuItem(hViewMenu,i,MF_BYPOSITION|MF_CHECKED);
		else CheckMenuItem(hViewMenu,i,MF_BYPOSITION|MF_UNCHECKED);
	}
	RefreshView(iViewMode);
	return 0;
}

int DoViewFollowScan() {
	if(bClicked) {
		bClicked=false;
		CheckMenuItem(hViewMenu,IDM_VIEW_FOLLOWS_SCAN,MF_BYCOMMAND|MF_CHECKED);
	}
	else {
		bClicked=true;
		CheckMenuItem(hViewMenu,IDM_VIEW_FOLLOWS_SCAN,MF_BYCOMMAND|MF_UNCHECKED);
	}
	
	return 0;
}

int DoHelpAbout() {
	MessageBox(hWnd,"MP3val-frontend 0.1.1.\nCopyright (c) Alexey Kuznetsov, 2007\nThis is a BETA release.","About MP3val-frontend",MB_OK|MB_ICONINFORMATION);
	return 0;
}

int HandleListViewRClick(LPNMITEMACTIVATE pnmact){
	HandleSelectionChange(-1);
	ClientToScreen(hListView,&(pnmact->ptAction));
    TrackPopupMenu(hPopup,
            TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
            pnmact->ptAction.x,
            pnmact->ptAction.y,
            0,
            hWnd,
            NULL);
	
	return 0;
}

int HandleSelectionChange(int item) {
	int i;
	FileInfo fi;
	CStringNode *current;
	static int prev=-1;
	
	if(bWorkingWithList) return 0;
	
	if(item==-1) {
		i=SendMessage(hListView,LVM_GETNEXTITEM,(WPARAM)-1,(LPARAM)MAKELPARAM(LVNI_SELECTED,0));
		if(i==-1) return 0;
	}
	else i=item;
	
	if(i==prev&&!SendMessage(hEdit,EM_GETMODIFY,(WPARAM)0,(WPARAM)0)) return 0;
	
	if(!(list.getfileno(iViewMode,i,&fi))) return 0;
	
	SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"");
	
	for(current=fi.proot->next;current;current=current->next) {
		if(current->str) {
			SendMessage(hEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)current->str);
			SendMessage(hEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"\x0D\x0A");
		}
	}
	
	if(!(fi.proot->next)) SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"Not scanned yet!");
	
	SendMessage(hEdit,EM_SETMODIFY,(WPARAM)FALSE,(WPARAM)0);
	
	return 0;
}

int CALLBACK BrowseCallbackProc(HWND hWnd,UINT message,LPARAM lParam,LPARAM lpData) {
	if(message==BFFM_INITIALIZED&&szDirBeingAdded[0]) {
		SendMessage(hWnd,BFFM_SETSELECTION,(WPARAM)TRUE,(LPARAM)szDirBeingAdded);
	}
	
	return 0;
}

BOOL CALLBACK OptionsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) {
	switch(uMsg) {
	case WM_INITDIALOG:
		SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
		SendMessage(GetDlgItem(hDlg,IDC_DELETEBAKS),BM_SETCHECK,(int)options.bDeleteBaks,0);
		SendMessage(GetDlgItem(hDlg,IDC_KEEPTIMESTAMPS),BM_SETCHECK,(int)options.bKeepTimestamps,0);
		SendMessage(GetDlgItem(hDlg,IDC_IGNORETAGS),BM_SETCHECK,(int)options.bIgnoreMissingTags,0);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_DELETEBAKS:
		case IDC_KEEPTIMESTAMPS:
		case IDC_IGNORETAGS:
			return TRUE;
		case IDOK:
			if(SendMessage(GetDlgItem(hDlg,IDC_DELETEBAKS),BM_GETCHECK,0,0)) options.bDeleteBaks=true;
			else options.bDeleteBaks=false;
			
			if(SendMessage(GetDlgItem(hDlg,IDC_KEEPTIMESTAMPS),BM_GETCHECK,0,0)) options.bKeepTimestamps=true;
			else options.bKeepTimestamps=false;
			
			if(SendMessage(GetDlgItem(hDlg,IDC_IGNORETAGS),BM_GETCHECK,0,0)) options.bIgnoreMissingTags=true;
			else options.bIgnoreMissingTags=false;
			
			options.write();
			
			EndDialog(hDlg,0);
			
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg,0);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
