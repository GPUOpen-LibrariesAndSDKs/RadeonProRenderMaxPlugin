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

#include "Common.h"
#include "Testing.h"
#include "utils\Utils.h"
#include "utils/Stack.h"
#include <direct.h>
#include "bitmap.h"
#include "plugin/FireRenderer.h"
#include "plugin/ParamBlock.h"
FIRERENDER_NAMESPACE_BEGIN;

/// Renders an image in 3ds Max and saves it to specified path. The scene must exist at location testPath/testName/scene.max
/// and it must be set up to render with RPR
/// \param testPath Root folder where the test sub-directory is located
/// \param testName Name of the test, which is also the name of sub-directory inside testPath
/// \param passes How many passes to render
/// \param outputPath where to write the result image
/// \return true if the operation succeeded, false if there was any problem (non-existing scene, RPR not set as renderer, 
///         ...)
bool renderSingle(const std::string& testPath, const std::string& testName, const int passes, const std::string& outputPath) {
    Interface11* ip = GetCOREInterface11();
    
    const std::string fullName = testPath + "/" + testName + "/scene.max";
    ip->SetQuietMode(TRUE);
    int res = ip->LoadFromFile(ToUnicode(fullName).c_str(), Interface8::kSuppressPrompts | Interface8::kUseFileUnits | Interface8::kSetCurrentFilePath);
    ip->SetQuietMode(FALSE);
    
    if(!res) { //scene cannot be open
        FASSERT(false);
        return false;
    }

    Renderer* renderer = ip->GetCurrentRenderer(true);
    FASSERT(renderer && dynamic_cast<FireRenderer*>(renderer));

    if(!(renderer && dynamic_cast<FireRenderer*>(renderer))) { //RPR is not the current renderer
        FASSERT(false);
        return false;
    }

    IParamBlock2* pb = renderer->GetParamBlock(0); // sets the required number of passes
    SetInPb(pb, PARAM_PASS_LIMIT, passes);

    // Prepare the bitmap where the results are written
    BitmapInfo bi;
    bi.SetType(BMM_FLOAT_RGBA_32);
    bi.SetWidth(WORD(ip->GetRendWidth()));
    bi.SetHeight(WORD(ip->GetRendHeight()));
    bi.SetFlags(MAP_HAS_ALPHA);
    bi.SetCustomFlag(0);
    bi.SetAspect(1.f);
    ::Bitmap *bmap = TheManager->Create(&bi);
    FASSERT(bmap != NULL);
     
    // Render the image at time 0 into our bitmap
    ip->QuickRender(0, bmap);
    
    // Save the image
    bi.SetName(ToUnicode(outputPath + testName + ".png").c_str());
    res = bmap->OpenOutput(&bi);
    FASSERT(res == BMMRES_SUCCESS);
    res = bmap->Write(&bi);
    FASSERT(res == BMMRES_SUCCESS);
    res = bmap->Close(&bi);
    bmap->DeleteThis();

    return true;
}


void Tester::stopRender() {
    this->cancelled = true;
    GetCOREInterface11()->AbortRender();
}

/// Returns all suitable (not starting with .) directories inside given input directory
Stack<std::string> getSuitableDirs(const std::string& path) {
    WIN32_FIND_DATA findFileData;
    HANDLE handle = FindFirstFile(ToUnicode(path + "/*").c_str(), &findFileData);

    Stack<std::string> result;
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = ToAscii(findFileData.cFileName);
            if (filename.size() > 0 && filename[0] != '.' && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                result.push(filename);
            }
        } while (FindNextFile(handle, &findFileData));
        FindClose(handle);
    }
    return result;
}


void Tester::renderTests(std::string directory, std::string filter, const int passes) {
    if (directory.size() == 0) {
        return;
    }
    this->cancelled = false;

    if (directory.back() != '/')
	        directory += '/';
    
    const std::string outputPath = directory + ".output/"; //folder where output images are saved

	int mkres = _mkdir(outputPath.c_str());

    if ((mkres == 0) || errno == EEXIST)
	{
		Stack<std::string> dirs = getSuitableDirs(directory);
		if (filter.size() > 0) { //non-empty filter -> we will use it to remove some directories
			Stack<std::string> tmp = dirs;
			dirs.clear();
			for (auto i : tmp) {
				if (i.find(filter) != i.npos) {
					dirs.push(i);
				}
			}
		}

		int testNumber = 0;
		for (const auto& i : dirs) { // go through all suitable directories, update status message, and render each test
			this->progressMsg = toString(testNumber) + "/" + toString(dirs.size()) + " done. Current file: " + i;

			bool res = renderSingle(directory, i, passes, outputPath);
			FASSERT(res);
		}
	}
}

FIRERENDER_NAMESPACE_END;
