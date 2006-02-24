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

extern int iMappingLength;

int SplitFileName(char *szFileNameIn,char **szPathOut,char **szFileNameOut);

int main(int argc, char *argv[]) {
	MPEGINFO mpginfo;
	unsigned char *pImage;
	ofstream log_out;
	unsigned int hData,hFind;
	ostream *out;
	char *p;
	bool FixErrors=false;
	char *szLogFile=NULL;
	int i;
	CROSSAPI_FIND_DATA cfd;
	bool help=false;
	
	char *szFile,*szPath;
	
	char szStartDir[CROSSAPI_MAX_PATH+2];
	
	if(argc<2) help=true;
	if(argc==2) {
		if(!strcmp(argv[1],"/?")) help=true;
		else if(!strcmp(argv[1],"-h")) help=true;
		else if(!strcmp(argv[1],"--help")) help=true;
	}

	if(help) {
		cerr<<"MP3val - a program for MPEG audio stream validation\n";
		cerr<<"Version 0.1.2+ (not for public release)\n";
		cerr<<"Usage: "<<argv[0]<<" <files to validate> [-l<log file>] [-f]\n";
		cerr<<"-f - try to fix errors\n";
		cerr<<"Wildcards are allowed.\n";
		cerr<<"If log file isn't specified, stdout will be used.\n\n";
		cerr<<"(c) ring0, jetsys, 2005-2006.\n";
		cerr<<"This program is released under GPL, see the attached file for details.\n";
		return 0;
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
			
			pImage=(unsigned char *)CrossAPI_MapFile(cfd.cFileName);
	
			if(!pImage) {
				cerr<<"Cannot open input file "<<cfd.cFileName<<'\n';
				continue;
			}
			
			if(szPath) CrossAPI_SetCurrentDirectory(szStartDir);
			
			if(szLogFile) {
				log_out.open(szLogFile,ios::out|ios::app|ios::binary);
				if(!log_out) {
					cerr<<"Cannot open log file\n";
					CrossAPI_UnmapFile(pImage);
					continue;
				}
				out=&log_out;
			}
			else {
				out=&cout;
			}
			
			if(szPath) CrossAPI_SetCurrentDirectory(szPath);
	
			cout<<"Analyzing file \""<<cfd.cFileName<<"\"...\n";
			
			if(CrossAPI_GetFullPathName(cfd.cFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) ValidateFile(pImage,iMappingLength,&mpginfo,out,pcBuffer,false,-1);
			else ValidateFile(pImage,iMappingLength,&mpginfo,out,cfd.cFileName,false,-1);
	
			if(CrossAPI_GetFullPathName(cfd.cFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) PrintReport(out,pcBuffer,&mpginfo);
			else PrintReport(out,cfd.cFileName,&mpginfo);
	
			if(FixErrors&&mpginfo.iErrors) {
				hData=CrossAPI_GetTempFileAndName(CROSSAPI_MAX_PATH,pcBuffer);
				if(hData==-1) {
					cerr<<"Cannot open temporary file\n";
					CrossAPI_UnmapFile(pImage);
					log_out.close();
					continue;
				}
	
				cout<<"Rebuilding file \""<<cfd.cFileName<<"\"...\n";
	
				ValidateFile(pImage,iMappingLength,&mpginfo,NULL,NULL,true,(int)hData);
	
				CrossAPI_CloseFile(hData);
			}
	
			CrossAPI_UnmapFile(pImage);
	
			if(FixErrors&&mpginfo.iErrors) {
				strcpy((char *)pcBuffer2,cfd.cFileName);
				strcat((char *)pcBuffer2,".bak");
				if(!CrossAPI_MoveFile((char *)pcBuffer2,cfd.cFileName)) {
					cerr<<"Error renaming \""<<cfd.cFileName<<"\"\n";
					CrossAPI_UnmapFile(pImage);
					log_out.close();
					continue;
				}
				if(!CrossAPI_MoveFile(cfd.cFileName,pcBuffer)) {
					cerr<<"Error renaming temporary file\n";
					CrossAPI_UnmapFile(pImage);
					log_out.close();
					continue;
				}
	
			}
	
			if(FixErrors&&mpginfo.iErrors) {
				if(CrossAPI_GetFullPathName(cfd.cFileName,(char *)pcBuffer,CROSSAPI_MAX_PATH+1)) PrintMessage(out,"FIXED",pcBuffer,"File was rebuilt",-1);
				else PrintMessage(out,"FIXED",cfd.cFileName,"File was rebuilt",-1);
			}
	
			if(szLogFile) log_out.close();
	
			cout<<"Done!\n";
		}while(CrossAPI_FindNextFile(hFind,&cfd));
	
		CrossAPI_FindClose(hFind);
	}

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
