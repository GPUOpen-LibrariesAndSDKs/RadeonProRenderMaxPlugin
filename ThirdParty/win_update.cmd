@echo off

set ThirdPartyDir="..\..\RadeonProRenderThirdPartyComponents"

if exist %ThirdPartyDir% ( 
	if "%1"=="update_thirdparty" (
		echo Updating %ThirdPartyDir% 
	    echo ===============================================================
	    pushd %ThirdPartyDir% 
    	git remote update
	    git status -uno
		popd
	)
	
    rd /S /Q AxfPackage
    rd /S /Q 'Expat 2.1.0'
    rd /S /Q 'RadeonProImageProcessing'
    rd /S /Q 'RadeonProRender SDK'
    rd /S /Q RadeonProRender-GLTF
    rd /S /Q json

    xcopy /S /Y /I %ThirdPartyDir%\AxfPackage\ReleaseDll\* AxfPackage\ReleaseDll
    xcopy /S /Y /I %ThirdPartyDir%\AxfPackage\AxfInclude\* AxfPackage\AxfInclude

    xcopy /S /Y /I "%ThirdPartyDir%\Expat 2.1.0\*" "Expat 2.1.0"
    xcopy /S /Y /I %ThirdPartyDir%\RadeonProImageProcessing\* RadeonProImageProcessing
    xcopy /S /Y /I "%ThirdPartyDir%\RadeonProRender SDK\*" "RadeonProRender SDK"
    xcopy /S /Y /I %ThirdPartyDir%\RadeonProRender-GLTF\* "RadeonProRender-GLTF"
    xcopy /S /Y /I %ThirdPartyDir%\json\* json

) else ( 
    echo Cannot update as %ThirdPartyDir% missing
)
