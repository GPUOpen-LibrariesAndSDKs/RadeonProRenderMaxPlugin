:: Copyright 2020 Advanced Micro Devices, Inc
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::     http://www.apache.org/licenses/LICENSE-2.0
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

@echo off

setlocal enabledelayedexpansion

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
