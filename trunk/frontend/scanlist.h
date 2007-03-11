#ifndef __SCANLIST_H__
#define __SCANLIST_H__

#include <windows.h>
#include "listman.h"
#include "spawn.h"

struct ScanListPars {
	HWND hListView;
	CFileList *plist;
	bool selected;
	bool fix;
};

DWORD WINAPI ScanListWrapper(LPVOID p);
int ScanList(HWND hListView, CFileList* plist, bool selected, bool fix);
int ScanFile(int i,CFileList *plist,CSpawner *pSpawner, bool fix);

#endif
