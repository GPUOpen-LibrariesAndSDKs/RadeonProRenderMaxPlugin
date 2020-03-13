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

#include "stdafx.h"

#include <thread>
#include <sstream>
#include <iomanip>

#include <filesystem>

void LogCallback(const char* sz)
{
	namespace fs = std::experimental::filesystem;

	std::string tmpPath = fs::temp_directory_path().string();

	std::string LogPath = tmpPath + "RprPluginFor3dsMax.log";

	std::ofstream os(LogPath, std::ofstream::out | std::ofstream::app);

	os << std::setbase(16) << std::setw(4) << std::this_thread::get_id() << ": " << sz << std::endl;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Logger::AddCallback(LogCallback, Logger::LevelInfo);
		LogSystem("Start istaller dll...");
		break;

	case DLL_PROCESS_DETACH:
		LogSystem("Stop istaller dll.");
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
