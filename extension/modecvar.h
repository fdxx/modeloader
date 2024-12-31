#pragma once

#include <string>
#include <unordered_map>

struct CvarInfo;

class ModeCvar
{
public:
	ModeCvar();
	~ModeCvar();

	void Insert(const char *name, CvarInfo *pInfo);
	bool Retrieve(const char *name, CvarInfo **pInfo);
	bool Remove(const char *name);
	void Clear(bool bRevert);
	std::string StrToLower(const char *name);
	void OnModeChanged();
	void OnExtensionUnload();

	bool m_bCvarChangedHook;
	bool m_bLockValue;
	
private:
	std::unordered_map<std::string, CvarInfo*> m_Cvars;
};

extern ModeCvar g_ModeCvar;

