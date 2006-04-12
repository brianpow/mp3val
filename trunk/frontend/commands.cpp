#include <windows.h>
#include <commctrl.h>

#include "commands.h"
#include "listman.h"

extern HWND hWnd,hListView,hEdit;
CFileList list;

char szOpenFileName[OPENFILENAME_BUFSIZE];
OPENFILENAME ofn;

int InitCommands() {
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
	
	return 0;
}

int RefreshView(int iViewType) {
	return 0;
}

int DoFileAddFile() {
	DWORD dwAttr;
	
	if(!GetOpenFileName(&ofn)) return 1;
	MessageBox(hWnd,ofn.lpstrFile,"ofn.lpstrFile",MB_OK);
	dwAttr=GetFileAttributes(ofn.lpstrFile);
	if(dwAttr&FILE_ATTRIBUTE_DIRECTORY) {
		MessageBox(hWnd,"Multiple files selected","Info",MB_OK);
	}
	else {
		MessageBox(hWnd,"Single file selected. Adding...","Info",MB_OK);
		list.addfile(ofn.lpstrFile);
	}
	return 0;
}

int DoFileAddDir() {
	return 0;
}

int DoFileQuit() {
	DestroyWindow(hWnd);
	return 0;
}

int DoActionsScanAll() {
	return 0;
}

int DoActionsScanSel() {
	return 0;
}

int DoActionsFixSel() {
	return 0;
}

int DoActionsOptions() {
	return 0;
}

int DoHelpAbout() {
	MessageBox(hWnd,"MP3valGUI 0.0.0+ (not for public release), (c) ring0, 2006","About MP3valGUI",MB_OK|MB_ICONINFORMATION);
	return 0;
}
