/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Manages file I/O for persistent settings
*********************************************************************************************************************************/

#include "FRSettingsFileHandler.h"

#include <iostream>
#include <fstream>
#include "utils/Utils.h"


#define settingsFileName "FireRenderSettings.ini"

const std::string FRSettingsFileHandler::GlobalDontShowNotification = "GlobalDontShowNotification";

std::string FRSettingsFileHandler::settingsFolder = "";


const std::string &FRSettingsFileHandler::getSettingsFileName()
{
	if (settingsFolder.empty())
	{
		const std::string s = FireRender::ToAscii(FireRender::GetDataStoreFolder());
		settingsFolder = s + settingsFileName;
	}
	return settingsFolder;
}

std::string FRSettingsFileHandler::getAttributeSettingsFor(const std::string &attributeName)
{

	const std::string &fileName = getSettingsFileName();
	std::ifstream file(fileName);
	if (!file.is_open())
		return "";
	std::string str;
	std::string result = "";
	std::size_t loc;
	while (std::getline(file, str))
	{
		loc = str.find(attributeName);
		if (loc != std::string::npos)
			 result = str.substr(attributeName.length() + 1);
	}
	file.close();
	return result;
}

void FRSettingsFileHandler::setAttributeSettingsFor(const std::string  &attributeName, const std::string &value)
{
	const std::string &fileName = getSettingsFileName();
	std::ifstream iFile(fileName);
	std::string nSettings = "";

	bool found = false;
	if (iFile.is_open())
	{
		std::string lineStr;
		std::size_t loc;
		while (std::getline(iFile, lineStr))
		{
			loc = lineStr.find(' ');
			if (loc != std::string::npos)
			{
				std::string attributeNameActual = lineStr.substr(0, loc);
				if(attributeNameActual==attributeName)
				{
					if(!found)
					{
						nSettings += attributeName + " " + value + "\n";
						found = true;
					}
				}
				else
				{
					//keep value
					nSettings += lineStr + "\n";
				}
			}
		}
		iFile.close();
	}
	
	if(!found) 
	{
		nSettings += attributeName + " " + value + "\n";
	}

	std::ofstream oFile;
	oFile.open(fileName, std::ios::trunc);
	if (oFile.is_open())
	{
		oFile << nSettings;
		oFile.close();
	}
}
