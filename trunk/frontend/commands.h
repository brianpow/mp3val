#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#define OPENFILENAME_BUFSIZE 65536

int InitCommands();

int RefreshView(int iViewType);

int DoFileAddFile();
int DoFileAddDir();
int DoFileQuit();

int DoActionsScanAll();
int DoActionsScanSel();
int DoActionsFixSel();
int DoActionsOptions();

int DoHelpAbout();

#endif
