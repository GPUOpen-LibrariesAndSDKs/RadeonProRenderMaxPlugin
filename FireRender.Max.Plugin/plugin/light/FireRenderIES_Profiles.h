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

#include <functional>

#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN

// Utility class that manages all the work with IES profiles files
class FireRenderIES_Profiles
{
public:
	static std::wstring GetIESProfilesDirectory();

	//	Prompts user to select an IES file
	//	Returns false if there is any error or user canceled the dialog
	//		'filename' - mandatory output parameter - contains full file name
	//		'nFileOffset' - optional output parameter - the zero-based offset,
	//			in characters, from the beginning of the path to the file name in 'filename' string
	//		'nExtOffset' - optional output parameter - The zero-based offset,
	//			in characters, from the beginning of the path to the file name extension in 'filename' string
	static bool GetIESFileName(std::wstring& filename,
		size_t* nFileOffset = nullptr,
		size_t* nExtOffset = nullptr);

	// Copies profile file to the profiles directory
	// Returns true if new file was created
	// Returns false if there is any error or file was replaced
	static bool CopyIES_File(const TCHAR* from, size_t nameOffset);

	// Calls passed function object for each profile in the profiles directory
	static void ForEachProfile(std::function<void(const TCHAR* profileName)> f);

	// Combines profiles directory path and the profile name
	static std::wstring ProfileNameToPath(const TCHAR* profileName);
};

FIRERENDER_NAMESPACE_END
