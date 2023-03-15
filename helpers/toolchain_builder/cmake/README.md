# Build scripts for platforms
osgVerse depends on OpenSceneGraph (OSG), so developers should first compile OSG with different GL/GLES options. A few common combinations of options to control OSG CMake results are listed as below.

#### OSG for GL3/4 Core profile
* CMake options: (not in cmake-gui)
  * OPENGL_PROFILE: GL3Core
* You will have to find glcorearb.h from Khronos website. And then put include files to OSG's include folder.

#### OSG for GLES2/GLES3 (Desktop)
* CMake options: (not in cmake-gui)
  * OPENGL_PROFILE: GLES2/GLES3
  * EGL_INCLUDE_DIR: <PowerVR_SDK>/include
  * EGL_LIBRARY: <PowerVR_SDK>/lib/libEGL.lib
  * OPENGL_INCLUDE_DIR: <PowerVR_SDK>/include
  * OPENGL_gl_LIBRARY: <PowerVR_SDK>/lib/libGLESv2.lib
* You will have to find include-files and libraries from PowerVR / Angel SDK. Only support OSG 3.7.0 or later.
* Command-line example: (Windows only)
  * cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=<sdk_path> -DOPENGL_PROFILE=GLES2 -DEGL_INCLUDE_DIR=<angle_path>/include -DOPENGL_INCLUDE_DIR=<angle_path>/include -DEGL_LIBRARY=<angle_path>/lib/libEGL.lib -DOPENGL_gl_LIBRARY=<angle_path>/lib/libGLESv2.lib ..
  * cmake --build .

#### OSG for Android (Cross-compiling)
* CMake options: (not in cmake-gui)
  * CMAKE_TOOLCHAIN_FILE: android.toolchain.cmake
  * CMAKE_BUILD_TYPE: Release
  * ANDROID_NDK: <Android NDK path>
  * ANDROID_ABI: armeabi/armeabi-v7a/armeabi-v7a with NEON/arm64-v8a/mips/mips64/x86/x86_64
  * ANDROID_NATIVE_API_LEVEL: 21 (find a version in <Android NDK path>/platforms)
  * CMAKE_MAKE_PROGRAM (Windows only): <Android NDK path>/prebuilt/windows-x86_64/bin/make.exe
  * DYNAMIC_OPENSCENEGRAPH: OFF
  * DYNAMIC_OPENTHREADS: OFF
  * OPENGL_PROFILE: GLES2/GLES3
* Command-line example: (Windows only)
  * cmake -G"MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="android.toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DANDROID_NDK="<ndk_path>" -DANDROID_ABI="armeabi-v7a with NEON" -DCMAKE_MAKE_PROGRAM="<ndk_path>/prebuilt/windows-x86_64/bin/make.exe" -DANDROID_NATIVE_API_LEVEL=21 -DDYNAMIC_OPENSCENEGRAPH=OFF -DDYNAMIC_OPENTHREADS=OFF -DOPENGL_PROFILE=GLES2 "<osg_root_path>"
  * cmake --build .
