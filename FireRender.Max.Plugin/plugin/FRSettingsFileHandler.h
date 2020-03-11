/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

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

