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
echo 5. Android / GLES3
echo q. Quit
echo -----------------------------------
set /p BuildMode="Enter selection [0-5] > "
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
if "!BuildMode!"=="5" (
    goto precheck_android
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
    -DVERSE_BUILD_3RDPARTIES=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if !BuildModeWasm!==1 (
    if exist "%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a" (
        set ThirdDepOptions=!ThirdDepOptions! ^
            -DTIFF_INCLUDE_DIR=%CurrentDir%\..\Dependencies\wasm\include ^
            -DTIFF_LIBRARY_RELEASE=%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a
    )
)

:: Fix some OpenSceneGraph compile errors
set SedEXE=%CurrentDir%\wasm\sed.exe
%SedEXE% "s/if defined(__ANDROID__)/if defined(__EMSCRIPTEN__) || defined(__ANDROID__)/g" "%OpenSceneGraphRoot%\src\osgDB\FileUtils.cpp" > FileUtils.cpp.tmp
xcopy /y FileUtils.cpp.tmp "%OpenSceneGraphRoot%\src\osgDB\FileUtils.cpp"
%SedEXE% "s/std::mem_fun_ref/std::mem_fn/g" "%OpenSceneGraphRoot%\src\osgUtil\tristripper\include\detail\graph_array.h" > graph_array.h.tmp
xcopy /y graph_array.h.tmp "%OpenSceneGraphRoot%\src\osgUtil\tristripper\include\detail\graph_array.h"
%SedEXE% "s/ADD_PLUGIN_DIRECTORY(cfg)/#ADD_PLUGIN_DIRECTORY(#cfg)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
%SedEXE% "s/ADD_PLUGIN_DIRECTORY(obj)/#ADD_PLUGIN_DIRECTORY(#obj)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
%SedEXE% "s/TIFF_FOUND AND OSG_CPP_EXCEPTIONS_AVAILABLE/TIFF_FOUND/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
%SedEXE% "s/ANDROID_3RD_PARTY()/#ANDROID_3RD_PARTY(#)/g" "%OpenSceneGraphRoot%\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\CMakeLists.txt"

:: Fix WebGL running errors
if "!BuildMode!"=="3" (
    %SedEXE% "s#dlopen(#NULL;\/\/dlopen\/\/(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
    xcopy /y DynamicLibrary.cpp.tmp "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
    %SedEXE% "s#isTexture2DArraySupported = validContext#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp" > GLExtensions.cpp.tmp
    xcopy /y GLExtensions.cpp.tmp "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
)
if "!BuildMode!"=="4" (
    %SedEXE% "s#dlopen(#NULL;\/\/dlopen\/\/(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
    xcopy /y DynamicLibrary.cpp.tmp "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
    %SedEXE% "s#isTexture2DArraySupported = validContext#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp" > GLExtensions.cpp.tmp
    xcopy /y GLExtensions.cpp.tmp "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
)
%SedEXE% "s#ifndef GL_EXT_texture_compression_s3tc#if !defined(GL_EXT_texture_compression_s3tc) || !defined(GL_EXT_texture_compression_s3tc_srgb)#g" "%OpenSceneGraphRoot%\include/osg/Texture" > Texture.tmp
xcopy /y Texture.tmp "%OpenSceneGraphRoot%\include\osg\Texture"
%SedEXE% "s#glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#;\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#g" "%OpenSceneGraphRoot%\src\osg\Texture.cpp" > Texture.cpp.tmp
xcopy /y Texture.cpp.tmp "%OpenSceneGraphRoot%\src\osg\Texture.cpp"
%SedEXE% "s#case(GL_HALF_FLOAT):#case GL_HALF_FLOAT: case 0x8D61:#g" "%OpenSceneGraphRoot%\src\osg\Image.cpp" > Image.cpp.tmp
xcopy /y Image.cpp.tmp "%OpenSceneGraphRoot%\src\osg\Image.cpp"

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

:: Android / gradle
:precheck_android
set SedEXE=%CurrentDir%\wasm\sed.exe
set Sdl2Root=%CurrentDir%\..\SDL2
set GradleLocalPropFile=%CurrentDir%\android\local.properties
set GradleSettingsFile=%CurrentDir%\android\settings.gradle

where gradle -v >nul 2>&1
if not %errorlevel%==0 (
    echo Gradle failed. Please make sure it can be found in PATH variable and JDK 1.7 set in JAVA_HOME variable.
    goto exit
)

if not exist %Sdl2Root%\ (
    echo SDL2 source folder not found. Please download and unzip it in ..\SDL2.
    goto exit
)

if not exist %GradleLocalPropFile% (
    if defined ANDROID_SDK (
        if defined ANDROID_NDK (
            set "AndroidPathSDK0=!ANDROID_SDK!"
            set "AndroidPathNDK0=!ANDROID_NDK!"
            set "AndroidPathSDK=!AndroidPathSDK0:\=/!"
            set "AndroidPathNDK=!AndroidPathNDK0:\=/!"
            @echo off
            (
                echo sdk.dir=!AndroidPathSDK!
                echo ndk.dir=!AndroidPathNDK!
            ) > "%GradleLocalPropFile%"
        ) else (
            echo Environment variable ANDROID_NDK not set. Unable to create local.properties.
            goto exit
        )
    ) else (
        echo Environment variable ANDROID_SDK not set. Unable to create local.properties.
        goto exit
    )
)

set /p AndroidCheckingFlag="Would you like to set a specific SDK version? (y/n) > "
if "!AndroidCheckingFlag!"=="y" (
    set /p BuildToolsVersion="Please set build-tools version (e.g. 32.0.0) > "
    set /p TargetSdkVersion="Please set target SDK version (e.g. 32) > "
    set /p MinimumSdkVersion="Please set minimum SDK version (e.g. 21) > "
    @echo off
    (
        echo gradle.ext.buildToolsVersion = '!BuildToolsVersion!'
        echo gradle.ext.sdkVersion = !TargetSdkVersion!
        echo gradle.ext.minSdkVersion = !MinimumSdkVersion!
        echo gradle.ext.targetSdkVersion = !TargetSdkVersion!
        echo gradle.ext.libDistributionRoot = '../build'
        echo include ':thirdparty'
        echo include ':sdl2'
        echo include ':osg'
        echo include ':osgverse'
        echo include ':app'
    ) > "%GradleSettingsFile%"
)

cd %CurrentDir%\android
gradle assembleDebug

:: TODO and exit process
:todo
echo Current option is not implemented yet. Be patient :-)

:exit
if not %errorlevel%==0 (
    echo Last error = %errorlevel%
    pause
)
cd %CurrentDir%

:: Reset some OpenSceneGraph source code
%SedEXE% "s/ADD_PLUGIN_DIRECTORY(#cfg)/#ADD_PLUGIN_DIRECTORY(cfg)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
%SedEXE% "s/ADD_PLUGIN_DIRECTORY(#obj)/#ADD_PLUGIN_DIRECTORY(obj)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
%SedEXE% "s/#ANDROID_3RD_PARTY(#)/ANDROID_3RD_PARTY()/g" "%OpenSceneGraphRoot%\CMakeLists.txt" > CMakeLists.txt.tmp
xcopy /y CMakeLists.txt.tmp "%OpenSceneGraphRoot%\CMakeLists.txt"
%SedEXE% "s#NULL;\/\/dlopen\/\/(#dlopen(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
xcopy /y DynamicLibrary.cpp.tmp "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
%SedEXE% "s#\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#;glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#g" "%OpenSceneGraphRoot%\src\osg\Texture.cpp" > Texture.cpp.tmp
xcopy /y Texture.cpp.tmp "%OpenSceneGraphRoot%\src\osg\Texture.cpp"
%SedEXE% "s#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#isTexture2DArraySupported = validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp" > GLExtensions.cpp.tmp
xcopy /y GLExtensions.cpp.tmp "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
del /Q *.tmp

echo Quited.
endlocal
pause
