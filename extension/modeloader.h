#pragma once

#include <string>
#include <smsdk_ext.h>

class ModeLoader : public SDKExtension
{
public:
	bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);
	bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	void SDK_OnAllLoaded();
	void SDK_OnUnload();

	bool GetLoadData(const char *modename);
	static void LoadMode(void *data);
	bool ExecFile(const std::string &file);
	char *TrimWhitespace(char *str, size_t len);
	char *TrimQuotes(char *str, size_t len);
	std::vector<std::string> SplitString(std::string str, std::string_view delimiter);

private:
	std::string m_modename;
	std::vector<std::string> m_pluginCfg;
	std::vector<std::string> m_settingCfg;
};
