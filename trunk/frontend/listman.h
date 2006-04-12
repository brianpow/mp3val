#ifndef __LISTMAN_H__
#define __LISTMAN_H__

#define ST_NOT_SCANNED 0
#define ST_NORMAL 1

#include <cstring>

#include <windows.h>

struct FileInfo {
	char szFileName[MAX_PATH+1];
	int state;
	bool IsDir;
};

struct CFileNode {
	char szNodeName[MAX_PATH+1];
	int state;
	int count;
	CFileNode *next;
	CFileNode *child;
	bool IsDir;
	
	CFileNode() {
		szNodeName[0]='\0';
		state=ST_NOT_SCANNED;
		count=0;
		next=NULL;
		child=NULL;
		IsDir=false;
	}
	
	CFileNode(char *name,int st,bool isdir) {
		strcpy(szNodeName,name);
		state=st;
		count=0;
		next=NULL;
		child=NULL;
		IsDir=isdir;
	}
	
	CFileNode *search(char *name) {
		CFileNode *current;
		for(current=this->child;current;current=current->next) {
			if(!strcmp(name,current->szNodeName)) return current;
		}
		return NULL;
	}
	
	CFileNode *addnode(char *name,int st, bool isdir) {
		CFileNode *current;
		if(!this->child) {
			this->child=new CFileNode(name,st,isdir);
			return this->child;
		}
		for(current=this->child;current->next;current=current->next);
		current->next=new CFileNode(name,st,isdir);
		return current->next;
	}
};

class CFileList {
	CFileNode root;
	int cleanup_recursive(CFileNode *node);
public:
	CFileList() {
	}
	~CFileList() {
	}
	int cleanup();
	int addfile(char *szFileName);
	int deletefile(char *szFileName);
	int getfileno(int n,FileInfo *fi);
};

#endif
