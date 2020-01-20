@echo off

setlocal enabledelayedexpansion

:gen_guid
for /f %%v in ('powershell -executionPolicy bypass -file GenGuid.ps1') do (
	set RPR_MAX_PLUGIN_GUID=%%v
)

echo Generated new GUID for installer: %RPR_MAX_PLUGIN_GUID%

:get_plugin_version
for /f %%v in ('powershell -executionPolicy bypass -file GetVersion.ps1') do (
	set MAX_PLUGIN_VERSION=%%v
)

:cleanup
del RadeonProRender3dsMax*.msi
rmdir /S /Q output system PluginInstaller\bin PluginInstaller\obj PluginInstaller\Generated

:: parse options
if "%1"=="clean" goto :eof

if "%1"=="build_installer" goto :build_installer

:build_plugin
echo Building Radeon ProRender plug-ins for 3ds Max
pushd ..
call build.cmd
popd

:build_installer
:: build an installer and patch its name with a version
echo Building Radeon ProRender for 3ds Max installer %MAX_PLUGIN_VERSION%

call build.cmd

move PluginInstaller\bin\Release\RadeonProRender3dsMax.msi "RadeonProRender3dsMax_%MAX_PLUGIN_VERSION%.msi"
