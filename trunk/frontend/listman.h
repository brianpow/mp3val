#define ST_NOT_SCANNED 0
#define ST_NORMAL 1

struct CFileNode {
	char szFileName[MAX_PATH+1];
	int state;
	CFileNode *next;
	CFileNode *child;
	bool IsDir;
	node() {
		*szFileName='\0';
		state=ST_NOT_SCANNED;
		next=NULL;
		child=NULL;
		IsDir=false;
	}
	void clean() {
		node *current,*temp;
		for(current=next;current;current=temp) {
			temp=current->next;
			delete current;
		}
	}
};
