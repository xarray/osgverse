#!/bin/sh
CurrentDir=$(cd $(dirname $0); pwd)
CurrentSystem=$(uname)
CurrentKernel=$(uname -r)
CheckCmakeExe=$(command -v cmake)
CMakeExe=$(printf "cmake -DCMAKE_BUILD_TYPE=Release")
UsingWSL=0
UseWasmOption=1
SkipCMakeConfig=0
SkipOsgBuild=0

MingwSystem=$(echo $CurrentSystem | grep "MINGW")
WslKernel=$(echo $CurrentKernel | grep "Microsoft")
if [ "$WslKernel" != "" ]; then
    echo "Using Windows subsystem for Linux..."
    UsingWSL=1
elif [ "$MingwSystem" != "" ]; then
    # Should install MinGW-CMake first: pacman -S mingw-w64-x86_64-cmake
    echo "Using MinGW system..."
    CMakeExe=$(printf 'cmake -DCYGWIN=ON -DCMAKE_BUILD_TYPE=Release')
fi

# Pre-build Checks
if [ "$CheckCmakeExe" = "" ]; then
    echo "CMake not found. Please run 'apt install cmake'."
    exit 1
fi

if [ ! -d "../OpenSceneGraph" ]; then
    echo "OSG source folder not found. Please download and unzip it in ../OpenSceneGraph."
    exit 1
fi

# Select how to compile
echo "
How do you like to compile OSG and osgVerse?
-----------------------------------
Please Select:

0. Desktop / OpenGL Compatible Mode
1. Desktop / OpenGL Core Mode
2. Desktop / OpenGLES 3
3. WASM / WebGL 1.0
4. WASM / WebGL 2.0 (optional with osgEarth)
q. Quit
-----------------------------------"
read -p "Enter selection [0-4] > " BuildMode
case "$BuildMode" in
    1)  echo "OpenGL Core Mode."
        BuildResultChecker=build/sdk_core/bin/osgviewer
        CMakeResultChecker=build/osg_core/CMakeCache.txt
        ;;
    2)  echo "OpenGL ES 3 Mode."
        BuildResultChecker=build/sdk_es3/bin/osgviewer
        CMakeResultChecker=build/osg_es3/CMakeCache.txt
        ;;
    3)  echo "WebAssembly WebGL 1."
        BuildResultChecker=build/sdk_wasm/lib/libosgviewer.a
        CMakeResultChecker=build/osg_wasm/CMakeCache.txt
        ;;
    4)  echo "WebAssembly WebGL 2."
        BuildResultChecker=build/sdk_wasm2/lib/libosgviewer.a
        CMakeResultChecker=build/osg_wasm2/CMakeCache.txt
        ;;
    5)  echo "Android GLES 2."
        BuildResultChecker=build/sdk_android/lib/libosgviewer.a
        CMakeResultChecker=build/osg_android/CMakeCache.txt
        ;;
    q)  exit 0
        ;;
    *)  echo "OpenGL Compatible Mode."
        BuildResultChecker=build/sdk/bin/osgviewer
        CMakeResultChecker=build/osg_def/CMakeCache.txt
        ;;
esac

# Check for Emscripten/NDK location
GLES_LibPath="$1/libGLESv2.so"
EGL_LibPath="$1/libEGL.so"
EmsdkToolchain="$1/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
NdkToolchain="$1/build/cmake/android.toolchain.cmake"
AndroidDepOptions=""
if [ "$BuildMode" = '2' ]; then
    # GLES toolchain
    if [ ! -f "$GLES_LibPath" ] || [ ! -f "$EGL_LibPath" ]; then
        echo "libEGL.so or libGLESv2.so not found. Please run as follows: ./Setup.sh <path_of_ligEGL>"
        exit 1
    fi
elif [ "$BuildMode" = '3' ] || [ "$BuildMode" = '4' ]; then
    # WASM toolchain
    if [ ! -f "$EmsdkToolchain" ]; then
        echo "Emscripten.cmake not found. Please run as follows: ./Setup.sh <your_path>/emsdk-3.?.?/"
        exit 1
    fi
elif [ "$BuildMode" = '5' ]; then
    # Android toolchain
    if [ ! -f "$NdkToolchain" ]; then
        echo "android.toolchain.cmake not found. Please run as follows: ./Setup.sh <your_path>/ndk-r??-linux/"
        exit 1
    fi

    AndroidABI="armeabi-v7a"
    AndroidSdkLevel=21
    echo "
-----------------------------------
Please Select an Android ABI option:

0. armeabi-v7a (AArch32, Armv7)
1. armeabi-v7a with NEON
2. arm64-v8a (AArch64, Armv8.0)
3. x86 (MMX, SSE/2/3)
4. x86_64 (MMX, SSE/2/3, SSE4.1/4.2)
-----------------------------------"
    read -p "Enter selection [0-4] > " OptionAndroidABI
    case "$OptionAndroidABI" in
        1)  AndroidABI="armeabi-v7a with NEON" ;;
        2)  AndroidABI="arm64-v8a" ;;
        3)  AndroidABI="x86" ;;
        4)  AndroidABI="x86_64" ;;
        *)  AndroidABI="armeabi-v7a" ;;
    esac

    AndroidDepOptions="
        -DCMAKE_TOOLCHAIN_FILE=$NdkToolchain
        -DANDROID_ABI=$AndroidABI
        -DANDROID_PLATFORM=$AndroidSdkLevel"
    if [ "$UsingWSL" = 1 ]; then
        # Please download NDK for LINUX at https://github.com/android/ndk/wiki/Unsupported-Downloads
        echo "Please download NDK for Linux r20e at present. The latest r25 version seems having problems
              with WSL, as clang will report 'Exec format error'. Need more tests later..."

        # See https://github.com/android/ndk/issues/1755
        # You should first install patchelf: $ sudo apt-get install patchelf
        #/usr/bin/patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2 "$1/toolchains/llvm/prebuilt/linux-x86_64/bin/clang-14"
    fi
fi

# Check if CMake is already configured, or OSG is already built
if [ -f "$CurrentDir/$BuildResultChecker" ]; then
    read -p "Would you like to use current OSG built? (y/n) > " RebuildFlag
    if [ "$RebuildFlag" = 'n' ]; then
        SkipOsgBuild=0
    else
        SkipOsgBuild=1
    fi
#elif [ -f "$CurrentDir/$CMakeResultChecker" ]; then
#    read -p "Would you like to use current CMake cache? (y/n) > " RecmakeFlag
#    if [ "$RecmakeFlag" = 'n' ]; then
#        SkipCMakeConfig=0
#    else
#        SkipCMakeConfig=1
#    fi
fi

# Compile 3rdparty libraries
echo "*** Building 3rdparty libraries..."
if [ ! -d "$CurrentDir/build" ]; then
    mkdir $CurrentDir/build
fi

ThirdPartyBuildDir="$CurrentDir/build/3rdparty"
if [ "$BuildMode" = '3' ] || [ "$BuildMode" = '4' ]; then
    read -p "Would you like to use WASM 64bit (experimental)? (y/n) > " Wasm64Flag
    if [ "$Wasm64Flag" = 'y' ]; then
        UseWasmOption=2
    fi

    # WASM toolchain
    ThirdPartyBuildDir="$CurrentDir/build/3rdparty_wasm"
    if [ ! -d "$ThirdPartyBuildDir" ]; then
        mkdir $ThirdPartyBuildDir
    fi

    if [ "$SkipOsgBuild" = 0 ]; then
        cd $ThirdPartyBuildDir
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe -DCMAKE_TOOLCHAIN_FILE="$EmsdkToolchain" -DUSE_WASM_OPTIONS=$UseWasmOption $CurrentDir/helpers/toolchain_builder
        fi
        cmake --build . || exit 1
    fi

elif [ "$BuildMode" = '5' ]; then

    # Android toolchain
    ThirdPartyBuildDir="$CurrentDir/build/3rdparty_android"
    if [ ! -d "$ThirdPartyBuildDir" ]; then
        mkdir $ThirdPartyBuildDir
    fi

    if [ "$SkipOsgBuild" = 0 ]; then
        cd $ThirdPartyBuildDir
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $AndroidDepOptions $CurrentDir/helpers/toolchain_builder
        fi
        cmake --build . || exit 1
    fi

else

    # Default toolchain for desktop builds
    if [ ! -d "$ThirdPartyBuildDir" ]; then
        mkdir $ThirdPartyBuildDir
    fi

    if [ "$SkipOsgBuild" = 0 ]; then
        cd $ThirdPartyBuildDir
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $CurrentDir/helpers/toolchain_builder
        fi
        cmake --build . || exit 1
    fi

fi

# Generate 3rdparty options
ThirdDepOptions="
    -DFREETYPE_INCLUDE_DIR_freetype2=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_INCLUDE_DIR_ft2build=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_LIBRARY_RELEASE=$ThirdPartyBuildDir/freetype/libfreetype.a
    -DJPEG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/jpeg
    -DJPEG_LIBRARY_RELEASE=$ThirdPartyBuildDir/jpeg/libjpeg.a
    -DPNG_PNG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/png
    -DPNG_LIBRARY_RELEASE=$ThirdPartyBuildDir/png/libpng.a
    -DZLIB_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/zlib
    -DZLIB_LIBRARY_RELEASE=$ThirdPartyBuildDir/zlib/libzlib.a
    -DVERSE_BUILD_3RDPARTIES=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
if [ "$BuildMode" = '3' ] || [ "$BuildMode" = '4' ]; then
    if [ -e "$CurrentDir/../Dependencies/wasm/lib/libtiff.a" ]; then
        ThirdDepOptions="
            $ThirdDepOptions
            -DTIFF_INCLUDE_DIR=$CurrentDir/../Dependencies/wasm/include
            -DTIFF_LIBRARY_RELEASE=$CurrentDir/../Dependencies/wasm/lib/libtiff.a"
    fi
fi

# Fix some OpenSceneGraph compile errors
OpenSceneGraphRoot=$CurrentDir/../OpenSceneGraph
sed 's/if defined(__ANDROID__)/if defined(__EMSCRIPTEN__) || defined(__ANDROID__)/g' "$OpenSceneGraphRoot/src/osgDB/FileUtils.cpp" > FileUtils.cpp.tmp
mv FileUtils.cpp.tmp "$OpenSceneGraphRoot/src/osgDB/FileUtils.cpp"
sed 's/std::mem_fun_ref/std::mem_fn/g' "$OpenSceneGraphRoot/src/osgUtil/tristripper/include/detail/graph_array.h" > graph_array.h.tmp
mv graph_array.h.tmp "$OpenSceneGraphRoot/src/osgUtil/tristripper/include/detail/graph_array.h"
sed 's/ADD_PLUGIN_DIRECTORY(cfg)/#ADD_PLUGIN_DIRECTORY(#cfg)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/ADD_PLUGIN_DIRECTORY(obj)/#ADD_PLUGIN_DIRECTORY(#obj)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/TIFF_FOUND AND OSG_CPP_EXCEPTIONS_AVAILABLE/TIFF_FOUND/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/ANDROID_3RD_PARTY()/#ANDROID_3RD_PARTY(#)/g' "$OpenSceneGraphRoot/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/CMakeLists.txt"

# Fix OpenSceneGraph build warnings and errors
if [ "$BuildMode" = '3' ] || [ "$BuildMode" = '4' ]; then
    sed 's#dlopen(#NULL;\/\/dlopen\/\/(#g' "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
else
    sed 's#NULL;\/\/dlopen\/\/(#dlopen(#g' "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
fi
mv DynamicLibrary.cpp.tmp "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp"
sed 's/#ifndef GL_EXT_texture_compression_s3tc/#if !defined(GL_EXT_texture_compression_s3tc) || !defined(GL_EXT_texture_compression_s3tc_srgb)/g' "$OpenSceneGraphRoot/include/osg/Texture" > Texture.tmp
mv Texture.tmp "$OpenSceneGraphRoot/include/osg/Texture"
sed 's#glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#;\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#g' "$OpenSceneGraphRoot/src/osg/Texture.cpp" > Texture.cpp.tmp
mv Texture.cpp.tmp "$OpenSceneGraphRoot/src/osg/Texture.cpp"
sed 's#case(GL_HALF_FLOAT):#case GL_HALF_FLOAT: case 0x8D61:#g' "$OpenSceneGraphRoot/src/osg/Image.cpp" > Image.cpp.tmp
mv Image.cpp.tmp "$OpenSceneGraphRoot/src/osg/Image.cpp"
if [ "$BuildMode" = '3' ] || [ "$BuildMode" = '4' ]; then
    sed 's#isTexture2DArraySupported = validContext#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#g' "$OpenSceneGraphRoot/src/osg/GLExtensions.cpp" > GLExtensions.cpp.tmp
    mv GLExtensions.cpp.tmp "$OpenSceneGraphRoot/src/osg/GLExtensions.cpp"
fi

# Compile OpenSceneGraph
echo "*** Building OpenSceneGraph..."
if [ "$BuildMode" = '1' ]; then

    # OpenGL Core
    if [ ! -d "$CurrentDir/build/osg_core" ]; then
        mkdir $CurrentDir/build/osg_core
    fi

    ExtraOptions="
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DOPENGL_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_core
        -DOPENGL_PROFILE=GLCORE"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_core
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $ThirdDepOptions $ExtraOptions $OpenSceneGraphRoot
        fi
        cmake --build . --target install --config Release || exit 1
    fi

elif [ "$BuildMode" = '2' ]; then

    # OpenGL ES 3
    if [ ! -d "$CurrentDir/build/osg_es3" ]; then
        mkdir $CurrentDir/build/osg_es3
    fi

    ExtraOptions="
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DOPENGL_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_es3
        -DEGL_LIBRARY=$EGL_LibPath -DOPENGL_gl_LIBRARY=$GLES_LibPath
        -DOSG_WINDOWING_SYSTEM=None -DOPENGL_PROFILE=GLES3"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_es3
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $ThirdDepOptions $ExtraOptions $OpenSceneGraphRoot
        fi
        cmake --build . --target install --config Release || exit 1
    fi

elif [ "$BuildMode" = '3' ]; then

    # WASM toolchain (WebGL 1)
    if [ ! -d "$CurrentDir/build/osg_wasm" ]; then
        mkdir $CurrentDir/build/osg_wasm
    fi

    ExtraOptions="
        -DCMAKE_TOOLCHAIN_FILE="$EmsdkToolchain"
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_wasm
        -DUSE_WASM_OPTIONS=$UseWasmOption
        -DOSG_SOURCE_DIR=$OpenSceneGraphRoot
        -DOSG_BUILD_DIR=$CurrentDir/build/osg_wasm/osg"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_wasm
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $ThirdDepOptions $ExtraOptions $CurrentDir/helpers/osg_builder/wasm
        fi
        cmake --build . --target install --config Release || exit 1
    fi

elif [ "$BuildMode" = '4' ]; then

    # WASM toolchain (WebGL 2)
    if [ ! -d "$CurrentDir/build/osg_wasm2" ]; then
        mkdir $CurrentDir/build/osg_wasm2
    fi

    ExtraOptions="
        -DCMAKE_TOOLCHAIN_FILE="$EmsdkToolchain"
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_wasm2
        -DUSE_WASM_OPTIONS=$UseWasmOption
        -DOSG_SOURCE_DIR=$OpenSceneGraphRoot
        -DOSG_BUILD_DIR=$CurrentDir/build/osg_wasm2/osg"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_wasm2
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $ThirdDepOptions $ExtraOptions $CurrentDir/helpers/osg_builder/wasm2
        fi
        cmake --build . --target install --config Release || exit 1
    fi

elif [ "$BuildMode" = '5' ]; then

    # Android toolchain
    if [ ! -d "$CurrentDir/build/osg_android" ]; then
        mkdir $CurrentDir/build/osg_android
    fi

    ExtraOptions="
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_android
        -DOSG_SOURCE_DIR=$OpenSceneGraphRoot
        -DOSG_BUILD_DIR=$CurrentDir/build/osg_android/osg"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_android
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $AndroidDepOptions $ThirdDepOptions $ExtraOptions $CurrentDir/helpers/osg_builder/android
        fi
        cmake --build . --target install --config Release || exit 1
    fi

else

    # OpenGL default
    if [ ! -d "$CurrentDir/build/osg_def" ]; then
        mkdir $CurrentDir/build/osg_def
    fi

    ExtraOptions="-DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_def
        if [ "$SkipCMakeConfig" = 0 ]; then
            $CMakeExe $ThirdDepOptions $ExtraOptions $OpenSceneGraphRoot
        fi
        cmake --build . --target install --config Release || exit 1
    fi
    
fi

# Build osgEarth (Optional)
WithOsgEarth=0
if [ "$BuildMode" = '4' ]; then

    if [ -d "$CurrentDir/../osgearth-wasm" ]; then

        # WASM toolchain (WebGL 2)
        if [ ! -d "$CurrentDir/build/osgearth_wasm2" ]; then
            mkdir $CurrentDir/build/osgearth_wasm2
        fi

        echo "*** Building osgEarth 2.10..."
        ExtraOptions2="
            -DOSG_DIR=$CurrentDir/build/sdk_wasm2
            -DTHIRDPARTY_ROOT=$CurrentDir/../Dependencies/wasm
            -DUSE_WASM_OPTIONS=$UseWasmOption
            -DOSGEARTH_SOURCE_DIR=$CurrentDir/../osgearth-wasm
            -DOSGEARTH_BUILD_DIR=$CurrentDir/build/osgearth_wasm2/osgearth"
        cd $CurrentDir/build/osgearth_wasm2
        $CMakeExe $ExtraOptions $ExtraOptions2 $CurrentDir/helpers/osg_builder/wasm2_oe
        make install || exit 1
        WithOsgEarth=1

    else
        echo "osgEarth-WASM not found. Please download it and unzip in ../osgearth-wasm if you wish."
    fi

fi

# Build osgVerse
echo "*** Building osgVerse..."
if [ "$BuildMode" = '3' ]; then

    # WASM toolchain (WebGL 1)
    if [ ! -d "$CurrentDir/build/verse_wasm" ]; then
        mkdir $CurrentDir/build/verse_wasm
    fi

    OsgRootLocation="$CurrentDir/build/sdk_wasm"
    cd $CurrentDir/build/verse_wasm
    $CMakeExe -DUSE_WASM_OPTIONS=$UseWasmOption -DOSG_ROOT="$OsgRootLocation" $ThirdDepOptions $ExtraOptions $CurrentDir
    cmake --build . --target install --config Release || exit 1

elif [ "$BuildMode" = '4' ]; then

    # WASM toolchain (WebGL 2)
    if [ ! -d "$CurrentDir/build/verse_wasm2" ]; then
        mkdir $CurrentDir/build/verse_wasm2
    fi

    OsgRootLocation="$CurrentDir/build/sdk_wasm2"
    cd $CurrentDir/build/verse_wasm2
    $CMakeExe -DUSE_WASM_OPTIONS=$UseWasmOption -DUSE_WASM_OSGEARTH=$WithOsgEarth -DOSG_ROOT="$OsgRootLocation" $ThirdDepOptions $ExtraOptions $CurrentDir
    cmake --build . --target install --config Release || exit 1

elif [ "$BuildMode" = '5' ]; then

    # WASM toolchain
    if [ ! -d "$CurrentDir/build/verse_android" ]; then
        mkdir $CurrentDir/build/verse_android
    fi

    OsgRootLocation="$CurrentDir/build/sdk_android"
    cd $CurrentDir/build/verse_android
    $CMakeExe $AndroidDepOptions -DOSG_ROOT="$OsgRootLocation" $ThirdDepOptions $ExtraOptions $CurrentDir
    cmake --build . --target install --config Release || exit 1

else

    # Default toolchain
    if [ "$BuildMode" = '1' ]; then
        ExtraOptions2="-DOPENGL_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/opengl"
        OsgRootLocation="$CurrentDir/build/sdk_core"
        if [ ! -d "$CurrentDir/build/verse_core" ]; then
            mkdir $CurrentDir/build/verse_core
        fi
        cd $CurrentDir/build/verse_core
    elif [ "$BuildMode" = '2' ]; then
        ExtraOptions2="-DOPENGL_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/opengl
                       -DOSG_EGL_LIBRARY=$EGL_LibPath -DOSG_GLES_LIBRARY=$GLES_LibPath"
        OsgRootLocation="$CurrentDir/build/sdk_es3"
        if [ ! -d "$CurrentDir/build/verse_es3" ]; then
            mkdir $CurrentDir/build/verse_es3
        fi
        cd $CurrentDir/build/verse_es3
    else
        ExtraOptions2=""
        OsgRootLocation="$CurrentDir/build/sdk"
        if [ ! -d "$CurrentDir/build/verse_def" ]; then
            mkdir $CurrentDir/build/verse_def
        fi
        cd $CurrentDir/build/verse_def
    fi
    $CMakeExe -DOSG_ROOT="$OsgRootLocation" $ThirdDepOptions $ExtraOptions $ExtraOptions2 $CurrentDir
    cmake --build . --target install --config Release || exit 1

fi

# Reset some OpenSceneGraph source code
sed 's/ADD_PLUGIN_DIRECTORY(#cfg)/#ADD_PLUGIN_DIRECTORY(cfg)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/ADD_PLUGIN_DIRECTORY(#obj)/#ADD_PLUGIN_DIRECTORY(obj)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/#ANDROID_3RD_PARTY(#)/ANDROID_3RD_PARTY()/g' "$OpenSceneGraphRoot/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/CMakeLists.txt"
sed 's#NULL;\/\/dlopen\/\/(#dlopen(#g' "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
mv DynamicLibrary.cpp.tmp "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp"
sed 's#\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#;glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#g' "$OpenSceneGraphRoot/src/osg/Texture.cpp" > Texture.cpp.tmp
mv Texture.cpp.tmp "$OpenSceneGraphRoot/src/osg/Texture.cpp"
sed 's#isTexture2DArraySupported = isTexture3DSupported;\/\/validContext#isTexture2DArraySupported = validContext#g' "$OpenSceneGraphRoot/src/osg/GLExtensions.cpp" > GLExtensions.cpp.tmp
mv GLExtensions.cpp.tmp "$OpenSceneGraphRoot/src/osg/GLExtensions.cpp"
