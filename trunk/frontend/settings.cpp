#include "settings.h"
#include <windows.h>
#include <shlwapi.h>

CSettings::CSettings() {
	GetModuleFileName(NULL,szIniFileName,MAX_PATH+1);
	PathRemoveFileSpec(szIniFileName);
	lstrcat(szIniFileName,"\\mp3val-frontend.ini");
	load();
}

CSettings::~CSettings() {
	write();
}

int CSettings::load() {
	char szTemp[MAX_PROFILE_STRING];
	
	GetPrivateProfileString("Core","DeleteBaks","false",szTemp,MAX_PROFILE_STRING,szIniFileName);
	if(!lstrcmp(szTemp,"true")) bDeleteBaks=true;
	else bDeleteBaks=false;
	
	GetPrivateProfileString("Core","KeepTimestamps","false",szTemp,MAX_PROFILE_STRING,szIniFileName);
	if(!lstrcmp(szTemp,"true")) bKeepTimestamps=true;
	else bKeepTimestamps=false;

	GetPrivateProfileString("Frontend","IgnoreMissingTags","false",szTemp,MAX_PROFILE_STRING,szIniFileName);
	if(!lstrcmp(szTemp,"true")) bIgnoreMissingTags=true;
	else bIgnoreMissingTags=false;
	
	return 0;
}

int CSettings::write() {
	char szTemp[MAX_PROFILE_STRING];
	
	if(bDeleteBaks) lstrcpy(szTemp,"true");
	else lstrcpy(szTemp,"false");
	WritePrivateProfileString("Core","DeleteBaks",szTemp,szIniFileName);

	if(bKeepTimestamps) lstrcpy(szTemp,"true");
	else lstrcpy(szTemp,"false");
	WritePrivateProfileString("Core","KeepTimestamps",szTemp,szIniFileName);

	if(bIgnoreMissingTags) lstrcpy(szTemp,"true");
	else lstrcpy(szTemp,"false");
	WritePrivateProfileString("Frontend","IgnoreMissingTags",szTemp,szIniFileName);
	
	return 0;
}
