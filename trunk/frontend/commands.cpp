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

char szBrowseForFolderTitle[]="Choose a folder";

char szStateNotApplicable[]="N/A";
char szStateNotScanned[]="NOT SCANNED";
char szStateOK[]="OK";
char szStateProblem[]="PROBLEM";
char szStateFixed[]="FIXED";

char *szStates[5]={szStateNotApplicable,szStateNotScanned,szStateOK,szStateProblem,szStateFixed};

extern HWND hWnd,hListView,hEdit;
extern HMENU hViewMenu,hPopup;
extern bool bClicked;
extern CSpawner MySpawner;
ScanListPars pars;
ScanDirPars scandirpars;

CFileList list;
bool bWorkingWithList=false;

char szOpenFileName[OPENFILENAME_BUFSIZE];
OPENFILENAME ofn;

int iViewMode=VM_EVERYTHING;

HANDLE hThread=NULL;
HANDLE hDirThread=NULL;

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
	int i,count;
	
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
	char filename[MAX_PATH+1];
	char dirname[MAX_PATH+1];
	char *p;
	
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
	char buf[MAX_PATH+1];
	BROWSEINFO bi;
	LPCITEMIDLIST pidl;

	bi.hwndOwner=hWnd;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=buf;
	bi.lpszTitle=szBrowseForFolderTitle;
	bi.ulFlags=BIF_EDITBOX;
	bi.lpfn=NULL;
	bi.lParam=(LPARAM)0;

	OleInitialize(NULL);
	pidl=SHBrowseForFolder(&bi);
	if(!pidl) {
		OleUninitialize();
		return 0;
	}
	
	SHGetPathFromIDList(pidl,buf);
	
	CoTaskMemFree((void *)pidl);
	
	OleUninitialize();
	
	AddDir(buf);
		
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
	
	SendMessage(hEdit,WM_SETTEXT,(WPARAM)0,(LPARAM)"Scanning in progress...");
	
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
	
//	ScanList(hListView,&list,selection);
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
		MessageBox(hWnd,"Processing thread has been terminated by user","MP3val-frontend",MB_OK|MB_ICONERROR);
	}
	else MessageBox(hWnd,"Nothing to abort","MP3val-frontend",MB_OK|MB_ICONERROR);
	return 0;
}

int DoActionsOptions() {
	MessageBox(hWnd,"Options dialog will be here","MP3val-frontend",MB_OK);
	return 0;
}

int DoViewSetMode(int mode) {
	int i;
	
	iViewMode=mode;
	for(i=0;i<=VM_COUNT-1;i++) {
		if(i==iViewMode) CheckMenuItem(hViewMenu,i,MF_BYPOSITION|MF_CHECKED);
		else CheckMenuItem(hViewMenu,i,MF_BYPOSITION|MF_UNCHECKED);
	}
	RefreshView(iViewMode);
	return 0;
}

int DoHelpAbout() {
	MessageBox(hWnd,"MP3val-frontend 0.1.0+ (alpha), (c) Alexey Kuznetsov, 2007\nThis program is in an early development stage. Some important features can be missing. There may be some bugs.","About MP3val-frontend",MB_OK|MB_ICONINFORMATION);
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
