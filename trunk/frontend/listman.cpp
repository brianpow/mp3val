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

//Public functions

int CFileList::cleanup() {
	if(root.next) cleanup_recursive(root.next);
	return 0;
}

int CFileList::addfile(char *szFileName) {
	char part[MAX_PATH+1];
	char *p;
	char *p2;
	CFileNode *current=&root;
	CFileNode *temp;
	bool IsDir;
	
	for(p=szFileName;p>(char *)0x01;p=strchr(p,'\\')+1) {
		strcpy(part,p);
		p2=strchr(part,'\\');
		if(p2) {
			*p2='\0';
			IsDir=true;
		}
		else IsDir=false;
		
		temp=current->search(part);
		if(!temp) temp=current->addnode(part,ST_NOT_SCANNED,IsDir);
		temp->count++;
		
		current=temp;
	}
	
	return 0;
}

int CFileList::getfileno(int n,FileInfo *fi) {
	CFileNode *current=root.child;
	int cn=0;
	char szConstructedName[MAX_PATH+1];

	szConstructedName[0]='\0';

	for(;current;) {
		if(cn==n&&!current->child) {
			strcat(szConstructedName,current->szNodeName);
			strcpy(fi->szFileName,szConstructedName);
			fi->state=current->state;
			fi->IsDir=current->IsDir;
			return 1;
		}

		if(!current->next||(current->next->count+cn>n)) {
			strcat(szConstructedName,current->szNodeName);
			strcat(szConstructedName,"\\");
			current=current->child;
		}
		else {
			cn+=current->count;
			current=current->next;
		}
	}

	return 0;
}
