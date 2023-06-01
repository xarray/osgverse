![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/logo.jpg)
# osgVerse

#### Introduction
osgVerse, a complete 3D engine solution based on OpenSceneGraph.
| Operating System | Compiler                 | Supported          | Notes |
|------------------|--------------------------|--------------------|-------|
| Windows 11       | Visual Studio 2019       | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2022       | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2017       | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2015       | :heavy_check_mark: | Requires VS2015 Update-3 |
| Windows 10       | MSYS2 (GCC 10.2)         | :heavy_check_mark: |       |
| Windows 10 UWP   |                          | :soon:             |       |
| Ubuntu 18.04     | GCC 7.5 (or later)       | :heavy_check_mark: |       |
| Debian 11.7.0    | GCC 10.2 (or later)      | :heavy_check_mark: |       |
| Kylin v10        | GCC 8.3 (or later)       | :heavy_check_mark: |       |
| NeoKylin v7      | GCC 8.5 (built manually) | :heavy_check_mark: | Must disable VERSE_USE_GLIBCXX11_ABI |
| Mac OS X         |                          | :soon:             |       |
| Android          |                          | :soon:             |       |
| IOS              |                          | :soon:             |       |

#### Main Features (ongoing to v1.0)
1. Supports from OSG 3.1.1 to the latest version, and GLSL 1.2 to 4.6, so to work with most hardware in the world.
2. Supports PBR-based rendering and deferred pipeline, with real-time shadowing and deferred lighting.
3. Supports physics simuation based on Bullet3 library, and character animation based on OZZ library.
4. Supports complex model simplication, optimizing and tiling, and saving to OSGB format for better efficiency.
5. Provides a LevelDB nosql plugin, for reading tiles from database rather than local folders.
6. Provides a media streaming plugin, which supports pixel streaming through WebRTC / Websockets.
7. Provides a scripting plugin, which supports scripting using OSGB serialization and changing to Restful-like format.
8. Supports GL3 Core profile, as well as GLES2 / GLES3. Google Angel is also supported for future bridging uses.
9. Supports Emscripten / WASM compilation and works with WebGL / WebGPU based browsers.
10. Provides an initial visual scene editing tool, comparing with the famous Unity Editor.

#### Dependencies
1. Please use CMake 3.0 or higher version. (https://cmake.org/download/)
2. Please use a C++ compiler supporting C++ 14 at least.
3. OpenSceneGraph is always required for building osgVerse. (https://github.com/openscenegraph/OpenSceneGraph) Current project mainly depends on OSG 3.7.0, but can compile on OSG 3.1.1 or later versions.
4. Optional dependencies:
- 4.1 osgEarth 2.10.1 or later, for earth related applications and examples. (https://github.com/gwaldron/osgearth)
- 4.2 Bullet 3.17 or later, for physics support in osgVerseAnimation module and related examples. (https://github.com/bulletphysics/bullet3). Remember to enable USE_MSVC_RUNTIME_LIBRARY_DLL while compiling Bullet.
- 4.3 Entwine 2.0 or later, for EPT point cloud octree constructing. (https://github.com/connormanning/entwine)
- 4.4 Qt 5.5 or later, for Qt related applications and examples. (https://www.qt.io/licensing/open-source-lgpl-obligations)
- 4.5 SDL2 or later, for SDL/GLES related applications and examples. (https://github.com/libsdl-org/SDL)
- 4.6 ZLMediaKit (git version), for media streaming plugin. (https://github.com/ZLMediaKit/ZLMediaKit) Remember to uncheck the ENABLE_MSVC_MT option while compiling. To encode to H264 frame and pull to media server, you may also check ENABLE_X264 and add x264 (http://www.videolan.org/developers/x264.html) to ZLMediaKit.
- 4.7 cesium-native (git version), for 3dtiles reader/writer plugin. (https://github.com/CesiumGS/cesium-native)

#### Supported Hardware
To use osgVerse libraries and applications, OpenGL version must be higher than 2.0. Both core profile and compatible profile will work. Our project uses the GLSL functionality, and supports from GLSL 120 to the latest GLSL version.
Our project is already tested on graphics cards listed as below:
| Grapihcs Card             | OpenGL Version | Supported          | Notes |
|---------------------------|----------------|--------------------|-------|
| NVIDIA RTX 30** Series    | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| NVIDIA 10** Series        | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| NVIDIA 1070 (Nouveau)     | 4.3 / GLSL 4.3 | :zap:              | Display has broken problems with Nouveau driver |
| AMD Radeon RX5500         |                | :soon:             |       |
| Intel UHD Graphics        | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| MooreThreads S80, S2000   | 3.3 / GLSL 3.3 | :heavy_check_mark: | Enable VERSE_USE_MTT_DRIVER before solving driver problems |
| Zhaoxin C-960 (SIS)       | 3.2 / GLSL 1.5 | :zap:              | Segment fault in osg::Texture at present  |
| VirtualBox SVGA 3D        | 2.1 / GLSL 1.2 | :zap:              | osgVerse_Test_Pipeline can work; standard can't |

#### Modules
1. osgVersePipeline: modern rendering pipeline supporting PBR materials, realtime shadows, deferred lighting and effects.
2. osgVerseReaderWriter: full featured reader-writer support for FBX, GLTF and KTX formats, and for more later.
3. osgVerseAnimation: physics and character animation supports.
4. osgVerseModeling: model simplification pipeline, modeling operators, and computational geometry utilities
5. osgVerseUI: IMGUI based quick UI support, HTML based UI solution, and related utilities.
6. osgVerseScript: Scripting support based on OSG serialization functionalities.
7. TBD...

#### Applications
1. osgVerse_SceneEditor: a forward-looking scene editor for osgVerse scene & components.
2. osgVerse_Viewer: a single-camera viewer with modern rendering pipeline support.
3. osgVerse_ViewerComposite: a multi-camera (multi-view) viewer with modern rendering pipeline support.
4. osgVerse_ViewerGLES: an example demostrating how to integrate osgVerse with SDL2, mainly with GLES2/3.
5. osgVerse_EarthViewer: an example demostrating how to integrate osgVerse with osgEarth.
6. osgVerse_QtViewer: an example demostrating how to integrate osgVerse with Qt.
7. TBD...

#### Tests and Examples
1. osgVerse_Test_FastRtt: a quick test for using newly-introduced RTT draw callback.
2. osgVerse_Test_Obb_KDop: a quick test for creating a model's obb/kdop bounding volume.
3. osgVerse_Test_ImGui: a quick test for demostrating the use of IMGUI in osg scene.
4. osgVerse_Test_Mesh_Process: a mesh process and topology builder example.
5. osgVerse_Test_Physics_Basic: a physics world example supporting rigid bodies & kinematics, requiring Bullet3.
6. osgVerse_Test_Shadow: a test for shadow algorithm debugging and optimizing.
7. osgVerse_Test_CubeRtt: a quick test for render-to-cubemap (6 faces) demonstaration.
8. osgVerse_Test_Pipeline: a program for simple cases and compatiblity tests of osgVerse pipeline.
9. osgVerse_Test_Pbr_Prerequisite: a quick utility to pre-compute global PBR textures and save them to IBL osgb files.
10. osgVerse_Test_Paging_Lod: a test for paged LOD file combination and transferring to levelDB.
11. osgVerse_Test_Point_Cloud: a test for point cloud viewing and manipulating.
12. osgVerse_Test_Media_Stream: a test for media streaming / pixel streaming, including server and pusher implementation.
13. osgVerse_Test_Restful_Server: a test for restful HTTP API, which is based on libhv.
14. osgVerse_Test_Indirect_Draw: a demo program to demonstrate how to use indirect drawing of OpenGL 4.x.
15. osgVerse_Test_Tesselation: a demo program to demonstrate how to use tessellation shaders of OpenGL 4.x.
16. osgVerse_Test_Scripting: a test for scripting implementation based on OSGB serialization format.
17. TBD...

#### OSG-style Plugins
1. osgdb_verse_ept: a plugin for massive point cloud paging and rendering based on Entwine.
2. osgdb_verse_fbx: a plugin with full-featured FBX format support.
3. osgdb_verse_gltf: a plugin with full-featured GLTF & GLB format support.
4. osgdb_verse_web: a plugin for HTTP and more web protocols, which may replace the curl plugin.
5. osgdb_verse_leveldb: a plugin for reading/writing from LevelDB database.
6. osgdb_verse_ms: a plugin for reading/writing from media streaming protocols like RTSP/RTMP/WebRTC.
7. osgdb_verse_cesium: a plugin for reading/writing from Cesium 3dtiles. It requires C++ 17. (UNFINISHED)
8. osgdb_verse_osgparticle: a plugin to wrap osgParticle classes for use in scene editor, mainly as an example for custom extensions.
9. osgdb_pbrlayout: a pseudo-plugin to change PBR textures' layout to osgVerse standard. It supports following options:
  - Diffuse (D), Specular (S), Normal (N), Metallic (M), Roughness (R), Occlusion (O), Emissive (E), Ambient (A), Omitted (X)
  - Every source texture is defined by a option character and a channel number (1-4), and separated with a ','.
  - Example input: model.fbx.D4,M1R1X2,N3.pbrlayout (Tex0 = Diffuse x 4, Tex1 = Metallic+Roughness, Tex2 = Normal)
  - All layouts will be converted to osgVerse standard: D4,N3,S4,O1R1M1,A3,E3
10. TBD...

#### Assets
1. models: 3D models for test use, mainly in GLTF format.
2. shaders: Shaders for osgVerse rendering pipeline use.
3. skyboxes: Skyboxes for test use, may be one HDR image or 6 cubemaps.
4. misc: Chinese IME CiKu files, font files, ...

#### CMake options
| Option                      | Type    | Default Value | Notes |
|-----------------------------|---------|---------------|-------|
| OSG_INCLUDE_DIR             | Path    | (Required)    | Set to path of osg/Node |
| OSG_BUILD_INCLUDE_DIR       | Path    | (Required)    | Set to path of osg/Version |
| OSG_LIB_DIR                 | Path    | (Required)    | Set to path of libosg.a or osg.lib |
| OSG_DEBUG_POSTFIX           | String  | d             | Set a postfix for OSG debug built-libraries |
| OSG_GLES_INCLUDE_DIR        | Path    |               | Set to path of GLES2/gl2.h or GLES3/gl3.h, for GLES build only |
| OSG_GLES_LIBRARY            | Path    |               | Set to path of libGLESv2.so or libGLESv2.lib, for GLES build only |
| OSG_EGL_LIBRARY             | Path    |               | Set to path of libEGL.so or libEGL.lib, for GLES build only |
| BULLET_INCLUDE_DIR          | Path    |               | Set to path of btBulletDynamicsCommon.h |
| BULLET_LIB_DIR              | Path    |               | Set to path of libBullet3Dynamics.a or BulletDynamics.lib |
| BULLET_DEBUG_POSTFIX        | String  | _Debug        | Set a postfix for Bullet debug built-libraries |
| OSGEARTH_INCLUDE_DIR        | Path    |               | Set to path of osgEarth/EarthManipulator |
| OSGEARTH_BUILD_INCLUDE_DIR  | Path    |               | Set to path of osgEarth/BuildConfig |
| OSGEARTH_LIB_DIR            | Path    |               | Set to path of libosgEarth.so or osgEarth.lib |
| SDL2_INCLUDE_DIR            | Path    |               | Set to path of SDL.h |
| SDL2_LIB_DIR                | Path    |               | Set to path of libSDL2.so or SDL2.lib |
| Qt5_DIR                     | Path    |               | Set to path of <qt_dist>/lib/cmake/Qt5 |
| VERSE_SUPPORT_CPP17         | Boolean | OFF           | Enable build of libraries using C++ 17 standard |
| VERSE_STATIC_BUILD          | Boolean | OFF           | Enable static build of osgVerse |
| VERSE_USE_OSG_STATIC        | Boolean | OFF           | Use static build of OpenSceneGraph (will force osgVerse to be static) |
| VERSE_WITH_DWARF            | Boolean | OFF           | Enable detailed debug-info and symbols. 'libdw-dev' must be installed |
| VERSE_USE_GLIBCXX11_ABI     | Boolean | ON            | Enable to use libraries built with GCC compiler newer than 4.9 |
| VERSE_USE_MTT_DRIVER        | Boolean | OFF           | Enable to use MooreThreads MTT drivers correctly |

#### Screenshots
![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/sponza.jpg)
