#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <windows.h>

#define MAX_PROFILE_STRING (2*MAX_PATH)

class CSettings {
	char szIniFileName[MAX_PATH+1];
public:
// core-related options
	bool bDeleteBaks;
	bool bKeepTimestamps;
// frontend-related options
	bool bIgnoreMissingTags;
// member functions
	CSettings();	
	~CSettings();
	int load();
	int write();
};

#endif
