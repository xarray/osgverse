@echo off
setlocal enabledelayedexpansion
chcp 65001

set BuildMode=""
set BuildGles2=0
set BuildModeWasm=0
set SourceCodePatched=0
set CurrentDir=%cd%
set OpenSceneGraphRoot=%CurrentDir%\..\OpenSceneGraph

set OptionalDir=%1
if not "!OptionalDir!"=="" (
    set "OptionalDir=!OptionalDir:\=/!"
    set "LastCharOfOptionalDir=!OptionalDir:~-1!"
    if "!LastCharOfOptionalDir!"=="/" (set "OptionalDir=!OptionalDir:~0,-1!")
)
set GLES_LibPath="!OptionalDir!/libGLESv2.lib"
set EGL_LibPath="!OptionalDir!/libEGL.lib"

where cmake --version >nul 2>&1
if not %errorlevel%==0 (
    echo CMake not found. Please make sure it can be found in PATH variable.
    goto exit
)

if not exist %OpenSceneGraphRoot%\ (
    git clone https://gitee.com/mirrors/OpenSceneGraph.git %OpenSceneGraphRoot%\
    if not exist %OpenSceneGraphRoot%\ (
        echo OSG source folder not found. Please download and unzip it in ..\OpenSceneGraph.
        goto exit
    )
)

echo How do you like to compile OSG and osgVerse?
echo -----------------------------------
echo Please Select:
echo 0. Desktop / OpenGL Compatible Mode
echo 1. Desktop / OpenGL Core Mode
echo 2. Desktop / OpenGL ES
echo 3. WASM / WebGL 1.0
echo 4. WASM / WebGL 2.0 (optional with osgEarth)
echo 5. Android / OpenGLES 3
echo q. Quit
echo -----------------------------------
set /p BuildMode="Enter selection [0-5] > "
if "!BuildMode!"=="0" (
    set BuildResultChecker=build\sdk_def\lib\osgViewer.lib
    set CMakeResultChecker=build\osg_def\CMakeCache.txt
    goto precheck
)
if "!BuildMode!"=="1" (
    set BuildResultChecker=build\sdk_core\lib\osgViewer.lib
    set CMakeResultChecker=build\osg_core\CMakeCache.txt
    goto precheck
)
if "!BuildMode!"=="2" (
    set BuildResultChecker=build\sdk_es\lib\osgViewer.lib
    set CMakeResultChecker=build\osg_es\CMakeCache.txt

    if not exist %GLES_LibPath% (
        echo "libGLESv2.lib not found. Please run as follows: ./Setup.sh <path_of_libGLES>"
        goto exit
    )
    if not exist %EGL_LibPath% (
        echo "libEGL.lib not found. Please run as follows: ./Setup.sh <path_of_libEGL>"
        goto exit
    )
    goto precheck
)
if "!BuildMode!"=="3" (
    set BuildResultChecker=build\sdk_wasm\lib\libosgViewer.a
    set CMakeResultChecker=build\osg_wasm\CMakeCache.txt
    set BuildModeWasm=1
    goto precheck
)
if "!BuildMode!"=="4" (
    set BuildResultChecker=build\sdk_wasm2\lib\libosgViewer.a
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
    set /p RebuildFlag="Would you like to use current OSG built (default: yes)? (y/n) > "
    if /i "!RebuildFlag!"=="n" set SkipOsgBuild="0"
)

set BasicCmakeOptions=""
ver > nul
if !BuildModeWasm!==0 (
    where nmake /? >nul 2>&1
    if not %errorlevel%==0 (
        echo %errorlevel% NMake not found. Please start from Developer Command Prompt of Visual Studio.
        goto exit
    )

    :: Desktop build
    set /p DebugLibFlag="Would you like to build Debug libraries (default: Release)? (y/n) > "
    if /i "!DebugLibFlag!"=="y" (
        set BasicCmakeOptions=-G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
    ) else (
        set BasicCmakeOptions=-G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
    )
    set ThirdPartyBuildDir="%CurrentDir%\build\3rdparty"

    if "!BuildMode!"=="2" (
        :: OpenGL ES
        if not !SkipOsgBuild!=="1" (
            set /p Gles2Flag="Would you like to compile GLES2 version (default: GLES3)? (y/n) > "
            if /i "!Gles2Flag!"=="y" set BuildGles2=1
        )
    )
)
if !BuildModeWasm!==1 (
    where ninja --version >nul 2>&1
    if not %errorlevel%==0 (
        echo Ninja not found. Please make sure it can be found in PATH variable.
        goto exit
    )

    :: WASM (WebGL 1 and WebGL 2)
    if not defined EMSDK (
        echo EMSDK variable not found. Please download Emscripten and run 'emsdk_env.bat' before current work.
        goto exit
    )
    if not exist %EMSDK%\ (
        echo EMSDK folder not found. Please download Emscripten and run 'emsdk_env.bat' before current work.
        goto exit
    )

    set /p Wasm64Flag="Would you like to use WASM 64bit (experimental, default: no)? (y/n) > "
    if /i "!Wasm64Flag!"=="y" set UseWasmOption=2

    set BasicCmakeOptions=-GNinja -DCMAKE_BUILD_TYPE=Release
    set EmsdkToolchain="%EMSDK%\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake"
    set ThirdPartyBuildDir="%CurrentDir%\build\3rdparty_wasm"
)

:: Compile 3rdparties
echo *** Building 3rdparty libraries...
if not exist %ThirdPartyBuildDir%\ mkdir %ThirdPartyBuildDir%
set ExtraOptions=""
set ExtraOptions2=""
if !BuildModeWasm!==0 (
    if not !SkipOsgBuild!=="1" (
        cd %ThirdPartyBuildDir%
        cmake %BasicCmakeOptions% "%CurrentDir%\helpers\toolchain_builder"
        cmake --build .
        if not !errorlevel! == 0 (goto exit)
    )
)
if !BuildModeWasm!==1 (
    if not !SkipOsgBuild!=="1" (
        cd %ThirdPartyBuildDir%
        cmake %BasicCmakeOptions% -DCMAKE_TOOLCHAIN_FILE="%EmsdkToolchain%" -DUSE_WASM_OPTIONS=!UseWasmOption! "%CurrentDir%\helpers\toolchain_builder"
        cmake --build .
        if not !errorlevel! == 0 (goto exit)
    )
)

set ThirdDepOptions=%BasicCmakeOptions% -DVERSE_BUILD_3RDPARTIES=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if !BuildModeWasm!==0 (
    set ThirdDepOptions=!ThirdDepOptions! ^
        -DFREETYPE_INCLUDE_DIR_freetype2=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
        -DFREETYPE_INCLUDE_DIR_ft2build=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
        -DFREETYPE_LIBRARY_RELEASE=%ThirdPartyBuildDir%\freetype\freetype.lib ^
        -DJPEG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\jpeg ^
        -DJPEG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\jpeg\jpeg.lib ^
        -DPNG_PNG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\png ^
        -DPNG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\png\png.lib ^
        -DZLIB_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\zlib ^
        -DZLIB_LIBRARY_RELEASE=%ThirdPartyBuildDir%\zlib\zlib.lib ^
        -DTIFF_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\tiff ^
        -DTIFF_LIBRARY_RELEASE=%ThirdPartyBuildDir%\tiff\libtiff.a
)
if !BuildModeWasm!==1 (
    set ThirdDepOptions=!ThirdDepOptions! ^
        -DFREETYPE_INCLUDE_DIR_freetype2=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
        -DFREETYPE_INCLUDE_DIR_ft2build=%CurrentDir%\helpers\toolchain_builder\freetype\include ^
        -DFREETYPE_LIBRARY_RELEASE=%ThirdPartyBuildDir%\freetype\libfreetype.a ^
        -DJPEG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\jpeg ^
        -DJPEG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\jpeg\libjpeg.a ^
        -DPNG_PNG_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\png ^
        -DPNG_LIBRARY_RELEASE=%ThirdPartyBuildDir%\png\libpng.a ^
        -DZLIB_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\zlib ^
        -DZLIB_LIBRARY_RELEASE=%ThirdPartyBuildDir%\zlib\libzlib.a
    if exist "%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a" (
        set ThirdDepOptions=!ThirdDepOptions! ^
            -DTIFF_INCLUDE_DIR=%CurrentDir%\..\Dependencies\wasm\include ^
            -DTIFF_LIBRARY_RELEASE=%CurrentDir%\..\Dependencies\wasm\lib\libtiff.a
    )
)

:: Fix some OpenSceneGraph compile errors
if not !SkipOsgBuild!=="1" (
    echo *** Automatically patching source code...
    set SourceCodePatched=1
    set SedEXE=%CurrentDir%\wasm\sed.exe
    if exist "%OpenSceneGraphRoot%\src\osgUtil\tristripper\include\detail\graph_array.h" (
        %SedEXE% -i.bak "s/std::mem_fun_ref/std::mem_fn/g" "%OpenSceneGraphRoot%\src\osgUtil\tristripper\include\detail\graph_array.h"
    )
    %SedEXE% -i.bak "s/if defined(__ANDROID__)/if defined(__EMSCRIPTEN__) || defined(__ANDROID__)/g" "%OpenSceneGraphRoot%\src\osgDB\FileUtils.cpp"
    %SedEXE% -i.bak "s/TARGET_EXTERNAL_LIBRARIES ${FREETYPE_LIBRARIES}/TARGET_EXTERNAL_LIBRARIES ${PNG_LIBRARY} ${FREETYPE_LIBRARIES}/g" "%OpenSceneGraphRoot%\src\osgPlugins\freetype\CMakeLists.txt"
    %SedEXE% -i.bak "s/ADD_PLUGIN_DIRECTORY(cfg)/#ADD_PLUGIN_DIRECTORY(#cfg)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
    %SedEXE% -i.bak "s/ADD_PLUGIN_DIRECTORY(obj)/#ADD_PLUGIN_DIRECTORY(#obj)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
    %SedEXE% -i.bak "s/TIFF_FOUND AND OSG_CPP_EXCEPTIONS_AVAILABLE/TIFF_FOUND/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
    %SedEXE% -i.bak "s/IF(WIN32 AND NOT ANDROID)/IF(${OSG_WINDOWING_SYSTEM} STREQUAL \"Win32\" AND WIN32 AND NOT ANDROID)/g" "%OpenSceneGraphRoot%\src\osgViewer\CMakeLists.txt"
    %SedEXE% -i.bak "s/ANDROID_3RD_PARTY()/#ANDROID_3RD_PARTY(#)/g" "%OpenSceneGraphRoot%\CMakeLists.txt"

    :: Fix WebGL running errors
    if "!BuildMode!"=="3" (
        %SedEXE% -i.bak "s#dlopen(#NULL;\/\/dlopen\/\/(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
        %SedEXE% -i.bak "s#isTexture2DArraySupported = validContext#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
    )
    if "!BuildMode!"=="4" (
        %SedEXE% -i.bak "s#dlopen(#NULL;\/\/dlopen\/\/(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
        %SedEXE% -i.bak "s#isTexture2DArraySupported = validContext#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
    )
    %SedEXE% -i.bak "s#ifndef GL_EXT_texture_compression_s3tc#if defined(GL_EXT_texture_compression_s3tc)==0 || defined(GL_EXT_texture_compression_s3tc_srgb)==0#g" "%OpenSceneGraphRoot%\include/osg/Texture"
    %SedEXE% -i.bak "s#glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#;\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#g" "%OpenSceneGraphRoot%\src\osg\Texture.cpp"
    %SedEXE% -i.bak "s#case(GL_HALF_FLOAT):#case GL_HALF_FLOAT: case 0x8D61:#g" "%OpenSceneGraphRoot%\src\osg\Image.cpp"
)

:: Compile OpenSceneGraph
echo *** Building OpenSceneGraph...
if "!BuildMode!"=="0" (
    :: OpenGL Compatible Profile
    if not exist %CurrentDir%\build\osg_def\ mkdir %CurrentDir%\build\osg_def
    set ExtraOptions=-DCMAKE_INSTALL_PREFIX=%CurrentDir%\build\sdk
    if not !SkipOsgBuild!=="1" (
        cd %CurrentDir%\build\osg_def
        cmake !ThirdDepOptions! !ExtraOptions! %OpenSceneGraphRoot%
        cmake --build . --target install --config Release
        if not !errorlevel! == 0 (goto exit)
    )
)
if "!BuildMode!"=="1" (
    :: OpenGL Core Profile
    if not exist %CurrentDir%\build\osg_core\ mkdir %CurrentDir%\build\osg_core
    set ExtraOptions=-DOPENGL_PROFILE=GLCORE ^
        -DGLCORE_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\opengl ^
        -DOPENGL_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\opengl ^
        -DCMAKE_INSTALL_PREFIX=%CurrentDir%\build\sdk_core
    if not !SkipOsgBuild!=="1" (
        cd %CurrentDir%\build\osg_core
        echo "cmake !ThirdDepOptions! !ExtraOptions! %OpenSceneGraphRoot%"
        cmake !ThirdDepOptions! !ExtraOptions! %OpenSceneGraphRoot%
        cmake --build . --target install --config Release
        if not !errorlevel! == 0 (goto exit)
    )
)
if "!BuildMode!"=="2" (
    :: OpenGL ES
    if not exist %CurrentDir%\build\osg_es\ mkdir %CurrentDir%\build\osg_es
    set ExtraOptions=-DOSG_WINDOWING_SYSTEM=None ^
        -DOPENGL_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\opengl ^
        -DEGL_LIBRARY=%EGL_LibPath% -DOPENGL_gl_LIBRARY=%GLES_LibPath% ^
        -DCMAKE_INSTALL_PREFIX=%CurrentDir%\build\sdk_es
    if not !SkipOsgBuild!=="1" (
        cd %CurrentDir%\build\osg_es
        if !BuildGles2!==1 (
            cmake !ThirdDepOptions! !ExtraOptions! -DOPENGL_PROFILE=GLES2 %OpenSceneGraphRoot%
        ) else (
            cmake !ThirdDepOptions! !ExtraOptions! -DOPENGL_PROFILE=GLES3 %OpenSceneGraphRoot%
        )
        cmake --build . --target install --config Release
        if not !errorlevel! == 0 (goto exit)
    )
)
if "!BuildMode!"=="3" (
    :: WASM toolchain: WebGL 1
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
        if not !errorlevel! == 0 (goto exit)
    )
)
if "!BuildMode!"=="4" (
    :: WASM toolchain: WebGL 2
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
        if not !errorlevel! == 0 (goto exit)
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
if "!BuildMode!"=="0" (
    :: OpenGL Compatible Profile
    if not exist %CurrentDir%\build\verse_def\ mkdir %CurrentDir%\build\verse_def
    cd %CurrentDir%\build\verse_def
    cmake !ThirdDepOptions! !ExtraOptions! -DOSG_ROOT="%CurrentDir%\build\sdk" %CurrentDir%
    cmake --build . --target install --config Release
    if not !errorlevel! == 0 (goto exit)
)
if "!BuildMode!"=="1" (
    :: OpenGL Core Profile
    if not exist %CurrentDir%\build\verse_core\ mkdir %CurrentDir%\build\verse_core
    cd %CurrentDir%\build\verse_core
    set ExtraOptions2=-DOPENGL_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\opengl
    cmake !ThirdDepOptions! !ExtraOptions! !ExtraOptions2! -DOSG_ROOT="%CurrentDir%\build\sdk_core" %CurrentDir%
    cmake --build . --target install --config Release
    if not !errorlevel! == 0 (goto exit)
)
if "!BuildMode!"=="2" (
    :: OpenGL ES
    if not exist %CurrentDir%\build\verse_es\ mkdir %CurrentDir%\build\verse_es
    cd %CurrentDir%\build\verse_es
    set ExtraOptions2=-DOPENGL_INCLUDE_DIR=%CurrentDir%\helpers\toolchain_builder\opengl ^
                      -DOSG_EGL_LIBRARY=%EGL_LibPath% -DOSG_GLES_LIBRARY=%GLES_LibPath%
    cmake !ThirdDepOptions! !ExtraOptions! !ExtraOptions2! -DOSG_ROOT="%CurrentDir%\build\sdk_es" %CurrentDir%
    cmake --build . --target install --config Release
    if not !errorlevel! == 0 (goto exit)
)
if "!BuildMode!"=="3" (
    :: WASM toolchain: WebGL 1
    if not exist %CurrentDir%\build\verse_wasm\ mkdir %CurrentDir%\build\verse_wasm
    set OsgRootLocation=%CurrentDir%\build\sdk_wasm
    cd %CurrentDir%\build\verse_wasm
    cmake !ThirdDepOptions! !ExtraOptions! -DUSE_WASM_OPTIONS=!UseWasmOption! -DOSG_ROOT="!OsgRootLocation!" %CurrentDir%
    cmake --build . --target install --config Release
    if not !errorlevel! == 0 (goto exit)
)
if "!BuildMode!"=="4" (
    :: WASM toolchain: WebGL 2
    if not exist %CurrentDir%\build\verse_wasm2\ mkdir %CurrentDir%\build\verse_wasm2
    set OsgRootLocation=%CurrentDir%\build\sdk_wasm2
    cd %CurrentDir%\build\verse_wasm2
    cmake !ThirdDepOptions! !ExtraOptions! -DUSE_WASM_OPTIONS=!UseWasmOption! -DUSE_WASM_OSGEARTH=!WithOsgEarth! -DOSG_ROOT="!OsgRootLocation!" %CurrentDir%
    cmake --build . --target install --config Release
    if not !errorlevel! == 0 (goto exit)
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

set /p AndroidCheckingFlag="Would you like to set a specific SDK version (default: no)? (y/n) > "
if /i "!AndroidCheckingFlag!"=="y" (
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
if !SourceCodePatched!==1 (
    echo *** Automatically unpatching source code...
    %SedEXE% -i.bak "s/TARGET_EXTERNAL_LIBRARIES ${PNG_LIBRARY} ${FREETYPE_LIBRARIES}/TARGET_EXTERNAL_LIBRARIES ${FREETYPE_LIBRARIES}/g" "%OpenSceneGraphRoot%\src\osgPlugins\freetype\CMakeLists.txt"
    %SedEXE% -i.bak "s/ADD_PLUGIN_DIRECTORY(#cfg)/#ADD_PLUGIN_DIRECTORY(cfg)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
    %SedEXE% -i.bak "s/ADD_PLUGIN_DIRECTORY(#obj)/#ADD_PLUGIN_DIRECTORY(obj)/g" "%OpenSceneGraphRoot%\src\osgPlugins\CMakeLists.txt"
    %SedEXE% -i.bak "s/IF(${OSG_WINDOWING_SYSTEM} STREQUAL \"Win32\" AND WIN32 AND NOT ANDROID)/IF(WIN32 AND NOT ANDROID)/g" "%OpenSceneGraphRoot%\src\osgViewer\CMakeLists.txt"
    %SedEXE% -i.bak "s/#ANDROID_3RD_PARTY(#)/ANDROID_3RD_PARTY()/g" "%OpenSceneGraphRoot%\CMakeLists.txt"
    %SedEXE% -i.bak "s#NULL;\/\/dlopen\/\/(#dlopen(#g" "%OpenSceneGraphRoot%\src\osgDB\DynamicLibrary.cpp"
    %SedEXE% -i.bak "s#;\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#g" "%OpenSceneGraphRoot%\src\osg\Texture.cpp"
    %SedEXE% -i.bak "s#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#isTexture2DArraySupported = validContext#g" "%OpenSceneGraphRoot%\src\osg\GLExtensions.cpp"
)
echo Quited.
endlocal
pause
