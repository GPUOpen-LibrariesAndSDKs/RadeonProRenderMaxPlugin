@echo off

REM Post build event that copies RPR plugin to 3ds max 
REM %1 = config name (eg "Debug-2016")

pushd "%~dp0.."

if not defined MAX_VERSION (
	echo MAX_VERSION not defined
	exit(1)
)

set CONFIG_NAME=%~1

echo ========================
echo Running Post-build Steps 
echo ========================
echo	Configuration: %CONFIG_NAME% 
echo	Max version: %MAX_VERSION%

REM the following call allows for some extra indirection, 2 escape passes

call set MAX_INSTALL_DIR=%%ADSK_3DSMAX_x64_%MAX_VERSION%%%

REM and trunctate the trailing slash

set MAX_INSTALL_DIR=%MAX_INSTALL_DIR:~0,-1%

echo Installing to %MAX_INSTALL_DIR%

if NOT DEFINED MAX_PLUGINS_DIR set MAX_PLUGINS_DIR=%MAX_INSTALL_DIR%\plugins

set SRC_PATH=bin\x64\%CONFIG_NAME%\RadeonProRenderMaxPlugin.dll
set DST_PATH=%MAX_PLUGINS_DIR%\RadeonProRender%MAX_VERSION%.dlr

echo Copying: %SRC_PATH% to %DST_PATH%

copy "%SRC_PATH%" "%DST_PATH%"

copy "Support\*.frs" "%MAX_PLUGINS_DIR%\*"
copy "Support\*.dat" "%MAX_PLUGINS_DIR%\*"

if NOT DEFINED SKIP_FR_DLLS_INSTALL copy "ThirdParty\RadeonProRender SDK\Win\bin\*.dll" "%MAX_INSTALL_DIR%\*"

if NOT DEFINED SKIP_FR_DLLS_INSTALL copy "ThirdParty\AxfPackage\ReleaseDll\AxfDll\*.dll" "%MAX_INSTALL_DIR%\*"
if NOT DEFINED SKIP_FR_DLLS_INSTALL copy "ThirdParty\AxfPackage\ReleaseDll\AxfDll\*.def" "%MAX_INSTALL_DIR%\*"

if "%CONFIG_NAME%"=="Release-%MAX_VERSION%" (
	IF exist "dist\%CONFIG_NAME%" RMDIR /q /s "dist"

	copy "ThirdParty\RadeonProRender SDK\Win\bin\*.dll" "dist\*"
	copy "ThirdParty\AxfPackage\ReleaseDll\AxfDll\*.dll" "dist\*"
	copy "ThirdParty\RadeonProRender-GLTF\lib\ProRenderGLTF.dll" "dist\*"
	echo Hello world > "dist\copy to max exe folder"

	md dist\plugins
	copy "%SRC_PATH%" "dist\plugins\RadeonProRender%MAX_VERSION%.dlr"
	copy "Support\*.frs" "dist\plugins\*"
	copy "Support\*.dat" "dist\plugins\*"
)
