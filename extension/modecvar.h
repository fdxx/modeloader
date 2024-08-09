#pragma once

#include <cstring>
#include <string>
#include <unordered_map>

struct CvarInfo;

class ModeCvar
{
public:
	~ModeCvar();

	void OnModeChanged();
	void OnExtensionUnload();

	void Insert(const char *name, CvarInfo *pInfo);
	bool Retrieve(const char *name, CvarInfo **pInfo);
	bool Remove(const char *name);
	void Clear(bool bRevert);
	std::string StrToLower(const char *name);

	bool m_bCvarChangedHook = false;
	bool m_bLockValue = true;
	
private:
	std::unordered_map<std::string, CvarInfo*> m_Cvars{};
};

extern ModeCvar g_ModeCvar;

