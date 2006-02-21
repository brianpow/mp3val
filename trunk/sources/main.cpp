/*
 * MP3val - a program for MPEG audio file validation
 * Copyright (C) 2005-2006 Alexey Kuznetsov (ring0) and Eugen Tikhonov (jetsys)
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

#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

#include <windows.h>
#include "mpegparse.h"
#include "report.h"

HANDLE hFile;
HANDLE hFileMapping;

char pcBuffer[MAX_PATH+1];
char pcDirBuffer[MAX_PATH+1];

bool FixErrors=false;

LPVOID MapFile(char *filename);
int UnmapFile(LPVOID);
int SplitFileName(char *szFileNameIn,char **szPathOut,char **szFileNameOut);

int main(int argc, char *argv[]) {
	MPEGINFO mpginfo;
	unsigned char *pImage;
	ofstream log_out;
	HANDLE hData,hFind;
	ostream *out;
	char *p;
	bool FixErrors=false;
	char *szLogFile=NULL;
	int i;
	WIN32_FIND_DATA wfd;
	
	char *szFile,*szPath;
	
	char szStartDir[MAX_PATH+2];

	if(argc<2) {
		cerr<<"MP3val - a program for MPEG audio stream validation\n";
		cerr<<"Version 0.1.1\n";
		cerr<<"Usage: "<<argv[0]<<" <file to validate> [-l<log file>] [-f]\n";
		cerr<<"-f - try to fix errors\n";
		cerr<<"Wildcards are allowed.\n";
		cerr<<"If log file isn't specified, stdout will be used.\n\n";
		cerr<<"(c) ring0, jetsys, 2005-2006.\n";
		cerr<<"This program is released under GPL, see the attached file for details.\n";
		return -1;
	}
	
	for(i=1;i<argc;i++) {
		if((strlen(argv[i])>=2)&&(!memcmp(argv[i],"-l",2))) {
			szLogFile=&argv[i][2];
		}
		else if((strlen(argv[i])>=2)&&(!memcmp(argv[i],"-f",2))) {
			FixErrors=true;
		}
		else if(argv[i][0]=='\"') {
			cerr<<"Wrong parameter \""<<argv[i]<<"\"\n";
			return -1;
		}
	}
	
	for(i=1;i<argc;i++) {
		if(argv[i][0]=='-') continue;
		
		SplitFileName(argv[i],&szPath,&szFile);
	
		if(szPath) {
			GetCurrentDirectory(MAX_PATH,szStartDir);
			SetCurrentDirectory(szPath);
		}

		hFind=FindFirstFile(szFile,&wfd);
		if(hFind==INVALID_HANDLE_VALUE) {
			cerr<<"Cannot open input file "<<szFile<<'\n';
			continue;
		}
		
		do {
			if(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;
			
			pImage=(unsigned char *)MapFile(wfd.cFileName);
	
			if(!pImage) {
				cerr<<"Cannot open input file "<<wfd.cFileName<<'\n';
				continue;
			}
			
			if(szPath) SetCurrentDirectory(szStartDir);
			
			if(szLogFile) {
				log_out.open(szLogFile,ios::out|ios::app|ios::binary);
				if(!log_out) {
					cerr<<"Cannot open log file\n";
					UnmapFile(pImage);
					continue;
				}
				out=&log_out;
			}
			else {
				out=&cout;
			}
			
			if(szPath) SetCurrentDirectory(szPath);
	
			cout<<"Analyzing file \""<<wfd.cFileName<<"\"...\n";
			
			if(GetFullPathName(wfd.cFileName,MAX_PATH+1,(char *)pcBuffer,&p)) ValidateFile(pImage,GetFileSize(hFile,NULL),&mpginfo,out,pcBuffer,false,NULL);
			else ValidateFile(pImage,GetFileSize(hFile,NULL),&mpginfo,out,wfd.cFileName,false,NULL);
	
			if(GetFullPathName(wfd.cFileName,MAX_PATH+1,(char *)pcBuffer,&p)) PrintReport(out,pcBuffer,&mpginfo);
			else PrintReport(out,wfd.cFileName,&mpginfo);
	
			if(FixErrors&&mpginfo.iErrors) {
				GetTempPath(MAX_PATH,(char *)pcDirBuffer);
				GetTempFileName((char *)pcDirBuffer,"mp3",0,(char *)pcBuffer);
				hData=CreateFile((char *)pcBuffer,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
				if(hData==INVALID_HANDLE_VALUE) {
					cerr<<"Cannot open temporary file\n";
					UnmapFile(pImage);
					log_out.close();
					continue;
				}
	
				cout<<"Rebuilding file \""<<wfd.cFileName<<"\"...\n";
	
				ValidateFile(pImage,GetFileSize(hFile,NULL),&mpginfo,NULL,NULL,true,hData);
	
				CloseHandle(hData);
			}
	
			UnmapFile(pImage);
	
			if(FixErrors&&mpginfo.iErrors) {
				strcpy((char *)pcDirBuffer,wfd.cFileName);
				strcat((char *)pcDirBuffer,".bak");
				if(!MoveFile(wfd.cFileName,(char *)pcDirBuffer)) {
					cerr<<"Error renaming \""<<wfd.cFileName<<"\"\n";
					UnmapFile(pImage);
					log_out.close();
					continue;
				}
				if(!MoveFile(pcBuffer,wfd.cFileName)) {
					cerr<<"Error renaming temporary file\n";
					UnmapFile(pImage);
					log_out.close();
					continue;
				}
	
			}
	
			if(FixErrors&&mpginfo.iErrors) {
				if(GetFullPathName(wfd.cFileName,MAX_PATH+1,(char *)pcBuffer,&p)) PrintMessage(out,"FIXED",pcBuffer,"File was rebuilt",-1);
				else PrintMessage(out,"FIXED",wfd.cFileName,"File was rebuilt",-1);
			}
	
			if(szLogFile) log_out.close();
	
			cout<<"Done!\n";
		}while(FindNextFile(hFind,&wfd));
	
		FindClose(hFind);
	}

	return 0;
}

LPVOID MapFile(char *filename) {
	LPVOID pImage;

	hFile=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile==INVALID_HANDLE_VALUE) {
		return NULL;
	}

	hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);

	pImage=MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);

	return pImage;
}

int UnmapFile(LPVOID pImage) {
	UnmapViewOfFile(pImage);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);

	return 0;
}

int SplitFileName(char *szFileNameIn,char **szPathOut,char **szFileNameOut) {
	int i;
	char *p;
	
	if(!szFileNameIn||!*szFileNameIn) return 1;
	
	for(p=szFileNameIn+(strlen(szFileNameIn)-1);(p>=szFileNameIn)&&(*p!='\\')&&(*p!='/');p--);
	p++;
	
	if(p==szFileNameIn) *szPathOut=NULL;
	else {
		*szPathOut=szFileNameIn;
		if(**szPathOut=='\"') *szPathOut++;
		*(p-1)='\0';
	}
	
	if(p==szFileNameIn) *szFileNameOut=szFileNameIn;
	else *szFileNameOut=p;
	
	if(**szFileNameOut=='\"') {
		*szFileNameOut++;
		if(*szFileNameOut[strlen(*szFileNameOut)-1]=='\"') *szFileNameOut[strlen(*szFileNameOut)-1]='\0';
	}
	
	return 0;
}
