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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#define OPENFILENAME_BUFSIZE 65536

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
int DoViewFollowScan();

int DoHelpAbout();

int HandleListViewRClick(LPNMITEMACTIVATE pnmact);
int HandleSelectionChange(int item);

int CALLBACK BrowseCallbackProc(HWND hWnd,UINT message,LPARAM lParam,LPARAM lpData);

#endif
