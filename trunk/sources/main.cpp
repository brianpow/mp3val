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

#include "crossapi.h"
#include "mpegparse.h"
#include "report.h"

char pcBuffer[CROSSAPI_MAX_PATH+1];
char pcBuffer2[CROSSAPI_MAX_PATH+1];

bool FixErrors=false;
bool bSuppressInfo=false;
bool bPipeMode=false;

extern int iMappingLength;

int SplitFileName(char *szFileNameIn,char **szPathOut,char **szFileNameOut);

int ProcessFile(char *szFileName,char *szLogFileName);

int main(int argc, char *argv[]) {
	int hFind;
	int i;
	CROSSAPI_FIND_DATA cfd;
	bool help=false;
	
	char *szFile,*szPath;
	
	char *szLogFile=NULL;
	char szFullLogFile[CROSSAPI_MAX_PATH+2];
	char szStartDir[CROSSAPI_MAX_PATH+2];
	
	char szPipedFileName[CROSSAPI_MAX_PATH+2];
	char ch;
	
	if(argc<2) help=true;
	if(argc==2) {
		if(!strcmp(argv[1],"/?")) help=true;
		else if(!strcmp(argv[1],"-h")) help=true;
		else if(!strcmp(argv[1],"--help")) help=true;
	}

	if(help) {
		cerr<<"MP3val - a program for MPEG audio stream validation\n";
		cerr<<"Version 0.1.2+ (not for public release)\n";
		cerr<<"Usage: "<<argv[0]<<" <files to validate> [options]\n\n";
		cerr<<"Options:\n\n";
		cerr<<"\t-f                    try to fix errors\n";
		cerr<<"\t-l<file name>         write log to the specified file (default: stdout)\n";
		cerr<<"\t-si                   suppress INFO messages\n";
		cerr<<"\t-p                    pipe mode (receive input file names from stdin)\n";
		cerr<<"\n";
		cerr<<"Wildcards are allowed.\n\n";
		cerr<<"(c) ring0, jetsys, 2005-2006.\n";
		cerr<<"This program is released under GPL, see the attached file for details.\n";
		return 0;
	}
	
	for(i=1;i<argc;i++) {
		if(argv[i][0]!='-') continue;
		if((strlen(argv[i])>=2)&&(!memcmp(argv[i],"-l",2))) {
			szLogFile=&argv[i][2];
		}
		else if(!strcmp(argv[i],"-f")) {
			FixErrors=true;
		}
		else if(!strcmp(argv[i],"-si")) {
			bSuppressInfo=true;
		}
		else if(!strcmp(argv[i],"-p")) {
			bPipeMode=true;
		}
		else {
			cerr<<"Wrong parameter \""<<argv[i]<<"\"\n";
			return -1;
		}
	}
	
	if(szLogFile) CrossAPI_GetFullPathName(szLogFile,szFullLogFile,CROSSAPI_MAX_PATH+2);
	
	if(bPipeMode) {
		i=0;
		for(;;) {
			if(i>CROSSAPI_MAX_PATH) i=0;
			cin.get(ch);
			if(!ch) break;
			if(ch==0x0D||ch==0x0A) {
				szPipedFileName[i]='\0';
				if(*szPipedFileName) ProcessFile(szPipedFileName,szLogFile?szFullLogFile:NULL);
				i=0;
			}
			else {
				szPipedFileName[i]=ch;
				i++;
			}
		}
	}
	else {
		for(i=1;i<argc;i++) {
			if(argv[i][0]=='-') continue;
			
			SplitFileName(argv[i],&szPath,&szFile);
		
			if(szPath) {
				CrossAPI_GetCurrentDirectory(CROSSAPI_MAX_PATH,szStartDir);
				CrossAPI_SetCurrentDirectory(szPath);
			}
	
			hFind=CrossAPI_FindFirstFile(szFile,&cfd);
			if(hFind==-1) {
				cerr<<"Cannot open input file "<<szFile<<'\n';
				continue;
			}
			
			do {
				if(cfd.bIsDirectory) continue;
				ProcessFile(cfd.cFileName,szLogFile?szFullLogFile:NULL);
			}while(CrossAPI_FindNextFile(hFind,&cfd));
		
			CrossAPI_FindClose(hFind);
			
			if(szPath) CrossAPI_SetCurrentDirectory(szStartDir);
		}
	}

	return 0;
}

int ProcessFile(char *szFileName,char *szLogFileName) {
	MPEGINFO mpginfo;
	unsigned char *pImage;
	ofstream log_out;
	int hData;
	ostream *out;
	
	pImage=(unsigned char *)CrossAPI_MapFile(szFileName);

	if(!pImage) {
		cerr<<"Cannot open input file \""<<szFileName<<"\" or it is empty\n";
		return 0;
	}
	
	if(szLogFileName) {
		log_out.open(szLogFileName,ios::out|ios::app|ios::binary);
		if(!log_out) {
			cerr<<"Cannot open log file\n";
			CrossAPI_UnmapFile(pImage);
			return 0;
		}
		out=&log_out;
	}
	else {
		out=&cout;
	}
	
	cout<<"Analyzing file \""<<szFileName<<"\"...\n";
	
	if(CrossAPI_GetFullPathName(szFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) ValidateFile(pImage,iMappingLength,&mpginfo,out,pcBuffer,false,-1);
	else ValidateFile(pImage,iMappingLength,&mpginfo,out,szFileName,false,-1);

	if(CrossAPI_GetFullPathName(szFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) PrintReport(out,pcBuffer,&mpginfo);
	else PrintReport(out,szFileName,&mpginfo);

	if(FixErrors&&mpginfo.iErrors) {
		hData=CrossAPI_GetTempFileAndName(CROSSAPI_MAX_PATH,pcBuffer);
		if(hData==-1) {
			cerr<<"Cannot open temporary file\n";
			CrossAPI_UnmapFile(pImage);
			log_out.close();
			return 0;
		}

		cout<<"Rebuilding file \""<<szFileName<<"\"...\n";

		ValidateFile(pImage,iMappingLength,&mpginfo,NULL,NULL,true,(int)hData);

		CrossAPI_CloseFile(hData);
	}

	CrossAPI_UnmapFile(pImage);

	if(FixErrors&&mpginfo.iErrors) {
		strcpy((char *)pcBuffer2,szFileName);
		strcat((char *)pcBuffer2,".bak");
		if(!CrossAPI_MoveFile((char *)pcBuffer2,szFileName)) {
			cerr<<"Error renaming \""<<szFileName<<"\"\n";
			CrossAPI_UnmapFile(pImage);
			log_out.close();
			return 0;
		}
		if(!CrossAPI_MoveFile(szFileName,pcBuffer)) {
			cerr<<"Error renaming temporary file\n";
			CrossAPI_UnmapFile(pImage);
			log_out.close();
			return 0;
		}

	}

	if(FixErrors&&mpginfo.iErrors) {
		if(CrossAPI_GetFullPathName(szFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) PrintMessage(out,"FIXED",pcBuffer,"File was rebuilt",-1);
		else PrintMessage(out,"FIXED",szFileName,"File was rebuilt",-1);
	}

	if(szLogFileName) log_out.close();

	cout<<"Done!\n";
	
	return 0;
}


int SplitFileName(char *szFileNameIn,char **szPathOut,char **szFileNameOut) {
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
