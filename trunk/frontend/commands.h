#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#define ST_NOT_SCANNED 0
#define ST_NORMAL 1

struct node {
	char szFileName[MAX_PATH+1];
	int state;
	node *next;
	node() {
		*szFileName='\0';
		state=ST_NOT_SCANNED;
		next=NULL;
	}
	void clean() {
		node *current,*temp;
		for(current=next;current;current=temp) {
			temp=current->next;
			delete current;
		}
	}
};

int InitCommands();

int RefreshView(int iViewType);

int DoFileAddFile();
int DoFileAddDir();
int DoFileQuit();

int DoActionsScanAll();
int DoActionsScanSel();
int DoActionsFixSel();
int DoActionsOptions();

int DoHelpAbout();

#endif
