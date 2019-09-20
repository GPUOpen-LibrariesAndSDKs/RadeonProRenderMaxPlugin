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

set CoreFiles=^
	"ThirdParty\RadeonProRender SDK\Win\bin\Tahoe64.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\RadeonProRender64.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\RprLoadStore64.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\OpenImageIO_RPR.dll"

set EmbreeFiles=^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\embree.dll"^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\tbb.dll"^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\tbbmalloc.dll"

set AxfFiles=^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\AxfConverter.dll"^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\AxFDecoding_r.dll"^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\FreeImage.dll"
	
set ImageLibFiles=^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\RadeonImageFilters64.dll"^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\MIOpen.dll"^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\OpenImageDenoise.dll"^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\RadeonProML.dll"^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\tbb-4233fe.dll"^
	"ThirdParty\RadeonProImageProcessing\Windows\lib\tbbmalloc-4233fe.dll"
	
set GltfFiles=^
	"ThirdParty\RadeonProRender-GLTF\Win\lib\ProRenderGLTF.dll"

::make distribution folder with files for installer
if not exist %DIST_PATH% md %DIST_PATH%

for %%a in (%DataFiles%) do ( 
	xcopy %%a "%DIST_PATH%\data\*" /S /Y /I
)

set PluginBundle=CoreFiles EmbreeFiles AxfFiles GltfFiles ImageLibFiles

for %%b in (%PluginBundle%) do (
	for %%a in (!%%b!) do ( 
		xcopy %%a "%DIST_PATH%\bin\*" /S /Y /I
	)
)

:: copy Image Library ML models
xcopy "ThirdParty\RadeonProImageProcessing\models" "%DIST_PATH%\data\models" /S /Y /I

:: copy scene convertion scripts
if not exist "%DIST_PATH%\data\SceneConvertionScripts" mkdir "%DIST_PATH%\data\SceneConvertionScripts"
copy /Y "ThirdParty\SceneConvertionScripts\vray2rpr.ms" "%DIST_PATH%\data\SceneConvertionScripts\vray2rpr.ms"

:: Copy plugin itself
xcopy %PLUGIN_PATH% "%DIST_PATH%\plug-ins\%MAX_VERSION%\*" /S /Y /I

:: Patch 3ds Max plugins configuration file
pushd Support
powershell -executionPolicy bypass -file RprSetup.ps1 %MAX_VERSION%
popd
