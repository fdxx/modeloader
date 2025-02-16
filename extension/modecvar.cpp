#include <smsdk_ext.h>
#include <tier1/convar.h>
#include "modecvar.h"

ModeCvar g_ModeCvar;
static void OnGlobalCvarChanged(IConVar *pCvar, const char *pOldValue, float flOldValue);

struct CvarInfo
{
	ConVar *m_pCvar;
	std::string m_sLockValue;

	CvarInfo(ConVar *pCvar, const char *lockvalue)
	{
		m_pCvar = pCvar;
		m_sLockValue = lockvalue;
	}
};

ModeCvar::ModeCvar()
{
	m_bCvarChangedHook = false;
	m_bLockValue = true;
	m_Cvars = {};
}

ModeCvar::~ModeCvar()
{
	Clear(false);
}

void ModeCvar::Insert(const char *name, CvarInfo *pInfo)
{
	m_Cvars[StrToLower(name)] = pInfo;
}

bool ModeCvar::Retrieve(const char *name, CvarInfo **pInfo)
{
	auto it = m_Cvars.find(StrToLower(name));
	if (it != m_Cvars.end())
	{
		*pInfo = it->second;
		return true;
	}
	return false;
}

bool ModeCvar::Remove(const char *name)
{
	auto it = m_Cvars.find(StrToLower(name));
	if (it != m_Cvars.end())
	{
		CvarInfo *pInfo = it->second;
		delete pInfo;
		m_Cvars.erase(it);
		return true;
	}
	return false;
}

void ModeCvar::Clear(bool bRevert)
{
	for (auto it = m_Cvars.begin(); it != m_Cvars.end(); ++it)
	{
		CvarInfo *pInfo = it->second;
		if (bRevert)
			pInfo->m_pCvar->Revert();
		delete pInfo;
	}
	m_Cvars.clear();
}

std::string ModeCvar::StrToLower(const char *name)
{
	std::string result(name);
	for (size_t i = 0, len = result.size(); i < len; i++)
		result[i] = std::tolower(result[i]);
	return result; // std::move(result);
}


void ModeCvar::OnModeChanged()
{
	m_bLockValue = false;
	Clear(true);
	m_bLockValue = true;
}

void ModeCvar::OnExtensionUnload()
{
	g_pCVar->RemoveGlobalChangeCallback(OnGlobalCvarChanged);
	Clear(false);
}

CON_COMMAND(sm_modecvar, "Add modecvar.")
{
	if (args.ArgC() != 3)
	{
		rootconsole->ConsolePrint("Usage: %s <CvarName> <LockValue>", args[0]);
		return;
	}

	const char *cvarName = args[1];
	const char *lockValue = args[2];

	CvarInfo *pInfo = nullptr;
	if (g_ModeCvar.Retrieve(cvarName, &pInfo))
	{
		rootconsole->ConsolePrint("cvar %s has been added!", args[1]);
		return;
	}

	// valve is not case sensitive.
	ConVar *pCvar = g_pCVar->FindVar(cvarName);
	if (!pCvar)
	{	
		rootconsole->ConsolePrint("Failed to FindConVar %s", cvarName);
		return;
	}

	pCvar->SetValue(lockValue);
	pInfo = new CvarInfo(pCvar, lockValue);
	g_ModeCvar.Insert(pCvar->GetName(), pInfo);
	smutils->LogMessage(myself, "ModeCvarAdd: %s %s", pCvar->GetName(), lockValue);

	if (!g_ModeCvar.m_bCvarChangedHook)
	{
		g_ModeCvar.m_bCvarChangedHook = true;
		g_pCVar->InstallGlobalChangeCallback(OnGlobalCvarChanged);
	}
}

CON_COMMAND(sm_modecvar_del, "delete modecvar.")
{
	if (args.ArgC() != 2)
	{
		rootconsole->ConsolePrint("Usage: %s <CvarName>", args[0]);
		return;
	}

	const char *cvarName = args[1];
	if (g_ModeCvar.Remove(cvarName))
		smutils->LogMessage(myself, "ModeCvarDel: %s", cvarName);
}


static void OnGlobalCvarChanged(IConVar *pCvar, const char *pOldValue, float flOldValue)
{
	ConVarRef convar(pCvar);
	if (!strcmp(convar.GetString(), pOldValue))
		return;

	CvarInfo *pInfo = nullptr;
	const char *cvarName = pCvar->GetName();
	if (!g_ModeCvar.Retrieve(cvarName, &pInfo))
		return;

	smutils->LogMessage(myself, "ModeCvarRevert: %s: %s -> %s", cvarName, pOldValue, convar.GetString());

	if (g_ModeCvar.m_bLockValue)
		convar.SetValue(pInfo->m_sLockValue.c_str());
}
