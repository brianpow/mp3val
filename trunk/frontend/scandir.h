#ifndef __SCANDIR_H__
#define __SCANDIR_H__

#include "listman.h"

struct ScanDirPars {
	CFileList *pl;
	char szDirName[MAX_PATH+1];
};

DWORD WINAPI ScanDirWrapper(LPVOID p);
int ScanDirForFiles(CFileList *pl,char *szDirName);

#endif
