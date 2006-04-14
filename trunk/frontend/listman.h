#ifndef __LISTMAN_H__
#define __LISTMAN_H__

#define VM_EVERYTHING 0
#define VM_NOT_SCANNED 1
#define VM_NORMAL 2
#define VM_PROBLEMS 3

#define VM_COUNT 4

#define ST_NOT_SCANNED 0
#define ST_NORMAL 1
#define ST_PROBLEM 2

#define ST_COUNT 3

#include <cstring>

#include <windows.h>

struct CCount {
	int total;
	int st_arr[ST_COUNT];
	
	CCount() {
		total=0;
		memset(&st_arr,'\0',sizeof(int)*ST_COUNT);
	}
	int get(int viewmode) {
		if(viewmode==VM_EVERYTHING) return total;
		if(viewmode>=VM_COUNT||viewmode<0) return -1;
		return st_arr[viewmode];
	}
	void inc(int viewmode) {
		total++;
		if(viewmode>0&&viewmode<=VM_COUNT-1) st_arr[viewmode]++;
	}
	void dec(int viewmode) {
		total--;
		if(viewmode>0&&viewmode<=VM_COUNT-1) st_arr[viewmode]--;
	}
};

struct FileInfo {
	char szFileName[MAX_PATH+1];
	int state;
	bool IsDir;
};

struct CFileNode {
	char szNodeName[MAX_PATH+1];
	int state;
	CCount count;
	CFileNode *next;
	CFileNode *child;
	CFileNode *parent;
	bool IsDir;
	
	CFileNode() {
		szNodeName[0]='\0';
		state=ST_NOT_SCANNED;
		next=NULL;
		child=NULL;
		parent=NULL;
		IsDir=false;
	}
	
	CFileNode(char *name,int st,bool isdir,CFileNode *par) {
		strcpy(szNodeName,name);
		state=st;
		next=NULL;
		child=NULL;
		parent=par;
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
			this->child=new CFileNode(name,st,isdir,this);
			return this->child;
		}
		for(current=this->child;current->next;current=current->next);
		current->next=new CFileNode(name,st,isdir,this);
		return current->next;
	}
	
	int delnode() {
		CFileNode *current;
		if(parent->child==this) {
			parent->child=next;
		}
		else {
			for(current=parent->child;current;current=current->next) if(current->next==this) break;
			if(!current) return -1;
			current->next=next;
		}
		delete this;
	}
};

class CFileList {
	CFileNode root;
	int cleanup_recursive(CFileNode *node);
	int getfileno_internal(int viewmode,int n,FileInfo *fi,CFileNode **fn);
public:
	CFileList() {
	}
	~CFileList() {
	}
	int cleanup();
	int addfile(char *szFileName);
	int getfileno(int viewmode,int n,FileInfo *fi);
	int deletefileno(int viewmode,int n);
};

#endif
