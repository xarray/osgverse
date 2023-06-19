#!/bin/sh
CurrentDir=$(cd $(dirname $0); pwd)
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
        ;;
    2)  echo "Google Angle."
        ;;
    3)  echo "WASM."
        ;;
    q)  exit 0
        ;;
    *)  echo "OpenGL Compatible Mode."
        BuildResultChecker=build/sdk/bin/osgviewer
        ;;
esac

if [ -f "$CurrentDir/$BuildResultChecker" ]; then
    read -p "Would you like to use current OSG built? (y/n) > " RebuildFlag
    if [ "$RebuildFlag" = 'n' ]; then
        SkipOsgBuild=0
    else
        SkipOsgBuild=1
    fi
fi

# Compile 3rdparty libraries
if [ ! -d "$CurrentDir/build" ]; then
    mkdir $CurrentDir/build
fi

if [ "$SkipOsgBuild" = 1 ]; then

    # Do nothing if skip OSG build
    :

elif [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    :

else

    # Default toolchain
    if [ ! -d "$CurrentDir/build/3rdparty" ]; then
        mkdir $CurrentDir/build/3rdparty
    fi

    cd $CurrentDir/build/3rdparty
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $CurrentDir/helpers/toolchain_builder
    cmake --build .

fi

DepOptions="
    -DFREETYPE_INCLUDE_DIR_freetype2=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_INCLUDE_DIR_ft2build=$CurrentDir/helpers/toolchain_builder/freetype/include
    -DFREETYPE_LIBRARY_RELEASE=$CurrentDir/build/3rdparty/freetype/libfreetype.a
    -DJPEG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/jpeg
    -DJPEG_LIBRARY_RELEASE=$CurrentDir/build/3rdparty/jpeg/libjpeg.a
    -DPNG_PNG_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/png
    -DPNG_LIBRARY_RELEASE=$CurrentDir/build/3rdparty/png/libpng.a
    -DTIFF_INCLUDE_DIR=$CurrentDir/build/3rdparty/tiff;$CurrentDir/helpers/toolchain_builder/tiff
    -DTIFF_LIBRARY_RELEASE=$CurrentDir/build/3rdparty/tiff/libtiff.a
    -DZLIB_INCLUDE_DIR=$CurrentDir/helpers/toolchain_builder/zlib
    -DZLIB_LIBRARY_RELEASE=$CurrentDir/build/3rdparty/zlib/libzlib.a"

# Compile OpenSceneGraph
ExtraOptions="-DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk"
if [ "$SkipOsgBuild" = 1 ]; then

    # Do nothing if skip OSG build
    :

elif [ "$BuildMode" = '1' ]; then

    # OpenGL Core
    if [ ! -d "$CurrentDir/build/osg_core" ]; then
        mkdir $CurrentDir/build/osg_core
    fi

    ExtraOptions="
        -DCMAKE_INCLUDE_PATH=$CurrentDir/helpers/toolchain_builder/opengl
        -DCMAKE_INSTALL_PREFIX=$CurrentDir/build/sdk_core
        -DOPENGL_PROFILE=GL3CORE"
    export OSG_DIR=$CurrentDir/build/sdk_core
    cd $CurrentDir/build/osg_core
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $DepOptions $ExtraOptions $CurrentDir/../OpenSceneGraph
    make install

elif [ "$BuildMode" = '2' ]; then

    # TODO: Google Angle
    :

elif [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    :

else

    # OpenGL default
    if [ ! -d "$CurrentDir/build/osg_def" ]; then
        mkdir $CurrentDir/build/osg_def
    fi

    export OSG_DIR=$CurrentDir/build/sdk
    cd $CurrentDir/build/osg_def
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $ExtraOptions $CurrentDir/../OpenSceneGraph
    make install

fi

# Build osgVerse
if [ "$BuildMode" = '3' ]; then

    # WASM toolchain
    :

else

    # Default toolchain
    if [ ! -d "$CurrentDir/build/verse" ]; then
        mkdir $CurrentDir/build/verse
    fi

    cd $CurrentDir/build/verse
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $ExtraOptions $CurrentDir
    make install

fi
