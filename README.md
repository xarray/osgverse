![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/logo.jpg)
# osgVerse

#### Introduction
osgVerse, a complete 3D engine solution based on OpenSceneGraph.
| Operating System | Compiler                 | Supported          | Notes |
|------------------|--------------------------|--------------------|-------|
| Windows 11       | Visual Studio 2017-2022  | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2017-2022  | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2015       | :heavy_check_mark: | Requires VS2015 Update-3 |
| Windows 10       | MSYS2 (GCC 10.2)         | :heavy_check_mark: |       |
| Windows 10       | Intel Compiler 2023      | :heavy_check_mark: | cmake -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icx -GNinja .. |
| Windows 10 UWP   |                          | :soon:             |       |
| Ubuntu 18.04     | GCC 7.5 (or later)       | :heavy_check_mark: |       |
| Debian 11.7.0    | GCC 10.2 (or later)      | :heavy_check_mark: |       |
| Kylin v10        | GCC 8.3 (or later)       | :heavy_check_mark: | Set VERSE_FIND_LEGACY_OPENGL to ON |
| NeoKylin v7      | GCC 8.5 (built manually) | :heavy_check_mark: | Must disable VERSE_USE_GLIBCXX11_ABI |
| Mac OS X         |                          | :soon:             |       |
| Android          | WSL + NDK r20e (Clang)   | :heavy_check_mark: |       |
| IOS              |                          | :soon:             |       |
| WebAssmebly      | WSL + Emscripten 3.1.64  | :heavy_check_mark: | Supports GLES2 (WebGL1) at present |

#### Main Features (ongoing to v1.0)
- [x] Supports from OSG 3.1.1 to the latest version, and GLSL 1.2 to 4.6, so to work with most hardware in the world.
- [x] (PARTLY) Supports PBR-based rendering and deferred pipeline, with real-time shadowing and deferred lighting.
- [x] (PARTLY) Supports physics simuation based on Bullet3/PhysX library, and character animation based on OZZ library.
- [ ] Supports complex model simplication, optimizing and tiling, and saving to OSGB format for better efficiency.
- [x] Provides a LevelDB nosql plugin, for reading tiles from database rather than local folders.
- [x] Provides a media streaming plugin, which supports pixel streaming through WebRTC / Websockets.
- [x] Provides a scripting plugin, which supports scripting using OSGB serialization and changing to Restful-like format.
- [x] Supports GL3 Core profile, as well as GLES2 / GLES3. Google Angel is also supported for future bridging uses.
- [x] (PARTLY) Supports major desktop and mobile operating systems, including Windows, Linux, Mac OSX, Android and IOS.
- [ ] Supports major embedded devices with GPU supports, including ARM and RISC-V.
- [x] Supports Emscripten / WASM compilation and works with WebGL 1/2 based browsers.
- [ ] Provides an initial visual scene editing tool, comparing with the famous Unity Editor.

#### Screenshots
* osgVerse_Viewer: PBR and deferred pipeline with Desktop GL
![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/sponza.jpg)
* osgVerse_ViewerWASM: PBR and deferred pipeline with WebGL 1.0, compiled from WASM
![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/sponza_wasm.jpg)
* osgVerse_JsCallerWASM: Scriptable osgb rendering with WebGL 1.0, compiled from WASM
![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/osgb_wasm.jpg)

#### Business partner cases
https://www.yuque.com/wangrui-jhjnz/zx531p/io8e7aq2ufsdhhl2

#### Dependencies
1. Please use CMake 3.0 or higher version. (https://cmake.org/download/)
2. Please use a C++ compiler supporting C++ 14 at least.
3. OpenSceneGraph is always required for building osgVerse. (https://github.com/openscenegraph/OpenSceneGraph) Current project mainly depends on OSG 3.7.0, but can compile on OSG 3.1.1 or later versions.
4. Important dependencies:
  - 4.1 SDL2 (https://github.com/libsdl-org/SDL): for windowing system supports on Android, IOS and WebAssembly builds.
  - 4.2 Google Angle (https://github.com/google/angle): for cross-Graphics API uses and Vulkan integrations.
  - 4.3 Emscripten SDK (https://emscripten.org/docs/getting_started/downloads.html): for WebAssembly builds.
5. Optional dependencies:
  - 5.1 osgEarth 2.10.1 or later, for earth related applications and examples. (https://github.com/gwaldron/osgearth)
  - 5.2 Bullet 3.17 or later, for physics support in osgVerseAnimation module and related examples. (https://github.com/bulletphysics/bullet3). Remember to enable INSTALL_LIBS (for correct installation) and USE_MSVC_RUNTIME_LIBRARY_DLL (for /MD flag) while compiling Bullet.
  - 5.3 Entwine 2.0 or later, for EPT point cloud octree constructing. (https://github.com/connormanning/entwine)
  - 5.4 Qt 5.5 or later, for Qt related applications and examples. (https://www.qt.io/licensing/open-source-lgpl-obligations)
  - 5.5 ZLMediaKit (git version), for media streaming plugin. (https://github.com/ZLMediaKit/ZLMediaKit) Remember to uncheck the ENABLE_MSVC_MT option while compiling. To encode to H264 frame and pull to media server, you may also check ENABLE_X264 and add x264 (http://www.videolan.org/developers/x264.html) to ZLMediaKit.
  - 5.6 OpenVDB 10.0 or later, for VDB point cloud and 3D image reader/writer plugin. (https://github.com/AcademySoftwareFoundation/openvdb)
  - 5.7 libDraco 1.5 or later, for Draco mesh compression support in osgVerseReaderWriter library. (https://github.com/google/draco)

#### Supported Hardware
To use osgVerse libraries and applications, OpenGL version must be higher than 2.0. Both core profile and compatible profile will work. Our project uses the GLSL functionality, and supports from GLSL 120 to the latest GLSL version.
Our project is already tested on graphics cards listed as below:
| Graphics Card             | OpenGL Version | Supported          | Notes |
|---------------------------|----------------|--------------------|-------|
| NVIDIA RTX 30** Series    | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| NVIDIA 10** Series        | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| NVIDIA 1070 (Nouveau)     | 4.3 / GLSL 4.3 | :zap:              | Display has broken problems with Nouveau driver |
| NVIDIA GT720              | 4.6 / GLSL 4.6 | :heavy_check_mark: | Current frame rate < 12fps |
| NVIDIA Quadro K2200       | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| AMD Radeon RX5500         |                | :soon:             |       |
| AMD Radeon (TM) Graphics  | 4.6 / GLSL 4.6 | :heavy_check_mark: | Current frame rate < 15fps |
| Intel UHD Graphics        | 4.6 / GLSL 4.6 | :heavy_check_mark: | Current frame rate ~= 30fps |
| MooreThreads S80, S2000   | 3.3 / GLSL 3.3 | :heavy_check_mark: | Enable VERSE_USE_MTT_DRIVER before solving driver problems |
| JingJia Micro JM7201      |                | :soon:             |       |
| Zhaoxin C-960 (SIS)       | 3.2 / GLSL 1.5 | :zap:              | Segment fault in osg::Texture at present  |
| VirtualBox SVGA 3D        | 2.1 / GLSL 1.2 | :zap:              | osgVerse_Test_Pipeline can work; standard can't |

#### Modules
1. osgVersePipeline: modern rendering pipeline supporting PBR materials, realtime shadows, deferred lighting and effects.
2. osgVerseReaderWriter: full featured reader-writer support for FBX, GLTF and KTX formats, and for more later.
3. osgVerseAnimation: physics and character animation supports.
4. osgVerseModeling: model simplification pipeline, modeling operators, and computational geometry utilities
5. osgVerseUI: IMGUI based quick UI support, HTML based UI solution, and related utilities.
6. osgVerseScript: Scripting support based on OSG serialization functionalities.
7. osgVerseAI: Artificial-Intelligence related classes, including navigation-mesh and so on.
8. TBD...

#### Applications
1. osgVerse_SceneEditor: a forward-looking scene editor for osgVerse scene & components.
2. osgVerse_Viewer: a single-camera viewer with modern rendering pipeline support.
3. osgVerse_ViewerComposite: a multi-camera (multi-view) viewer with modern rendering pipeline support.
4. osgVerse_ViewerGLES: an example demostrating how to integrate osgVerse with SDL2, mainly with GLES2/3.
5. osgVerse_EarthViewer: an example demostrating how to integrate osgVerse with osgEarth.
6. osgVerse_QtViewer: an example demostrating how to integrate osgVerse with Qt.
7. osgVerse_ViewerWASM (wasm/pbr_demo): an example of WASM, with modern rendering pipeline support.
8. osgVerse_JsCallerWASM (wasm/script_demo): an example of WASM, demostrating how to use OSGB scripts with JS.
9. TBD...

#### Tests and Examples
1. osgVerse_Test_ImGui: a quick test for demostrating the use of IMGUI in osg scene.
2. osgVerse_Test_Mesh_Process: a mesh process and topology builder example.
3. osgVerse_Test_Physics_Basic: a physics world example supporting rigid bodies & kinematics, requiring Bullet3.
4. osgVerse_Test_Shadow: a test for shadow algorithm debugging and optimizing.
5. osgVerse_Test_Pipeline: a program for simple cases and compatiblity tests of osgVerse pipeline.
6. osgVerse_Test_Pbr_Prerequisite: a quick utility to pre-compute global PBR textures and save them to IBL osgb files.
7. osgVerse_Test_Paging_Lod: a test for paged LOD file combination, optimization and transferring to levelDB.
8. osgVerse_Test_Point_Cloud: a test for point cloud viewing and manipulating.
9. osgVerse_Test_Media_Stream: a test for media streaming / pixel streaming, including server and pusher implementation.
10. osgVerse_Test_Restful_Server: a test for restful HTTP API, which is based on libhv.
11. osgVerse_Test_Scripting: a test for scripting implementation based on OSGB serialization format.
12. osgVerse_Test_Compressing: a test for KTX texture compression (DXT / ETC) and Draco geometry compressing.
13. osgVerse_Test_Thread: a test for using the Marl thread task scheduler along with osgViewer.
14. osgVerse_Test_Volume_Rendering: a test for different methods to implement volume rendering.
15. osgVerse_Test_Symbols: a test for displaying massive symbols with icons and texts.
16. osgVerse_Test_Tween_Animation: a test for tween animations, like path and data-driven animations.
17. Deprecated tests:
  - osgVerse_Test_FastRtt: a quick test for using newly-introduced RTT draw callback.
  - osgVerse_Test_Obb_KDop: a quick test for creating a model's obb/kdop bounding volume.
  - osgVerse_Test_CubeRtt: a quick test for render-to-cubemap (6 faces) demonstaration.
  - osgVerse_Test_Indirect_Draw: a demo program to demonstrate how to use indirect drawing of OpenGL 4.x.
  - osgVerse_Test_Tesselation: a demo program to demonstrate how to use tessellation shaders of OpenGL 4.x.
  - osgVerse_Test_MultiView_Shader: a test for using geometry shader to implement multi-view rendering.

#### OSG-style Plugins
1. osgdb_verse_ept: a plugin for massive point cloud paging and rendering based on Entwine.
2. osgdb_verse_fbx: a plugin with full-featured FBX format support.
3. osgdb_verse_gltf: a plugin with full-featured GLTF & GLB format support.
4. osgdb_verse_web: a plugin for HTTP and more web protocols, which may replace the curl plugin.
5. osgdb_verse_image: a plugin for reading common image formats like JPEG and PNG. It mainly works for WASM case.
6. osgdb_verse_webp: a plugin for reading WEBP formats. It mainly works for 3dtiles scene.
7. osgdb_verse_leveldb: a plugin for reading/writing from LevelDB database.
8. osgdb_verse_ms: a plugin for reading/writing from media streaming protocols like RTSP/RTMP/WebRTC.
9. osgdb_vese_tiles: a plugin for reading Cesium 3dtiles (.json) and Osgb files (metadata.xml, or just the root folder).
10. osgdb_pbrlayout: a pseudo-plugin to change PBR textures' layout to osgVerse standard. It supports following options:
  - Diffuse (D), Specular (S), Normal (N), Metallic (M), Roughness (R), Occlusion (O), Emissive (E), Ambient (A), Omitted (X)
  - Every source texture is defined by a option character and a channel number (1-4), and separated with a ','.
  - Example input: model.fbx.D4,M1R1X2,N3.pbrlayout (Tex0 = Diffuse x 4, Tex1 = Metallic+Roughness, Tex2 = Normal)
  - All layouts will be converted to osgVerse standard: D4,N3,S4,O1R1M1,A3,E3
11. TBD...

#### Assets
1. models: 3D models for test use, mainly in GLTF format.
2. shaders: Shaders for osgVerse rendering pipeline use.
3. skyboxes: Skyboxes for test use, may be one HDR image or 6 cubemaps.
4. misc: Chinese IME CiKu files, font files, screenshots...
5. tests: Some test data

#### Prepare and build third-party libraries
1. By default, osgVerse will find third-party libraries as follows:
  - OpenGL: from the system. Note that some Linux distributions (e.g., Qilin) should enable VERSE_FIND_LEGACY_OPENGL first.
  - OpenSceneGraph: from environment variable $OSG_ROOT.
  - SDL, Draco, Bullet, etc.: from CMake variable ${VERSE_3RDPARTY_PATH}, which is <osgverse_folder>/../Dependencies by default.
    - Actually path to find includes and libraries will be automatically set to '${VERSE_3RDPARTY_PATH}/<platform>'.
    - For x86/x64 build: <platform> is 'x86/x64'.
    - For Android build: <platform> is 'android'.
    - For MacOSX/IOS build: <platform> is 'apple/ios'.
    - For WebAssembly (WASM) build: <platform> is 'wasm'.
    - For Windows UWP build: <platform> is 'uwp'.
2. Build Draco:
  - For WebAssembly (WASM):
    - export EMSCRIPTEN=<emsdk_folder>/upstream/emscripten
    - cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake -DDRACO_WASM=ON
            -DDRACO_JS_GLUE=OFF -DCMAKE_INSTALL_PREFIX=<your_path>/Dependencies/wasm <draco_folder>
  - See https://github.com/google/draco/blob/master/BUILDING.md for other platforms.
3. Build ZLMediaKit: TBD...
4. TBD...

#### Build from Source
0. Assume that osgVerse source code is already at <osgverse_folder>.
1. Desktop Windows / Linux
  - Make sure you have a compiler environment (e.g., Visual Studio).
  - Download and install CMake tool.
  - Download OSG prebuilt libraries or build them from source, extracting to <osg_sdk_folder>.
  - Declare an environment variable OSG_ROOT, to indicate OSG root directory:
    - (Windows) $ set OSG_ROOT=<osg_sdk_folder>
    - (Linux)   $ export OSG_ROOT=<osg_sdk_folder>
  - Run commands below in terminal:
    - $ cd <osgverse_folder>
    - $ mkdir build
    - $ cd build
    - $ cmake ..
    - $ cmake --build .
  - You may also choose to use cmake-gui and set OSG related options in GUI mode.
  - For UWP build:
    - First download Windows Store SDK or latest Windows 11 SDK (with VS2022).
    - Download Angle for UWP: https://www.nuget.org/packages/ANGLE.WindowsStore
    - Rename the .nuget file to .zip and extract it. Find libraries and include files there.
    - Build OSG for GLES2/GLES3 (Desktop / GoogleAngle). See herlps/osg_builder/README.md for details.
    - Run commands below in terminal:
      - $ cmake -G "Visual Studio 17 2022" -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION="10.0" <osgverse_folder>
2. Desktop Mac OSX
  - TBD...
3. Android
  - TBD...
4. IOS
  - TBD...
5. WebAssembly
  - Download emsdk from https://emscripten.org/docs/getting_started/downloads.html, extracting to <emsdk_folder>.
  - Update and activate emsdk as required at the page above.
  - Build dependencies and save to <osgverse_folder>/../Dependencies/wasm
    - Build Draco (https://github.com/google/draco/blob/master/BUILDING.md)
      - $ export EMSCRIPTEN=<emsdk_folder>/upstream/emscripten
      - $ cmake ../ -DCMAKE_TOOLCHAIN_FILE=<emsdk_folder>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DDRACO_WASM=ON
                    -DDRACO_JS_GLUE=OFF -DCMAKE_INSTALL_PREFIX=<osgverse_folder>/../Dependencies/wasm
      - $ make install
    - Build GDAL, GEOS, etc. (https://github.com/bugra9/gdal3.js)
      - Modify GDAL_EMCC_FLAGS.mk to add '-pthread' to flags
      - $ sudo apt install automake sqlite3
      - $ curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/master/install.sh | bash
      - $ nvm install node
      - $ npm install -g pnpm@8.0
      - $ source <emsdk_folder>/emsdk_env.sh
      - $ pnpm install
      - $ pnpm run build
      - Copy bin/include/lib from gdal3.js\build\native\usr to <osgverse_folder>/../Dependencies/wasm
  - Download OSG source code and extract it to <osgverse_folder>/../OpenSceneGraph
  - Start a UNIX ternimal (under Windows, please install WSL v1 and start it).
  - Run commands below in terminal:
    - $ cd <osgverse_folder>
    - $ ./Setup.sh <emsdk_folder>
  - Select "3. WASM / OpenGL ES2" and starting building WASM at <osgverse_folder>/build/verse_wasm.
  - Start an HTTPS server at <osgverse_folder>/build/verse_wasm/bin. See <osgverse_folder>/wasm/run_webserver.py as an example.
  - Copy <osgverse_folder>/assets to the same root folder of the server, and enjoy our WebGL examples.

#### CMake options
| Option                      | Type    | Default Value | Notes |
|-----------------------------|---------|---------------|-------|
| OSG_INCLUDE_DIR             | Path    |               | (Required) Set to path of osg/Node |
| OSG_BUILD_INCLUDE_DIR       | Path    |               | (Required) Set to path of osg/Version |
| OSG_LIB_DIR                 | Path    |               | (Required) Set to path of libosg.a or osg.lib |
| OSG_DEBUG_POSTFIX           | String  | d             | Set a postfix for OSG debug built-libraries |
| OSG_GLES_INCLUDE_DIR        | Path    |               | Set to path of GLES2/gl2.h or GLES3/gl3.h, for GLES build only |
| OSG_GLES_LIBRARY            | Path    |               | Set to path of libGLESv2.so or libGLESv2.lib, for GLES build only |
| OSG_EGL_LIBRARY             | Path    |               | Set to path of libEGL.so or libEGL.lib, for GLES build only |
| BULLET_INCLUDE_DIR          | Path    |               | Set to path of btBulletDynamicsCommon.h |
| BULLET_LIB_DIR              | Path    |               | Set to path of libBulletDynamics.a or BulletDynamics.lib |
| BULLET_DEBUG_POSTFIX        | String  | _Debug        | Set a postfix for Bullet debug built-libraries |
| ZLMEDIAKIT_INCLUDE_DIR      | Path    |               | Set to path of mk_common.h |
| ZLMEDIAKIT_LIB_DIR          | Path    |               | Set to path of libmk_api.so or mk_api.lib |
| OPENVDB_INCLUDE_DIR         | Path    |               | Set to path of openvdb/openvdb.h |
| OPENVDB_BOOST_INCLUDE_DIR   | Path    |               | Set to path of boost/type.hpp |
| OPENVDB_TBB_INCLUDE_DIR     | Path    |               | Set to path of tbb/blocked_range.h |
| OPENVDB_LIB_DIR             | Path    |               | Set to path of libopenvdb.so or openvdb.lib |
| OPENVDB_TBB_LIB_DIR         | Path    |               | Set to path of libtbb.so or tbb.lib |
| OSGEARTH_INCLUDE_DIR        | Path    |               | Set to path of osgEarth/EarthManipulator |
| OSGEARTH_BUILD_INCLUDE_DIR  | Path    |               | Set to path of osgEarth/BuildConfig |
| OSGEARTH_LIB_DIR            | Path    |               | Set to path of libosgEarth.so or osgEarth.lib |
| SDL2_INCLUDE_DIR            | Path    |               | Set to path of SDL.h |
| SDL2_LIB_DIR                | Path    |               | Set to path of libSDL2.so or SDL2.lib |
| Qt5_DIR                     | Path    |               | Set to path of <qt_dist>/lib/cmake/Qt5 |
| Qt6_DIR                     | Path    |               | Set to path of <qt_dist>/lib/cmake/Qt6 |
| VERSE_3RDPARTY_PATH         | Path    |               | Set to path of third-party libraries |
| VERSE_INSTALL_PDB_FILES     | Boolean | ON            | Enable to install PDB files along with executables and dynamic libraries |
| VERSE_BUILD_GPL             | Boolean | OFF           | Enable build of GPL dependencies, which will makes osgVerse a GPL library |
| VERSE_BUILD_3RDPARTIES      | Boolean | ON            | Enable build of common libraries like FreeType, Jpeg and PNG |
| VERSE_BUILD_WITH_QT         | Boolean | OFF           | Enable build of Qt based applications and tests |
| VERSE_BUILD_DEPRECATED_TESTS| Boolean | OFF           | Enable build of deprecated tests |
| VERSE_NO_SIMD_FEATURES      | Boolean | OFF           | Enable to ignore all SIMD features (when struggling with compile errors) |
| VERSE_SUPPORT_CPP17         | Boolean | OFF           | Enable build of libraries using C++ 17 standard |
| VERSE_STATIC_BUILD          | Boolean | OFF           | Enable static build of osgVerse |
| VERSE_USE_OSG_STATIC        | Boolean | OFF           | Use static build of OpenSceneGraph (will force osgVerse to be static) |
| VERSE_USE_DWARF             | Boolean | OFF           | Enable detailed debug-info and symbols. 'libdw-dev' must be installed |
| VERSE_USE_GLIBCXX11_ABI     | Boolean | ON            | Enable to use libraries built with GCC compiler newer than 4.9 |
| VERSE_USE_MTT_DRIVER        | Boolean | OFF           | Enable to use MooreThreads MTT drivers correctly |
| VERSE_USE_FORCED_MULTIPLE   | Boolean | OFF           | Enable to solve LNK2005 problem when compiling OSG 3.2 and lower under MSVC |
| VERSE_WASM_USE_PTHREAD      | Boolean | ON            | Enable Pthread for WASM, which requires COOP / COEP on server-side, for WASM build only |
| VERSE_WASM_OPTIMIZE_SIZE    | Boolean | ON            | Enable -O3 for WASM, which is slow but with small generated size |
| VERSE_FIND_LEGACY_OPENGL    | Boolean | OFF           | Enable to use legacy mode to search OpenGL libraries, for some Linux systems like Kylin |
