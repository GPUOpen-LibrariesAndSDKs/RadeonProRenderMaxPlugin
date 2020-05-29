# Radeon ProRender Max Plugin

Development requires 3dsMax 2019 or later.

## Build Environment

Projects expect to find Max SDKs and binaries via environment variables (if they are not already present on your dev machine, 
set them to appropriate values)

Samples:
	ADSK_3DSMAX_SDK_2019=C:\Program Files\Autodesk\3ds Max 2019 SDK\maxsdk
	ADSK_3DSMAX_SDK_2020=C:\Program Files\Autodesk\3ds Max 2020 SDK\maxsdk
	ADSK_3DSMAX_SDK_2021=C:\Program Files\Autodesk\3ds Max 2021 SDK\maxsdk
	ADSK_3DSMAX_x64_2019=C:\Program Files\Autodesk\3ds Max 2019\
	ADSK_3DSMAX_x64_2020=C:\Program Files\Autodesk\3ds Max 2020\
	ADSK_3DSMAX_x64_2020=C:\Program Files\Autodesk\3ds Max 2021\

## ThirdParty libraries

Plugin includes 3 submodules:
RadeonProRender SDK:
git@github.com:Radeon-Pro/RadeonProRenderSDK.git

Image Processing Library:
git@github.com:Radeon-Pro/RadeonProImageProcessingSDK.git

Shared Components:
git@github.com:Radeon-Pro/RadeonProRenderSharedComponents.git

All of them are included via SSH protocol. You will need to create and install SSH keys

`git submodule update --init -f --recursive`

## To compile a release

Run the build (an appropriate solution's configuration should be selected according to an installed 3ds Max SDK). At the end of the
build a batch file will be run to create the "dist" directory which contains the binaries required for the max plugin. If the build
process has been finished successfully 3ds Max configuration file will be patched to point to appropriate location of the created plugin.

## Debugging

Once the project has been built
	Verify (or set) the paths in your Project->Debugging settings. Note that these values are stored in ".user" settings file.
	Set appropriate paths for different configurations, e.g. for Debug-2019 you will probably need:
		Debugging->Command = C:\Program Files\Autodesk\3ds Max 2019\3dsmax.exe
		Debugging->Working directory = C:\Program Files\Autodesk\3ds Max 2019

Now run using F5, and Max should launch, hopefully loading the plugin. To test if plugin binary is being detected, you can put a breakpoint
on LibInitialize() function in main.cpp, and it should be hit during Max startup.

## MSVS project & compilation notes

- "Multithreaded DLL" runtime library must be used, even in debug configuration. 3ds Max randomly segfaults otherwise. This 
  means STL containers will not perform any safety checks in debug mode, and are therefore unsafe to use. We use our own 
  containers isntead
- We add this to the environment _NO_DEBUG_HEAP=1 - should we not do it, memory allocations/deallocations in debug mode with 
  debugger present would be EXTREMELY slow

## Versioning

We maintain a version string in the plugin that must be bumped when we do a new release. The version numbers are located in the
.\version.h file and these are post processed into the .rc file for the plugin. A build script will update the build number 
everytime there is commit to the master branch. So it is only necessary to change the major and minor numbers as required when
there is a new release.

## To run autotests

1. Prepare test scenes. Each one goes into a separate folder, and needs to be named "scene.max".
2. In 3ds Max, go to command panel -> utilities -> more -> Autotesting tool
3. Set up parameters (how many passes to render in each scene, how many times to repeat tests (for memory leak hunting), ...) and click go.
Select folder with the tests on HDD. 
