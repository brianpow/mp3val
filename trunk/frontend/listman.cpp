#include "listman.h"
#include <cstring>

using namespace std;
//!!!!!!!!!!!!!!!!!!!!!!!!
#include <windows.h>
//!!!!!!!!!!!!!!!!!!!!!!!!

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
	
	for(p=szFileName;(*p&&p);p=strchr(p,'\\')+1) {
		MessageBox(NULL,p,"p is",MB_OK);
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
		MessageBox(NULL,temp->szNodeName,"temp->szNodeName",MB_OK);
		
		current=temp;
	}
	
	MessageBox(NULL,"File added",NULL,MB_OK);
	
	return 0;
}
