#!/bin/sh
CurrentDir=$(cd $(dirname $0); pwd)
SkipCMakeConfig=0
SkipOsgBuild=0

# Pre-build Checks
if [ ! command -v cmake >/dev/null 2>&1 ]; then
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
2. Desktop / Google Angle
3. WASM / OpenGL ES2
q. Quit
-----------------------------------"
read -p "Enter selection [0-3] > " BuildMode
case "$BuildMode" in
    1)  echo "OpenGL Core Mode."
        BuildResultChecker=build/sdk_core/bin/osgviewer
        CMakeResultChecker=build/osg_core/CMakeCache.txt
        ;;
    2)  echo "Google Angle."
        BuildResultChecker=build/sdk_es/lib/libosgviewer.a
        CMakeResultChecker=build/osg_es/CMakeCache.txt
        ;;
    3)  echo "WebAssembly."
        BuildResultChecker=build/sdk_wasm/lib/libosgviewer.a
        CMakeResultChecker=build/osg_wasm/CMakeCache.txt
        ;;
    q)  exit 0
        ;;
    *)  echo "OpenGL Compatible Mode."
        BuildResultChecker=build/sdk/bin/osgviewer
        CMakeResultChecker=build/osg_def/CMakeCache.txt
        ;;
esac

# Check if CMake is already configured, or OSG is already built
if [ -f "$CurrentDir/$BuildResultChecker" ]; then
    read -p "Would you like to use current OSG built? (y/n) > " RebuildFlag
    if [ "$RebuildFlag" = 'n' ]; then
        SkipOsgBuild=0
    else
        SkipOsgBuild=1
    fi
elif [ -f "$CurrentDir/$CMakeResultChecker" ]; then
    read -p "Would you like to use current CMake cache? (y/n) > " RecmakeFlag
    if [ "$RecmakeFlag" = 'n' ]; then
        SkipCMakeConfig=0
    else
        SkipCMakeConfig=1
    fi
fi

# Check for Emscripten location
EmsdkToolchain="$1/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
if [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    if [ ! -f "$EmsdkToolchain" ]; then
        echo "Emscripten.cmake not found. Please check if Emscripten root folder is provided as an argument."
        exit 1
    fi

fi

# Compile 3rdparty libraries
echo "*** Building 3rdparty libraries..."
if [ ! -d "$CurrentDir/build" ]; then
    mkdir $CurrentDir/build
fi

ThirdPartyBuildDir="$CurrentDir/build/3rdparty"
if [ "$SkipOsgBuild" = 1 ]; then

    # Do nothing if skip OSG build
    :

elif [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    ThirdPartyBuildDir="$CurrentDir/build/3rdparty_wasm"
    if [ ! -d "$ThirdPartyBuildDir" ]; then
        mkdir $ThirdPartyBuildDir
    fi

    cd $ThirdPartyBuildDir
    if [ "$SkipCMakeConfig" = 0 ]; then
        cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$EmsdkToolchain" -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_OPTIONS=1 $CurrentDir/helpers/toolchain_builder
    fi
    cmake --build .

else

    # Default toolchain
    if [ ! -d "$ThirdPartyBuildDir" ]; then
        mkdir $ThirdPartyBuildDir
    fi

    cd $ThirdPartyBuildDir
    if [ "$SkipCMakeConfig" = 0 ]; then
        cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $CurrentDir/helpers/toolchain_builder
    fi
    cmake --build .

fi

# Generate 3rdparty options
DepOptions="
    -DFREETYPE_INCLUDE_DIR_freetype2=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_INCLUDE_DIR_ft2build=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_LIBRARY_RELEASE=$ThirdPartyBuildDir/freetype/libfreetype.a
    -DJPEG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/jpeg
    -DJPEG_LIBRARY_RELEASE=$ThirdPartyBuildDir/jpeg/libjpeg.a
    -DPNG_PNG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/png
    -DPNG_LIBRARY_RELEASE=$ThirdPartyBuildDir/png/libpng.a
    -DTIFF_INCLUDE_DIR=$ThirdPartyBuildDir/tiff;$CurrentDir/helpers/toolchain_builder/tiff
    -DTIFF_LIBRARY_RELEASE=$ThirdPartyBuildDir/tiff/libtiff.a
    -DZLIB_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/zlib
    -DZLIB_LIBRARY_RELEASE=$ThirdPartyBuildDir/zlib/libzlib.a"

# Fix some OpenSceneGraph compile errors
OpenSceneGraphRoot=$CurrentDir/../OpenSceneGraph
sed 's/std::mem_fun_ref/std::mem_fn/g' "$OpenSceneGraphRoot/src/osgUtil/tristripper/include/detail/graph_array.h" > graph_array.h.tmp
mv graph_array.h.tmp "$OpenSceneGraphRoot/src/osgUtil/tristripper/include/detail/graph_array.h"
sed 's/ADD_PLUGIN_DIRECTORY(cfg)/#ADD_PLUGIN_DIRECTORY(#cfg)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"
sed 's/ADD_PLUGIN_DIRECTORY(obj)/#ADD_PLUGIN_DIRECTORY(#obj)/g' "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt" > CMakeLists.txt.tmp
mv CMakeLists.txt.tmp "$OpenSceneGraphRoot/src/osgPlugins/CMakeLists.txt"

# Fix WebGL running errors
if [ "$BuildMode" = '3' ]; then
    sed 's#dlopen(#NULL;\/\/dlopen\/\/(#g' "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
else
    sed 's#NULL;\/\/dlopen\/\/(#dlopen(#g' "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp" > DynamicLibrary.cpp.tmp
fi
mv DynamicLibrary.cpp.tmp "$OpenSceneGraphRoot/src/osgDB/DynamicLibrary.cpp"
sed 's#glTexParameterf(target, GL_TEXTURE_LOD_BIAS, _lodbias)#;\/\/glTexParameterf(target, \/\/GL_TEXTURE_LOD_BIAS, _lodbias)#g' "$OpenSceneGraphRoot/src/osg/Texture.cpp" > Texture.cpp.tmp
mv Texture.cpp.tmp "$OpenSceneGraphRoot/src/osg/Texture.cpp"
sed 's#case(GL_HALF_FLOAT):#case GL_HALF_FLOAT: case 0x8D61:#g' "$OpenSceneGraphRoot/src/osg/Image.cpp" > Image.cpp.tmp
mv Image.cpp.tmp "$OpenSceneGraphRoot/src/osg/Image.cpp"

# Compile OpenSceneGraph
echo "*** Building OpenSceneGraph..."
ExtraOptions="-DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk"
if [ "$BuildMode" = '1' ]; then

    # OpenGL Core
    if [ ! -d "$CurrentDir/build/osg_core" ]; then
        mkdir $CurrentDir/build/osg_core
    fi

    ExtraOptions="
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_core
        -DOPENGL_PROFILE=GL3CORE"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_core
        if [ "$SkipCMakeConfig" = 0 ]; then
            cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $DepOptions $ExtraOptions $OpenSceneGraphRoot
        fi
        make install
    fi

elif [ "$BuildMode" = '2' ]; then

    # TODO: Google Angle
    :

elif [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    if [ ! -d "$CurrentDir/build/osg_wasm" ]; then
        mkdir $CurrentDir/build/osg_wasm
    fi

    ExtraOptions="
        -DCMAKE_TOOLCHAIN_FILE="$EmsdkToolchain"
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_wasm
        -DOSG_SOURCE_DIR=$OpenSceneGraphRoot
        -DOSG_BUILD_DIR=$CurrentDir/build/osg_wasm/osg"
    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_wasm
        if [ "$SkipCMakeConfig" = 0 ]; then
            cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $DepOptions $ExtraOptions $CurrentDir/helpers/osg_builder/wasm
        fi
        make install
    fi

else

    # OpenGL default
    if [ ! -d "$CurrentDir/build/osg_def" ]; then
        mkdir $CurrentDir/build/osg_def
    fi

    if [ "$SkipOsgBuild" = 0 ]; then
        cd $CurrentDir/build/osg_def
        if [ "$SkipCMakeConfig" = 0 ]; then
            cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $ExtraOptions $OpenSceneGraphRoot
        fi
    fi
    make install

fi

# Build osgVerse
echo "*** Building osgVerse..."
if [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    if [ ! -d "$CurrentDir/build/verse_wasm" ]; then
        mkdir $CurrentDir/build/verse_wasm
    fi

    OsgRootLocation="$CurrentDir/build/sdk_wasm"
    cd $CurrentDir/build/verse_wasm
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_OPTIONS=1 -DVERSE_USE_WEBGL2=0 -DOSG_ROOT="$OsgRootLocation" $ExtraOptions $CurrentDir
    make install

else

    # Default toolchain
    if [ ! -d "$CurrentDir/build/verse" ]; then
        mkdir $CurrentDir/build/verse
    fi

    if [ "$BuildMode" = '1' ]; then
        OsgRootLocation="$CurrentDir/build/sdk_core"
    else
        OsgRootLocation="$CurrentDir/build/sdk"
    fi
    cd $CurrentDir/build/verse
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DOSG_ROOT="$OsgRootLocation" $ExtraOptions $CurrentDir
    make install

fi
