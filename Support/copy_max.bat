@echo off

setlocal enabledelayedexpansion

REM Post build event that creates plugins bundle
REM %1 = config name (eg "Debug-2016")

if not defined MAX_VERSION (
	echo MAX_VERSION not defined
	exit 1
)

set CONFIG_NAME=%~1

echo ========================
echo Running Post-build Steps 
echo ========================
echo	Configuration: %CONFIG_NAME% 
echo	Max version: %MAX_VERSION%

REM the following call allows for some extra indirection, 2 escape passes

set DIST_PATH=dist
set PLUGIN_PATH=bin\x64\%CONFIG_NAME%\RadeonProRenderMaxPlugin.dlr

::Copy silent, confirm overwrite and copy if different
set DataFiles=^
	"Support\*.frs"^
	"Support\*.dat"

set RprPath="RadeonProRenderSDK\RadeonProRender\binWin64"

set CoreFiles=^
	"%RprPath%\Tahoe64.dll"^
	"%RprPath%\RadeonProRender64.dll"^
	"%RprPath%\RprLoadStore64.dll"^
	"%RprPath%\ProRenderGLTF.dll"

set RifPath="RadeonProImageProcessingSDK\Windows"
	
set RifLibFiles=^
	"%RifPath%\bin\MIOpen.dll"^
	"%RifPath%\bin\RadeonImageFilters64.dll"^
	"%RifPath%\bin\RadeonML-DirectML.dll"^
	"%RifPath%\bin\RadeonML-MIOpen.dll"

::make distribution folder with files for installer
if not exist %DIST_PATH% md %DIST_PATH%

for %%a in (%DataFiles%) do ( 
	xcopy %%a "%DIST_PATH%\data\*" /S /Y /I
)

set PluginBundle=CoreFiles RifLibFiles

for %%b in (%PluginBundle%) do (
	for %%a in (!%%b!) do ( 
		xcopy %%a "%DIST_PATH%\bin\*" /S /Y /I
	)
)

:: copy Image Library ML models
xcopy "RadeonProImageProcessingSDK\models" "%DIST_PATH%\data\models" /S /Y /I

:: copy scene convertion scripts
if not exist "%DIST_PATH%\data\SceneConvertionScripts" mkdir "%DIST_PATH%\data\SceneConvertionScripts"
copy /Y "ThirdParty\SceneConvertionScripts\vray2rpr.ms" "%DIST_PATH%\data\SceneConvertionScripts\vray2rpr.ms"

:: Copy plugin itself
xcopy %PLUGIN_PATH% "%DIST_PATH%\plug-ins\%MAX_VERSION%\*" /S /Y /I

:: Patch 3ds Max plugins configuration file
pushd Support
powershell -executionPolicy bypass -file RprSetup.ps1 %MAX_VERSION%
popd
