#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#define OPENFILENAME_BUFSIZE 65536

#define _WIN32_IE 0x0400

#include "base.h"
#include <windows.h>
#include <commctrl.h>

int InitCommands();

int RefreshView(int iViewType);

int DoFileAddFile();
int DoFileAddDir();
int DoFileQuit();

int DoActionsRemove();
int DoActionsScanAll();
int DoActionsScanSel();
int DoActionsFixSel();
int DoActionsOptions();

int DoHelpAbout();

int HandleListViewRClick(LPNMITEMACTIVATE pnmact);

#endif
