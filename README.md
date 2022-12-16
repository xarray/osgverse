![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/logo.jpg)
# osgVerse

#### Introduction
osgVerse, a complete 3D engine solution based on OpenSceneGraph.
| Operating System | Compiler                 | Supported          | Notes |
|------------------|--------------------------|--------------------|-------|
| Windows 10       | Visual Studio 2022       | :heavy_check_mark: |       |
| Windows 10       | Visual Studio 2017       | :heavy_check_mark: |       |
| Windows 10       | MSYS2 (GCC 10.2)         | :heavy_check_mark: |       |
| Windows 10 UWP   |                          | :soon:             |       |
| Ubuntu 18.04     | GCC 7.5 (or later)       | :heavy_check_mark: |       |
| NeoKylin v7      | GCC 8.5 (built manually) | :heavy_check_mark: | Must disable VERSE_USE_GLIBCXX11_ABI |
| Mac OS X         |                          | :soon:             |       |
| Android          |                          | :soon:             |       |
| IOS              |                          | :soon:             |       |

#### Dependencies
1. Please use CMake 3.0 or higher version. (https://cmake.org/download/)
2. Please use a C++ compiler supporting C++ 14 at least.
3. OpenSceneGraph is always required for building osgVerse. (https://github.com/openscenegraph/OpenSceneGraph) Current project mainly depends on OSG 3.7.0, but can compile on OSG 3.1.1 or later versions.
4. Optional dependencies:
- 4.1 osgEarth 2.10.1 or later, for earth related applications and examples. (https://github.com/gwaldron/osgearth)
- 4.2 Bullet 3.17 or later, for physics support in osgVerseAnimation module and related examples. (https://github.com/bulletphysics/bullet3)
- 4.3 Entwine 2.0 or later, for EPT point cloud octree constructing. (https://github.com/connormanning/entwine)
- 4.4 Qt 5.5 or later, for Qt related applications and examples. (https://www.qt.io/licensing/open-source-lgpl-obligations)

#### Graphics Hardware
To use osgVerse libraries and applications, OpenGL version must be higher than 2.0. Both core profile and compatible profile will work. Our project uses the GLSL functionality, and supports from GLSL 120 to the latest GLSL version.
Our project is already tested on graphics cards listed as below:
| Grapihcs Card             | OpenGL Version | Supported          | Notes |
|---------------------------|----------------|--------------------|-------|
| NVIDIA RTX 3060 Laptop    | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| NVIDIA RTX 1050 Mobile    | 3.2 / GLSL 1.5 | :soon:             |       |
| AMD Radeon RX5500         |                | :soon:             |       |
| Intel UHD Graphics        | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| MooreThreads MTT S50, S80 |                | :heavy_check_mark: | Has blitting problem in D24S8 mode |
| Zhaoxin C-960 (SIS)       | 3.2 / GLSL 1.5 | :zap:              | Segment fault in osg::Texture at present  |
| VirtualBox SVGA 3D        | 2.1 / GLSL 1.2 | :zap:              | Black screen at present |

#### Modules
1. osgVersePipeline: modern rendering pipeline supporting PBR materials, realtime shadows, deferred lighting and effects.
2. osgVerseReaderWriter: full featured reader-writer support for FBX and GLTF formats, and for more later.
3. osgVerseAnimation: physics and character animation supports.
4. osgVerseModeling: model simplification pipeline, modeling operators, and computational geometry utilities
5. osgVerseUI: IMGUI based quick UI support, HTML based UI solution, and related utilities.
6. TBD...

#### Applications
1. osgVerse_SceneEditor: a forward-looking scene editor for osgVerse scene & components.
2. osgVerse_Viewer: a scene viewer with modern rendering pipeline support.
3. osgVerse_EarthViewer: an example demostrating how to integrate osgVerse with osgEarth.
4. osgVerse_QtViewer: an example demostrating how to integrate osgVerse with Qt.
5. TBD...

#### Tests and Examples
1. osgVerse_Test_FastRtt: a quick test for using newly-introduced RTT draw callback.
2. osgVerse_Test_Obb_KDop: a quick test for creating a model's obb/kdop bounding volume.
3. osgVerse_Test_ImGui: a quick test for demostrating the use of IMGUI in osg scene.
4. osgVerse_Test_Mesh_Process: a mesh process and topology builder example.
5. osgVerse_Test_Physics_Basic: a physics world example supporting rigid bodies & kinematics, requiring Bullet3.
6. osgVerse_Test_Shadow: a test for shadow algorithm debugging and optimizing.
7. osgVerse_Test_Pbr_Prerequisite: a quick utility to pre-compute global PBR textures and save them to IBL osgb files.
8. osgVerse_Test_Paging_Lod: a quick utility for paged LOD file combination and testing. (UNUSABLE)
9. TBD...

#### OSG-style Plugins
1. osgdb_verse_ept: a plugin for massive point cloud paging and rendering based on Entwine.
2. osgdb_verse_fbx: a plugin with full-featured FBX format support.
3. osgdb_verse_gltf: a plugin with full-featured GLTF & GLB format support.
4. osgdb_verse_osgparticle: a plugin to wrap osgParticle classes for use in scene editor, mainly as an example for custom extensions.
5. osgdb_pbrlayout: a pseudo-plugin to change PBR textures' layout to osgVerse standard. It supports following options:
  - Diffuse (D), Specular (S), Normal (N), Metallic (M), Roughness (R), Occlusion (O), Emissive (E), Ambient (A), Omitted (X)
  - Every source texture is defined by a option character and a channel number (1-4), and separated with a ','.
  - Example input: model.fbx.D4,M1R1X2,N3.pbrlayout (Tex0 = Diffuse x 4, Tex1 = Metallic+Roughness, Tex2 = Normal)
  - All layouts will be converted to osgVerse standard: D4,N3,S4,O1R1M1,A3,E3
6. TBD...

#### Assets
1. models: 3D models for test use, mainly in GLTF format.
2. shaders: Shaders for osgVerse rendering pipeline use.
3. skyboxes: Skyboxes for test use, may be one HDR image or 6 cubemaps.
4. misc: Chinese IME CiKu files, font files, ...

#### CMake options
| Option                      | Type    | Default Value | Notes |
|-----------------------------|---------|---------------------|-------|
| OSG_INCLUDE_DIR             | Path    | (Required)    | Set to path of osg/Node |
| OSG_BUILD_INCLUDE_DIR       | Path    | (Required)    | Set to path of osg/Version |
| OSG_LIB_DIR                 | Path    | (Required)    | Set to path of libosg.a or osg.lib |
| BULLET_INCLUDE_DIR          | Path    |               | Set to path of btBulletDynamicsCommon.h |
| BULLET_LIB_DIR              | Path    |               | Set to path of libBullet3Dynamics.a or BulletDynamics.lib |
| OSGEARTH_INCLUDE_DIR        | Path    |               | Set to path of osgEarth/EarthManipulator |
| OSGEARTH_BUILD_INCLUDE_DIR  | Path    |               | Set to path of osgEarth/BuildConfig |
| OSGEARTH_LIB_DIR            | Path    |               | Set to path of libosgEarth.so or osgEarth.lib |
| Qt5_DIR                     | Path    |               | Set to path of <qt_dist>/lib/cmake/Qt5 |
| VERSE_STATIC_BUILD          | Boolean | OFF           | Enable static build of osgVerse |
| VERSE_USE_OSG_STATIC        | Boolean | OFF           | Use static build of OpenSceneGraph (will force osgVerse to be static) |
| VERSE_WITH_DWARF            | Boolean | OFF           | Enable detailed debug-info and symbols. 'libdw-dev' must be installed |
| VERSE_USE_GLIBCXX11_ABI     | Boolean | ON            | Enable to use libraries built with GCC compiler newer than 4.9 |

#### Screenshots
![Image](https://gitee.com/xarray/osgverse/raw/master/assets/misc/sponza.jpg)
