#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include "modeloader.h"
#include "modecvar.h"
#include "convar.h"
#include "filesystem.h"
#include "KeyValues.h"

#define CFGPATH	"cfg/modeloader/modeloader.cfg"
#define COMMAND_MAX_LENGTH 512

#define LOAD_PLUGINS 1
#define LOAD_SETTINGS 2

struct LoadData_t
{
	int m_type;
	char *m_modename;
	char *m_pluginCfg;
	char *m_settingCfg;

	LoadData_t(const char *modename)
	{
		m_type = LOAD_PLUGINS;
		m_modename = strdup(modename);
		m_pluginCfg = nullptr;
		m_settingCfg = nullptr;
	}

	~LoadData_t()
	{
		if (m_modename)
			free(m_modename);

		if (m_pluginCfg)
			free(m_pluginCfg);

		if (m_settingCfg)
			free(m_settingCfg);
	}

	void SetCfgPath(const char *plugins, const char *setting)
	{
		m_pluginCfg = plugins ? strdup(plugins) : nullptr;
		m_settingCfg = setting ? strdup(setting) : nullptr;
	}
};

static bool GetLoadData(LoadData_t *data);
static void LoadMode(void *obj);
static bool ExecFile(const char* file);
static char *UTIL_TrimWhitespace(char *str, size_t len);


ModeLoader g_ModeLoader;
SMEXT_LINK(&g_ModeLoader);

static IFileSystem *filesystem;
static ConVar sv_modeloader_name("sv_modeloader_name", "", FCVAR_NOTIFY, "Track or get Current mode name.");



bool ModeLoader::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	return true;
}

bool ModeLoader::SDK_OnLoad( char *error, size_t maxlength, bool late )
{
	// Timer(AddFrameAction) won't work when server is hibernate.
	g_pCVar->FindVar("sv_hibernate_when_empty")->SetValue(0);
	ConVar_Register(0, nullptr);
	return true;
}

void ModeLoader::SDK_OnAllLoaded()
{
	ICommandLine *pCmdLine = gamehelpers->GetValveCommandLine();
	const char *value = nullptr;
	if (!pCmdLine->CheckParm("-modename", &value))
		return;

	LoadData_t *data = new LoadData_t(value);
	if (!GetLoadData(data))
	{
		delete data;
		return;
	}

	LoadMode(data);
}

void ModeLoader::SDK_OnUnload()
{
	g_ModeCvar.OnExtensionUnload();
	ConVar_Unregister();
}



CON_COMMAND(sm_modeloader, "Load the specified mode.")
{
	if (args.ArgC() != 2)
	{
		rootconsole->ConsolePrint("Usage: %s <modename>", args[0]);
		return;
	}

	LoadData_t *data = new LoadData_t(args[1]);
	if (!GetLoadData(data))
	{
		delete data;
		return;
	}

	LoadMode(data);
}

CON_COMMAND(sm_modeloader_reloadmap, "Reload the current map.")
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "changelevel \"%s\"\n", gamehelpers->GetCurrentMap());
	engine->ServerCommand(buffer);
}


CON_COMMAND(sm_modeloader_execfile, "Parse the file and execute it line by line.")
{
	if (args.ArgC() != 2)
	{
		rootconsole->ConsolePrint("Usage: %s <file>", args[0]);
		return;
	}

	ExecFile(args[1]);
}


static bool GetLoadData(LoadData_t *data)
{
	KeyValues *kvRoot = new KeyValues("");
	if (!kvRoot->LoadFromFile(filesystem, CFGPATH, nullptr))
	{
		smutils->LogError(myself, "Failed to load %s", CFGPATH);
		kvRoot->deleteThis();
		return false;
	}

	for (KeyValues *kv = kvRoot->GetFirstTrueSubKey(); kv; kv = kv->GetNextTrueSubKey())
	{
		if (strcmp(kv->GetString("modename"), data->m_modename))
			continue;
		
		const char *plugins = kv->GetString("plugins_cfg", nullptr);
		const char *settings = kv->GetString("settings_cfg", nullptr);
		data->SetCfgPath(plugins, settings);
		break;
	}

	kvRoot->deleteThis();

	if (!data->m_modename || !data->m_pluginCfg || !data->m_settingCfg)
	{
		smutils->LogError(myself, "Failed to GetLoadData: \nmodename: %s, \nplugins_cfg: %s, \nsettings_cfg: %s", data->m_modename, data->m_pluginCfg, data->m_settingCfg);
		return false;
	}

	return true;
}

static void LoadMode(void *obj)
{
	LoadData_t *data = (LoadData_t*)obj;

	switch (data->m_type)
	{
		case LOAD_PLUGINS:
		{
			sv_modeloader_name.SetValue(data->m_modename);
			g_ModeCvar.OnModeChanged();

			ExecFile(data->m_pluginCfg);

			data->m_type = LOAD_SETTINGS;
			smutils->AddFrameAction(LoadMode, data);
			return;
		}
		case LOAD_SETTINGS:
		{
			ExecFile(data->m_settingCfg);
			break;
		}
	}

	delete data;
}

static bool ExecFile(const char* file)
{
	FileHandle_t fp = filesystem->Open(file, "r", nullptr);
	if (!fp)
	{
		smutils->LogError(myself, "Failed to Open: %s", file);
		return false;
	}

	smutils->LogMessage(myself, "ExecFile %s", file);

	char buffer[COMMAND_MAX_LENGTH];
	while (!filesystem->EndOfFile(fp) && filesystem->ReadLine(buffer, sizeof(buffer)-2, fp))
	{
		// All codes after "//" are considered comments.
		char *ptr = strstr(buffer, "//");
		if (ptr) *ptr = '\0';

		ptr = UTIL_TrimWhitespace(buffer, strlen(buffer));
		if (*ptr == '\0')
			continue;

		size_t len = strlen(ptr);
		ptr[len] = '\n';
		ptr[len+1] = '\0';

		engine->ServerCommand(ptr);
		engine->ServerExecute();
	}

	filesystem->Close(fp);
	return true;
}

// https://github.com/alliedmodders/sourcemod/blob/master/core/logic/stringutil.cpp#L338
static char *UTIL_TrimWhitespace(char *str, size_t len)
{
	char *end = str + len - 1;

	if (!len)
		return str;

	/* Iterate backwards through string until we reach first non-whitespace char */
	while (end >= str && textparsers->IsWhitespace(end))
		end--;

	/* Replace first whitespace char (at the end) with null terminator.
	 * If there is none, we're just replacing the null terminator.
	 */
	*(end + 1) = '\0';

	while (*str != '\0' && textparsers->IsWhitespace(str))
		str++;
	return str;
}


