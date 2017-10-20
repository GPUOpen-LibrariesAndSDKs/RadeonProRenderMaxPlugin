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

::Copy silent, confirm overwrite and copy if different
set xCopyFlags=/S /Y

set ToMaxPluginsDir=^
	"Support\*.frs"^
	"Support\*.dat"

set ToMaxInstallDir=^
	"ThirdParty\RadeonProRender SDK\Win\bin\*.dll"^
	"ThirdParty\AxfPackage\ReleaseDll\AxfDll\*.dll"^
	"ThirdParty\RadeonProRender-GLTF\lib\ProRenderGLTF.dll"

::Copy libraries to max plugins directory (to debug it easy)
for %%a in (%ToMaxPluginsDir%) do (
	::echo Copying: "%%a" to "%MAX_PLUGINS_DIR%"
    xcopy "%%a" "%MAX_PLUGINS_DIR%\*" %xCopyFlags%
)

::Copy libraries to max install directory (to debug it easy)
if NOT DEFINED SKIP_FR_DLLS_INSTALL for %%a in (%ToMaxInstallDir%) do (
	::echo Copying: "%%a" to "%MAX_INSTALL_DIR%"
    xcopy "%%a" "%MAX_INSTALL_DIR%\*" %xCopyFlags%
)

::make distribution folder with files for installer
if "%CONFIG_NAME%"=="Release-%MAX_VERSION%" (
	IF exist "dist\%CONFIG_NAME%" RMDIR /q /s "dist"

	for %%a in (%ToMaxInstallDir%) do ( 
		::echo Copying: "%%a" to "dist"
		xcopy "%%a" "dist\*" %xCopyFlags%
	)

	md dist\plugins

	::Copy plugin library
	xcopy "%SRC_PATH%" "dist\plugins\RadeonProRender%MAX_VERSION%.dlr*" %xCopyFlags%

	for %%a in (%ToMaxPluginsDir%) do (
		::echo Copying: "%%a" to "dist\plugins\"
    	xcopy "%%a" "dist\plugins\*" %xCopyFlags%
	)
)
