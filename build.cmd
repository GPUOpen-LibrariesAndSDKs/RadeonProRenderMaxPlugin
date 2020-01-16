@echo off

set maxVersions=2018 2019 2020

setlocal enabledelayedexpansion

:vs_setup

call vs_path.bat

:: VS not detected
if %vs_ver%=="" goto :vs_error

:: check VS version
set vs_major=%vs_ver:~0,2%

if %vs_major%==14 (
	echo Visual Studio 2015 is installed.

	pushd "%VS140COMNTOOLS%..\..\VC"
	call vcvarsall.bat amd64
	popd
	
	goto :build_plugin
)

set vs17=""

if %vs_major%==15 (
	echo Visual Studio 2017 is installed.
	echo "%vs_dir%"

	echo Trying to setup toolset 14 [Visual Studio 2015] of Visual Studio 2017.

	set vs17="%vs_dir%\VC\Auxiliary\Build\vcvarsall.bat"

	pushd .	
	call !vs17! amd64 -vcvars_ver=14.0
	popd

	goto :build_plugin
)

:vs_error
	echo Visual Studio 2015 or newer has to be installed for 3ds Max 2017-2019.
	echo Visual Studio 2017 or newer has to be installed for 3ds Max 2020.
	echo Newer version of Visual Studio will be used if it's present.
	echo Platform toolset v140 has to be installed for 3ds Max 2017-2019
	echo Platform toolset v141 has to be installed for 3ds Max 2020
	goto :eof

:build_plugin

:cleanup
rmdir /S /Q bin Debug dist

for %%a in (%maxVersions%) do (
	set maxSDK=ADSK_3DSMAX_SDK_%%a

	if not defined !maxSDK! (
		echo !maxSDK! is not set. Skip.
	) else (
		echo !maxSDK! is set. Build.
		msbuild RadeonProRenderMaxPlugin.sln /property:Configuration=Release-%%a /property:Platform=x64
	)
)
