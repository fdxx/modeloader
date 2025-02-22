#include <filesystem.h>
#include <tier1/KeyValues.h>
#include "modeloader.h"
#include "modecvar.h"

#define CFGPATH	"cfg/modeloader/modeloader.cfg"
#define COMMAND_MAX_LENGTH 512

#define LOAD_PLUGINS 1
#define LOAD_SETTINGS 2

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
	const char *name = nullptr;
	if (!pCmdLine->CheckParm("-modename", &name))
		return;

	if (!GetLoadData(name))
		return;

	ModeLoader::LoadMode((void*)LOAD_PLUGINS);
}

void ModeLoader::SDK_OnUnload()
{
	g_ModeCvar.OnExtensionUnload();
	ConVar_Unregister();
}



bool ModeLoader::GetLoadData(const char *name)
{
	m_pluginCfg = {};
	m_settingCfg = {};

	KeyValues *kvRoot = new KeyValues("");
	if (!kvRoot->LoadFromFile(filesystem, CFGPATH, nullptr))
	{
		smutils->LogError(myself, "Failed to load %s", CFGPATH);
		kvRoot->deleteThis();
		return false;
	}

	for (KeyValues *kv = kvRoot->GetFirstTrueSubKey(); kv; kv = kv->GetNextTrueSubKey())
	{
		if (strcmp(kv->GetString("modename"), name))
			continue;
		
		m_pluginCfg = SplitString(kv->GetString("plugins_cfg"), ";");
		m_settingCfg = SplitString(kv->GetString("settings_cfg"), ";");
		break;
	}

	kvRoot->deleteThis();

	if (m_pluginCfg.empty() || m_settingCfg.empty())
	{
		smutils->LogError(myself, "Failed to GetLoadData: modename: %s", name);
		return false;
	}

	m_modename = name;
	return true;
}

void ModeLoader::LoadMode(void *data)
{
	int type = int(data);
	switch (type)
	{
		case LOAD_PLUGINS:
		{
			sv_modeloader_name.SetValue(g_ModeLoader.m_modename.c_str());
			g_ModeCvar.OnModeChanged();

			for (const std::string &file : g_ModeLoader.m_pluginCfg)
				g_ModeLoader.ExecFile(file);

			smutils->AddFrameAction(ModeLoader::LoadMode, (void*)LOAD_SETTINGS);
			return;
		}
		case LOAD_SETTINGS:
		{
			for (const std::string &file : g_ModeLoader.m_settingCfg)
				g_ModeLoader.ExecFile(file);
			
			break;
		}
	}
}

bool ModeLoader::ExecFile(const std::string &file)
{
	FileHandle_t fp = filesystem->Open(file.c_str(), "r", nullptr);
	if (!fp)
	{
		smutils->LogError(myself, "Failed to Open: %s", file.c_str());
		return false;
	}

	smutils->LogMessage(myself, "ExecFile %s", file.c_str());

	char buffer[COMMAND_MAX_LENGTH];
	while (!filesystem->EndOfFile(fp) && filesystem->ReadLine(buffer, sizeof(buffer)-2, fp))
	{
		// All codes after "//" are considered comments.
		char *ptr = strstr(buffer, "//");
		if (ptr) *ptr = '\0';

		ptr = TrimWhitespace(buffer, strlen(buffer));
		if (*ptr == '\0')
			continue;

		if (!strncmp(ptr, "exec", 4))
		{
			ptr = TrimWhitespace(ptr+4, strlen(ptr+4));
			ptr = TrimQuotes(ptr, strlen(ptr));
			std::string str = std::string("cfg/").append(ptr);
			ExecFile(str);
			continue;
		}

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
char *ModeLoader::TrimWhitespace(char *str, size_t len)
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

char *ModeLoader::TrimQuotes(char *str, size_t len)
{
	if (!len)
		return str;

	if (*str == '\"')
	{
		str++;
		len--;
	}
	
	if (len > 0 && str[len-1] == '\"')
		str[len-1] = '\0';

	return str;
}

std::vector<std::string> ModeLoader::SplitString(std::string str, std::string_view delimiter)
{
    std::vector<std::string> result;

    if (delimiter.empty())
	{
        result.push_back(str);
        return result;
    }

    size_t start = 0;
    size_t end = str.find(delimiter, start);

    while (end != std::string::npos)
	{
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.push_back(str.substr(start));
    return result;
}

CON_COMMAND(sm_modeloader, "Load the specified mode.")
{
	if (args.ArgC() != 2)
	{
		rootconsole->ConsolePrint("Usage: %s <modename>", args[0]);
		return;
	}

	if (!g_ModeLoader.GetLoadData(args[1]))
		return;

	ModeLoader::LoadMode((void*)LOAD_PLUGINS);
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

	g_ModeLoader.ExecFile(args[1]);
}
