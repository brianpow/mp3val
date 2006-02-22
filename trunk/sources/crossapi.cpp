#include "crossapi.h"

int hFile;
int hFileMapping;
int iMappingLength;

#if (defined WIN32)||(defined __WIN32__)

#include <windows.h>

int CrossAPI_GetCurrentDirectory(int iBufSize,char *pcBuffer) {
	return GetCurrentDirectory(iBufSize,pcBuffer);
}

int CrossAPI_SetCurrentDirectory(char *pcBuffer) {
	return SetCurrentDirectory(pcBuffer);
}

int CrossAPI_FindFirstFile(char *szFileName,CROSSAPI_FIND_DATA *cfd) {
	WIN32_FIND_DATA wfd;
	HANDLE res;
	res=FindFirstFile(szFileName,&wfd);
	strcpy(cfd->cFileName,wfd.cFileName);
	if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) cfd->bIsDirectory=true;
	else cfd->bIsDirectory=false;
	return (int)res;
}

int CrossAPI_FindNextFile(int iHandle,CROSSAPI_FIND_DATA *cfd) {
	WIN32_FIND_DATA wfd;
	BOOL res;
	res=FindNextFile((HANDLE)iHandle,&wfd);
	if(!res) return 0;
	strcpy(cfd->cFileName,wfd.cFileName);
	if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) cfd->bIsDirectory=true;
	else cfd->bIsDirectory=false;
	return 1;
}

int CrossAPI_FindClose(int iHandle) {
	FindClose((HANDLE)iHandle);
	return 0;
}

int CrossAPI_GetFullPathName(char *szFileName,char *pcBuffer,int iBufSize) {
	char *p;
	return GetFullPathName(szFileName,iBufSize,pcBuffer,&p);
}

int CrossAPI_GetTempFileAndName(int iBufSize,char *pcBuffer) {
	char pcDirBuffer[CROSSAPI_MAX_PATH];
	if(iBufSize<CROSSAPI_MAX_PATH) return -1;
	GetTempPath(CROSSAPI_MAX_PATH,(char *)pcDirBuffer);
	GetTempFileName((char *)pcDirBuffer,"mp3",0,(char *)pcBuffer);
      return (int)CreateFile((char *)pcBuffer,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
}

int CrossAPI_MoveFile(char *szNewName,char *szOldName) {
	DeleteFile(szNewName);
	return MoveFile(szOldName,szNewName);
}

int CrossAPI_OpenFile(char *szFileName,bool create,bool write) {
	return (int)CreateFile(szFileName,write?GENERIC_WRITE:GENERIC_READ,0,NULL,create?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
}

int CrossAPI_SetFilePointer(int iHandle,int iPointer,bool bFromCurrent) {
	if(iHandle==-1) return -1;
	return SetFilePointer((HANDLE)iHandle,iPointer,NULL,bFromCurrent?FILE_CURRENT:FILE_BEGIN);
}

int CrossAPI_WriteFile(int iHandle,char *pcBuffer,int iBytes,int *iBytesWritten) {
	if(iHandle==-1) return -1;
	return WriteFile((HANDLE)iHandle,pcBuffer,iBytes,(DWORD*)iBytesWritten,NULL);
}

int CrossAPI_CloseFile(int iHandle) {
	return CloseHandle(HANDLE(iHandle));
}

int CrossAPI_GetFileSize(int iHandle) {
	return GetFileSize((HANDLE)iHandle,NULL);
}

int CrossAPI_SetEndOfFile(int iHandle) {
	return SetEndOfFile((HANDLE)iHandle);
}

void *CrossAPI_MapFile(char *filename) {
	void *pImage;

	hFile=(int)CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==-1) {
		return NULL;
	}

	hFileMapping=(int)CreateFileMapping((HANDLE)hFile,NULL,PAGE_READONLY,0,0,NULL);

	pImage=MapViewOfFile((HANDLE)hFileMapping,FILE_MAP_READ,0,0,0);
	
	iMappingLength=CrossAPI_GetFileSize(hFile);

	return pImage;
}

int CrossAPI_UnmapFile(void *pImage) {
	UnmapViewOfFile(pImage);
	CloseHandle((HANDLE)hFileMapping);
	CloseHandle((HANDLE)hFile);

	return 0;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define TMPBUFSIZE 8192

char pcTmpBuf[TMPBUFSIZE];

int iRoundedMappingLength;

int CrossAPI_GetCurrentDirectory(int iBufSize,char *pcBuffer) {
	char *res;
	res=getcwd(pcBuffer,iBufSize);
	if(!res) return 0;
	return (res-pcBuffer);
}

int CrossAPI_SetCurrentDirectory(char *pcBuffer) {
	int res;
	res=chdir(pcBuffer);
	if(!res) return 1;
	return 0;
}

int CrossAPI_FindFirstFile(char *szFileName,CROSSAPI_FIND_DATA *cfd) {
	struct stat inode;
	stat(szFileName,&inode);
	strncpy(cfd->cFileName,szFileName,CROSSAPI_MAX_PATH);
	if(inode.st_mode&S_IFDIR) cfd->bIsDirectory=true;
	else cfd->bIsDirectory=false;
	return 1;
}

int CrossAPI_FindNextFile(int iHandle,CROSSAPI_FIND_DATA *cfd) {
	return 0;
}

int CrossAPI_FindClose(int iHandle) {
	return 0;
}

int CrossAPI_GetFullPathName(char *szFileName,char *pcBuffer,int iBufSize) {
	if(iBufSize<CROSSAPI_MAX_PATH) return 0;
	return (int)realpath(szFileName,pcBuffer);
}

int CrossAPI_GetTempFileAndName(int iBufSize,char *pcBuffer) {
	if(iBufSize<CROSSAPI_MAX_PATH) return -1;
	strcpy(pcBuffer,"/tmp/mp3val.XXXXXX");
	return mkstemp(pcBuffer);
}

int CrossAPI_MoveFile(char *szNewName,char *szOldName) {
	int res;
	int iBytesRead,iBytesWritten,iTotalBytesWritten;
	int id,od;
	struct stat mystat;
	
	unlink(szNewName);

	res=rename(szOldName,szNewName);
	if(!res) return 1;
//Moving failed. Check reason:
	if(errno!=EXDEV) return 0;
//Moving failed due to different logical drives of source and destination. Let's copy:
	id=open(szOldName,O_RDONLY);
	if(id==-1) return 0;
	od=open(szNewName,O_WRONLY|O_CREAT|O_TRUNC);
	if(od==-1) {
		close(id);
		return 0;
	}
	while(true) {
		iBytesRead=read(id,pcTmpBuf,TMPBUFSIZE);
		if(!iBytesRead) break;
		iTotalBytesWritten=0;
		while(iBytesRead) {
			iBytesWritten=write(od,pcTmpBuf+iTotalBytesWritten,iBytesRead);
			iBytesRead-=iBytesWritten;
			iTotalBytesWritten+=iBytesWritten;
		}
	}
	
	fstat(id,&mystat);
	fchmod(od,mystat.st_mode);
	
	close(id);
	close(od);
	unlink(szOldName);
	
	return 1;
}

int CrossAPI_OpenFile(char *szFileName,bool create,bool write) {
	int flags=0;
	if(create) flags|=(O_CREAT|O_TRUNC);
	if(write) flags|=O_WRONLY;
	else flags|=O_RDONLY;
	return open(szFileName,flags);
}

int CrossAPI_SetFilePointer(int iHandle,int iPointer,bool bFromCurrent) {
	if(iHandle==-1) return -1;
	return lseek(iHandle,iPointer,(bFromCurrent?SEEK_CUR:SEEK_SET));
}

int CrossAPI_WriteFile(int iHandle,char *pcBuffer,int iBytes,int *iBytesWritten) {
	int res;
	if(iHandle==-1) return -1;
	res=write(iHandle,pcBuffer,iBytes);
	if(res==-1) {
		*iBytesWritten=0;
		return 0;
	}
	*iBytesWritten=res;
	return 1;
}

int CrossAPI_CloseFile(int iHandle) {
	return close(iHandle);
}

int CrossAPI_GetFileSize(int iHandle) {
	int curpos;
	int size;
	
	curpos=lseek(iHandle,0,SEEK_CUR);
	size=lseek(iHandle,0,SEEK_END);
	lseek(iHandle,curpos,SEEK_SET);
	
	return size;
}

int CrossAPI_SetEndOfFile(int iHandle) {
	return ftruncate(iHandle,lseek(iHandle,0,SEEK_CUR));
}

void *CrossAPI_MapFile(char *filename) {
	void *pImage;
	double dPages;

	hFile=open(filename,O_RDONLY);

	if(hFile==-1) {
		return NULL;
	}
	
	iMappingLength=CrossAPI_GetFileSize(hFile);
	dPages=iMappingLength/getpagesize();
	iRoundedMappingLength=((int)ceil(dPages)+1)*getpagesize();

	pImage=mmap(NULL,iRoundedMappingLength,PROT_READ,0,hFile,0);
	
	if(pImage==MAP_FAILED) return NULL;

	return pImage;
}

int CrossAPI_UnmapFile(void *pImage) {
	munmap(pImage,iRoundedMappingLength);
	close(hFile);
	return 0;
}

#endif
