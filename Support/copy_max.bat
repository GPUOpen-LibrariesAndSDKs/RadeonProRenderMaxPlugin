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
set xCopyFlags=/S /Y

set DataFiles=^
	"Support\*.frs"^
	"Support\*.dat"

set CoreFiles=^
	"ThirdParty\RadeonProRender SDK\Win\bin\RadeonProRender64.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\RprSupport64.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\OpenImageIO.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\OpenImageIOD.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\FreeImage.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\glfw3.dll"^
	"ThirdParty\RadeonProRender SDK\Win\bin\RadeonProRenderIO.dll"

set EmbreeFiles=^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\embree.dll"^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\tbb.dll"^
	"..\RadeonProRenderPkgPlugin\MaxPkg\support\origindll\tbbmalloc.dll"

set AxfFiles=^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\AxfConverter.dll"^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\AxFDecoding_r.dll"^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\FreeImage.dll"^
	"ThirdParty\RadeonProImageProcessing\Win\lib\RadeonImageFilters64.dll"

set GltfFiles=^
	"ThirdParty\RadeonProRender-GLTF\Win\lib\ProRenderGLTF.dll"

::make distribution folder with files for installer
if not exist %DIST_PATH% md %DIST_PATH%

for %%a in (%DataFiles%) do ( 
	xcopy %%a "%DIST_PATH%\data\*" %xCopyFlags%
)

set PluginBundle=CoreFiles EmbreeFiles AxfFiles GltfFiles

for %%b in (%PluginBundle%) do (
	for %%a in (!%%b!) do ( 
		xcopy %%a "%DIST_PATH%\bin\*" %xCopyFlags%
	)
)

xcopy %PLUGIN_PATH% "%DIST_PATH%\plug-ins\%MAX_VERSION%\*" %xCopyFlags%

pushd Support
powershell -executionPolicy bypass -file RprSetup.ps1 %MAX_VERSION%
popd
