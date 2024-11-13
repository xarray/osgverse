@echo off
setlocal enabledelayedexpansion
set BuildMode=""
set BuildModeWasm=0
set CurrentDir=%cd%
set OpenSceneGraphRoot=%CurrentDir%\..\OpenSceneGraph

where ninja --version >nul 2>&1
if not %errorlevel%==0 (
    echo Ninja not found. Please make sure it can be found in PATH variable.
    goto exit
)

where cmake --version >nul 2>&1
if not %errorlevel%==0 (
    echo CMake not found. Please make sure it can be found in PATH variable.
    goto exit
)

if not exist %OpenSceneGraphRoot%\ (
    echo OSG source folder not found. Please download and unzip it in ..\OpenSceneGraph.
    goto exit
)

echo How do you like to compile OSG and osgVerse?
echo -----------------------------------
echo Please Select:
echo 0. Desktop / OpenGL Compatible Mode
echo 1. Desktop / OpenGL Core Mode
echo 2. Desktop / Google Angle
echo 3. WASM / WebGL 1.0
echo 4. WASM / WebGL 2.0 (optional with osgEarth)
echo q. Quit
echo -----------------------------------
set /p BuildMode="Enter selection [0-4] > "
if "!BuildMode!"=="0" (
    :: TODO
    goto todo
)
if "!BuildMode!"=="1" (
    :: TODO
    goto todo
)
if "!BuildMode!"=="2" (
    :: TODO
    goto todo
)
if "!BuildMode!"=="3" (
    set BuildResultChecker=build\sdk_wasm\lib\libosgviewer.a
    set CMakeResultChecker=build\osg_wasm\CMakeCache.txt
    set BuildModeWasm=1
    goto precheck
)
if "!BuildMode!"=="4" (
    set BuildResultChecker=build\sdk_wasm2\lib\libosgviewer.a
    set CMakeResultChecker=build\osg_wasm2\CMakeCache.txt
    set BuildModeWasm=1
    goto precheck
)
if "!BuildMode!"=="q" (
    goto exit
)
echo Invalid option selected.
pause
goto exit

:: Check if CMake is already configured, or OSG is already built
:precheck
set SkipOsgBuild="0"
set UseWasmOption=1
if exist %CurrentDir%\%BuildResultChecker% (
    set SkipOsgBuild="1"
    set /p RebuildFlag="Would you like to use current OSG built? (y/n) > "
    if "!RebuildFlag!"=="n" set SkipOsgBuild="0"
)

set BasicCmakeOptions=-GNinja -DCMAKE_BUILD_TYPE=Release
if !BuildModeWasm!==1 (
    :: WASM (WebGL 1 and WebGL 2)
    if not defined EMSDK (
        echo EMSDK variable not found. Please download Emscripten and run 'emsdk_env.bat' before current work.
        goto exit
    )
    if not exist %EMSDK%\ (
        echo EMSDK folder not found. Please download Emscripten and run 'emsdk_env.bat' before current work.
        goto exit
    )

    set /p Wasm64Flag="Would you like to use WASM 64bit (experimental)? (y/n) > "
    if "!Wasm64Flag!"=="y" set UseWasmOption=2

    set EmsdkToolchain="%EMSDK%\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake"
    set ThirdPartyBuildDir="%CurrentDir%\build\3rdparty_wasm"
)
if not exist %ThirdPartyBuildDir%\ mkdir %ThirdPartyBuildDir%

:: Compile 3rdparties
echo *** Building 3rdparty libraries...
set ExtraOptions=""
set ExtraOptions2=""
if !BuildModeWasm!==1 (
    if not !SkipOsgBuild!=="1" (
        cd %ThirdPartyBuildDir%
        cmake %BasicCmakeOptions% -DCMAKE_TOOLCHAIN_FILE="%EmsdkToolchain%" -DUSE_WASM_OPTIONS=!UseWasmOption! "%CurrentDir%\helpers\toolchain_builder"
        cmake --build .
        if not %errorlevel%==0 goto exit
    )
)

set ThirdDepOptions=%BasicCmakeOptions% ^
    -DFREETYPE_INCLUDE_DIR_freetype2=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
    -DFREETYPE_INCLUDE_DIR_ft2build=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
    -DFREETYPE_LIBRARY_RELEASE=%ThirdPartyBuildDir%\freetype\libfreetype.a ^
    -DJPEG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\jpeg ^
    -DJPEG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\jpeg\libjpeg.a ^
    -DPNG_PNG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\png ^
    -DPNG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\png\libpng.a ^
    -DZLIB_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\zlib ^
    -DZLIB_LIBRARY_RELEASE=%ThirdPartyBuildDir%\zlib\libzlib.a ^
    -DVERSE_BUILD_3RDPARTIES=OFF
if !BuildModeWasm!==1 (
    if exist "%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a" (
        set ThirdDepOptions=!ThirdDepOptions! ^
            -DTIFF_INCLUDE_DIR=%CurrentDir%\..\Dependencies\wasm\include ^
            -DTIFF_LIBRARY_RELEASE=%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a
    )
)

:: Fix some OpenSceneGraph compile errors
call :replaceInFile "%OpenSceneGraphRoot%\src\osgDB\FileUtils.cpp" "FileUtils.cpp.tmp" "if defined(__ANDROID__)" "if defined(__EMSCRIPTEN__) || defined(__ANDROID__)"
call :replaceInFile "%OpenSceneGraphRoot%\src\osgUtil\tristripper\include\detail\graph_array.h" "graph_array.h.tmp" "std::mem_fun_ref" "std::mem_fn"
call :replaceInFile "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" "CMakeLists.txt.tmp" "ADD_PLUGIN_DIRECTORY(cfg)" "#ADD_PLUGIN_DIRECTORY(#cfg)"
call :replaceInFile "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" "CMakeLists.txt.tmp" "ADD_PLUGIN_DIRECTORY(obj)" "#ADD_PLUGIN_DIRECTORY(#obj)"
call :replaceInFile "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" "CMakeLists.txt.tmp" "TIFF_FOUND AND OSG_CPP_EXCEPTIONS_AVAILABLE" "TIFF_FOUND"
call :replaceInFile "%OpenSceneGraphRoot%\CMakeLists.txt" "CMakeLists.txt.tmp" "ANDROID_3RD_PARTY()" "#ANDROID_3RD_PARTY(#)"

:: Compile OpenSceneGraph
echo *** Building OpenSceneGraph...
if "!BuildMode!"=="3" (
    if not exist %CurrentDir%\build\osg_wasm\ mkdir %CurrentDir%\build\osg_wasm
    set ExtraOptions=-DCMAKE_TOOLCHAIN_FILE="%EmsdkToolchain%" ^
        -DCMAKE_INCLUDE_PATH=%CurrentDir%\helpers\toolchain_builder\opengl ^
        -DCMAKE_INSTALL_PREFIX=%CurrentDir%\build\sdk_wasm ^
        -DUSE_WASM_OPTIONS=!UseWasmOption! ^
        -DOSG_SOURCE_DIR=%OpenSceneGraphRoot% ^
        -DOSG_BUILD_DIR=%CurrentDir%\build\osg_wasm\osg
    if not !SkipOsgBuild!=="1" (
        cd %CurrentDir%\build\osg_wasm
        cmake !ThirdDepOptions! !ExtraOptions! %CurrentDir%\helpers\osg_builder\wasm
        cmake --build . --target install --config Release
        if not %errorlevel%==0 goto exit
    )
)
if "!BuildMode!"=="4" (
    if not exist %CurrentDir%\build\osg_wasm2\ mkdir %CurrentDir%\build\osg_wasm2
    set ExtraOptions=-DCMAKE_TOOLCHAIN_FILE="%EmsdkToolchain%" ^
        -DCMAKE_INCLUDE_PATH=%CurrentDir%\helpers\toolchain_builder\opengl ^
        -DCMAKE_INSTALL_PREFIX=%CurrentDir%\build\sdk_wasm2 ^
        -DUSE_WASM_OPTIONS=!UseWasmOption! ^
        -DOSG_SOURCE_DIR=%OpenSceneGraphRoot% ^
        -DOSG_BUILD_DIR=%CurrentDir%\build\osg_wasm2\osg
    if not !SkipOsgBuild!=="1" (
        cd %CurrentDir%\build\osg_wasm2
        cmake !ThirdDepOptions! !ExtraOptions! %CurrentDir%\helpers\osg_builder\wasm2
        cmake --build . --target install --config Release
        if not %errorlevel%==0 goto exit
    )
)

:: Build osgEarth (Optional)
set WithOsgEarth=0
if "!BuildMode!"=="4" (
    if exist %CurrentDir%\..\osgearth-wasm\ (
        echo *** Building osgEarth 2.10...
        if not exist %CurrentDir%\build\osgearth_wasm2\ mkdir %CurrentDir%\build\osgearth_wasm2
        set ExtraOptions2=-DOSG_DIR=%CurrentDir%\build\sdk_wasm2 ^
            -DTHIRDPARTY_ROOT=%CurrentDir%\..\Dependencies\wasm ^
            -DUSE_WASM_OPTIONS=!UseWasmOption! ^
            -DOSGEARTH_SOURCE_DIR=%CurrentDir%\..\osgearth-wasm ^
            -DOSGEARTH_BUILD_DIR=%CurrentDir%\build\osgearth_wasm2\osgearth
        cd %CurrentDir%\build\osgearth_wasm2
        cmake %BasicCmakeOptions% !ExtraOptions! !ExtraOptions2! %CurrentDir%\helpers\osg_builder\wasm2_oe
        cmake --build . --target install --config Release
        set WithOsgEarth=1
    ) else (
        echo osgEarth-WASM not found. Please download it and unzip in ..\osgearth-wasm if you wish.
    )
)

:: Build osgVerse
echo *** Building osgVerse...
set OsgRootLocation=""
if "!BuildMode!"=="3" (
    if not exist %CurrentDir%\build\verse_wasm\ mkdir %CurrentDir%\build\verse_wasm
    set OsgRootLocation=%CurrentDir%\build\sdk_wasm
    cd %CurrentDir%\build\verse_wasm
    cmake !ThirdDepOptions! !ExtraOptions! -DUSE_WASM_OPTIONS=!UseWasmOption! -DOSG_ROOT="!OsgRootLocation!" %CurrentDir%
    cmake --build . --target install --config Release
    if not %errorlevel%==0 goto exit
)
if "!BuildMode!"=="4" (
    if not exist %CurrentDir%\build\verse_wasm2\ mkdir %CurrentDir%\build\verse_wasm2
    set OsgRootLocation=%CurrentDir%\build\sdk_wasm2
    cd %CurrentDir%\build\verse_wasm2
    cmake !ThirdDepOptions! !ExtraOptions! -DUSE_WASM_OPTIONS=!UseWasmOption! -DUSE_WASM_OSGEARTH=!WithOsgEarth! -DOSG_ROOT="!OsgRootLocation!" %CurrentDir%
    cmake --build . --target install --config Release
    if not %errorlevel%==0 goto exit
)
goto exit

:replaceInFile
set "source_file=%~1"
set "dest_file=%~2"
set "search_string=%~3"
set "replace_string=%~4"
if not exist "!source_file!" goto :eof
if exist "!dest_file!" del /f "!dest_file!"
for /f "delims=" %%a in ('type "!source_file!"') do (
    set "line=%%a"
    set "modified=!line:%search_string%=%replace_string%!"
    if "!line!" neq "!modified!" (
        @echo !modified! >> "!dest_file!"
    ) else (
        @echo !line! >> "!dest_file!"
    )
)
xcopy /y "!dest_file!" "%~1"
goto :eof

:: TODO and exit process
:todo
echo Current option is not implemented yet. Be patient :-)

:exit
if not %errorlevel%==0 echo Last error = %errorlevel%
endlocal
cd %CurrentDir%
echo Quited.
pause
