#pragma once

#include "smsdk_ext.h"

class ModeLoader : public SDKExtension
{
public:
	bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);
	bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	void SDK_OnAllLoaded();
	void SDK_OnUnload();
};
