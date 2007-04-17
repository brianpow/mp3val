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

#include <windows.h>

#include "spawn.h"

int CSpawner::SpawnProcess(char *szCmdLine) {
	BOOL res;
	SECURITY_ATTRIBUTES pipe_attr;
	STARTUPINFO si;
	
	pipe_attr.nLength=sizeof(SECURITY_ATTRIBUTES);
	pipe_attr.lpSecurityDescriptor=NULL;
	pipe_attr.bInheritHandle=TRUE;
	
	res=CreatePipe(&hStdinRead,&hStdinWrite,&pipe_attr,0);
	if(!res) return -1;
	res=CreatePipe(&hStdoutRead,&hStdoutWrite,&pipe_attr,0);
	if(!res) return -1;
	
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb=sizeof(STARTUPINFO);
	si.dwFlags=STARTF_USESTDHANDLES;
	si.hStdInput=hStdinRead;
	si.hStdOutput=hStdoutWrite;
	si.hStdError=hStdoutWrite;
	
	res=CreateProcess(NULL,szCmdLine,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	if(!res) return -1;
	
	created=true;
	
	return 0;
}

int CSpawner::DespawnProcess() {
	TerminateProcess(pi.hProcess,0);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	
	CloseHandle(hStdinRead);
	CloseHandle(hStdinWrite);
	CloseHandle(hStdoutRead);
	CloseHandle(hStdoutWrite);
	
	created=false;
	
	return 0;
}

int CSpawner::writestr(char *szStr) {
	char *p;
	int iRest;
	DWORD dwBytesWritten;
	
	p=szStr;
	iRest=lstrlen(szStr);
	while(iRest) {
		WriteFile(hStdinWrite,p,iRest,&dwBytesWritten,NULL);
		p+=dwBytesWritten;
		iRest-=dwBytesWritten;
	}
	WriteFile(hStdinWrite,"\x0A",1,&dwBytesWritten,NULL);
	
	return 0;
}

int CSpawner::readstr(char *buf,int iBufSize) {
	int i;
	DWORD dwBytesRead;
	
	for(i=0;i<=iBufSize-1;i++) {
		ReadFile(hStdoutRead,&buf[i],1,&dwBytesRead,NULL);
		if(buf[i]=='\x0D'||buf[i]=='\x0A'||buf[i]=='\0') {
			if(i) {
				buf[i]='\x0';
				break;
			}
			else i--; 
		}
		else if(!dwBytesRead) break;
	}
	
	return 0;
}
