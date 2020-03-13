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
#include <msi.h>
#include <msiquery.h>
#include <Shellapi.h>

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <regex>

#include <filesystem>

#pragma comment(lib, "msi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "User32.lib")

std::wstring getPropertyValue(MSIHANDLE hInstall, const std::wstring& propertyName)
{
	std::wstring result;

	DWORD valueSize = 0;

	UINT status = MsiGetProperty(hInstall, propertyName.c_str(), L"", &valueSize);

	if (ERROR_MORE_DATA == status)
	{
		valueSize++; // additional space for null terminator

		std::vector<WCHAR> value(valueSize);

		status = MsiGetProperty(hInstall, propertyName.c_str(), &value[0], &valueSize);

		if (ERROR_SUCCESS == status && !value.empty())
		{
			result = value.data();
		}
	}

	return std::move(result);
}

extern "C" __declspec(dllexport) UINT postInstall(MSIHANDLE hInstall)
{
	return ERROR_SUCCESS;
}
