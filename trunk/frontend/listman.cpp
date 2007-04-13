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
	
	if(node!=&root) delete node;
	
	return 0;
}

int CFileList::getfileno_internal(int viewmode,int n,FileInfo *fi,CFileNode **fn) {
	CFileNode *current=root.child;
	int cn=0;
	char szConstructedName[MAX_PATH+1];

	szConstructedName[0]='\0';

	for(;current;) {
		if(cn+current->count.get(viewmode)==n+1&&!current->child) {
			if(fn) *fn=current;
			if(fi) {
				strcat(szConstructedName,current->szNodeName);
				strcpy(fi->szFileName,szConstructedName);
				fi->state=current->state;
				fi->IsDir=current->IsDir;
				fi->proot=&(current->sroot);
			}
			return 1;
		}

		if(!current->next||(current->count.get(viewmode)+cn>n)) {
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
	if(root.child) cleanup_recursive(&root);
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
	
	if(!getfileno_internal(viewmode,n,NULL,&current)) return 0;
	if(current->state==ST_NOT_SCANNED) file_view_type=VM_NOT_SCANNED;
	else if(current->state==ST_NORMAL) file_view_type=VM_NORMAL;
	else file_view_type=VM_PROBLEMS;
	
	for(;current!=&root;current=temp) {
		temp=current->parent;
		delete current;
		if(temp->child) break;
	}
	
	for(current=temp;current!=&root;current=current->parent) current->count.dec(file_view_type);
	
	return 0;
}

int CFileList::changestate(int viewmode,int n,int state) {
	CFileNode *pfnThisFile;
	CFileNode *current;
	int curstate;
	if(getfileno_internal(viewmode,n,NULL,&pfnThisFile)) {
		curstate=pfnThisFile->state;
		pfnThisFile->state=state;
		for(current=pfnThisFile;current;current=current->parent) {
			current->count.dec(curstate);
			current->count.inc(state);
		}
	}
	
	return 0;
}

int CStringNode::addnode(char *p) {
	CStringNode *current;
	for(current=this;current->next;current=current->next);
	current->next=new CStringNode;
	current=current->next;
	current->str=new char[lstrlen(p)+1];
	lstrcpy(current->str,p);
	return 0;
}

int CStringNode::cleanup() {
	CStringNode *current,*temp;
	for(current=this->next;current;current=temp) {
		temp=current->next;
		delete current;
	}
	next=NULL;
	return 0;
}
