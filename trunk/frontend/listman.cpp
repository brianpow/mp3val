#include "listman.h"
#include <cstring>

//Private functions

int CFileList::cleanup_recursive(CFileNode *node) {
	CFileNode *current,*temp;
	
	if(node->IsDir==true) {	
		for(current=node->child;current;current=temp) {
			temp=current->next;
			cleanup_recursive(current);
		}
	}
	
	delete node;
	
	return 0;
}

int CFileList::getfileno_internal(int viewmode,int n,FileInfo *fi,CFileNode **fn) {
	CFileNode *current=root.child;
	int cn=0;
	char szConstructedName[MAX_PATH+1];

	szConstructedName[0]='\0';

	for(;current;) {
		if(cn==n&&!current->child) {
			if(fn) *fn=current;
			if(fi) {
				strcat(szConstructedName,current->szNodeName);
				strcpy(fi->szFileName,szConstructedName);
				fi->state=current->state;
				fi->IsDir=current->IsDir;
			}
			return 1;
		}

		if(!current->next||(current->next->count.get(viewmode)+cn>n)) {
			strcat(szConstructedName,current->szNodeName);
			strcat(szConstructedName,"\\");
			current=current->child;
		}
		else {
			cn+=current->count.get(viewmode);
			current=current->next;
		}
	}

	return 0;
}

//Public functions

int CFileList::cleanup() {
	if(root.next) cleanup_recursive(&root);
	return 0;
}

int CFileList::addfile(char *szFileName) {
	char part[MAX_PATH+1];
	char *p;
	char *p2;
	CFileNode *current=&root;
	CFileNode *temp;
	bool IsDir;
	bool added=false;
	
	for(p=szFileName;p>(char *)0x01;p=strchr(p,'\\')+1) {
		strcpy(part,p);
		p2=strchr(part,'\\');
		if(p2) {
			*p2='\0';
			IsDir=true;
		}
		else IsDir=false;
		
		temp=current->search(part);
		if(!temp) {
			temp=current->addnode(part,ST_NOT_SCANNED,IsDir);
			added=true;
		}
		temp->count.inc(VM_NOT_SCANNED);
		
		current=temp;
	}
	
	if(!added) {
//This file has already been in the list
		for(;current!=&root;current=current->parent) current->count.dec(VM_NOT_SCANNED);
	}
	
	return 0;
}

int CFileList::getfileno(int viewmode,int n,FileInfo *fi) {
	return getfileno_internal(viewmode,n,fi,NULL);
}

int CFileList::deletefileno(int viewmode,int n) {
	CFileNode *current,*temp;
	int file_view_type;
	
	getfileno_internal(viewmode,n,NULL,&current);
	if(current->state==ST_NOT_SCANNED) file_view_type=VM_NOT_SCANNED;
	else if(current->state==ST_NORMAL) file_view_type=VM_NORMAL;
	else file_view_type=VM_PROBLEMS;
	
	for(;current!=&root;current=temp) {
		temp=current->parent;
		current->delnode();
		if(temp->child) break;
	}
	
	for(current=temp;current!=&root;current=current->parent) current->count.dec(file_view_type);
	
	return 0;
}
