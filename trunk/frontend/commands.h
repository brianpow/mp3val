#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#define OPENFILENAME_BUFSIZE 65536

#define _WIN32_IE 0x0400

#include "base.h"
#include <windows.h>
#include <commctrl.h>

int InitCommands();

int RefreshView(int iViewType);
int AddDir(char *szDirName);

int DoDropFiles(HDROP hDrop);

int DoFileAddFile();
int DoFileAddDir();
int DoFileQuit();

int DoActionsRemove(bool selection);
int DoActionsScan(bool selection,bool fix);
int DoActionsStopScan();
int DoActionsOptions();

int DoViewSetMode(int mode);

int DoHelpAbout();

int HandleListViewRClick(LPNMITEMACTIVATE pnmact);
int HandleSelectionChange(int item);

int CALLBACK BrowseCallbackProc(HWND hWnd,UINT message,LPARAM lParam,LPARAM lpData);

#endif
