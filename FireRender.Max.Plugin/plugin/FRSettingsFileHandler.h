/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Manages file I/O for persistent settings
*********************************************************************************************************************************/

#pragma once

#include <string>

class FRSettingsFileHandler
{
private:
	static std::string settingsFolder;
	static const std::string &getSettingsFileName();

public:
	static const std::string GlobalDontShowNotification;
	static const std::string DevicesName;
	static const std::string DevicesSelected;
	static const std::string OverrideCPUThreadCount;
	static const std::string CPUThreadCount;

	static std::string getAttributeSettingsFor(const std::string &attributeName);

	static void setAttributeSettingsFor(const std::string &attributeName, const std::string &value);
};

