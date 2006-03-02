#include <windows.h>
#include <commctrl.h>

#include "commands.h"

extern HWND hWnd,hListView,hEdit;
node root;

int InitCommands() {
	return 0;
}

int RefreshView(int iViewType) {
	return 0;
}

int DoFileAddFile() {
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
