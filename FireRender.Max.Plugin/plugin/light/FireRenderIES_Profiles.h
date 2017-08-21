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
	static void ForEachProfile(std::function<void(const TCHAR* name)> f);
};

FIRERENDER_NAMESPACE_END
