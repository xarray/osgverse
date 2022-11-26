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
| Ubuntu 18.04     | GCC 7.5                  | :heavy_check_mark: |       |
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
| AMD Radeon RX5500         |                | :soon:             |       |
| Intel UHD Graphics        | 4.6 / GLSL 4.6 | :heavy_check_mark: |       |
| MooreThreads MTT S50, S80 |                | :heavy_check_mark: | Has blitting problem in D24S8 mode |
| Zhaoxin C-960 (SIS)       | 3.2 / GLSL 1.5 | :gear:             | Segment fault in osg::Texture at present  |
| VirtualBox SVGA 3D        | 2.1 / GLSL 1.2 | :gear:             | Black screen at present |

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
4. TBD...

#### Assets
1. models: 3D models for test use, mainly in GLTF format.
2. shaders: Shaders for osgVerse rendering pipeline use.
3. skyboxes: Skyboxes for test use, may be one HDR image or 6 cubemaps.
4. misc: Chinese IME CiKu files, font files, ...

#### OSG CMake HowTo
osgVerse depends on OpenSceneGraph (OSG), so developers should first compile OSG with different GL/GLES options. A few common combinations of options to control OSG CMake results are listed as below.
* Static building:
  * DYNAMIC_OPENSCENEGRAPH=OFF
  * DYNAMIC_OPENTHREADS=OFF
* GL3/4 Core profile: You will have to find glcorearb.h from Khronos website. And then put include files to OSG's include folder.
  * OPENGL_PROFILE=GL3Core
  * OPENGL_HEADER1="#include <GL/glcorearb.h>"
* GLES2 (Desktop): You will have to find include-files and libraries from PowerVR / Angel SDK. Only support OSG 3.7.0 or later.
  * OPENGL_PROFILE=GLES2
  * EGL_INCLUDE_DIR="<PowerVR_SDK>/include"
  * EGL_LIBRARY="<PowerVR_SDK>/lib/libEGL.lib"
  * OPENGL_INCLUDE_DIR="<PowerVR_SDK>/include"
  * OPENGL_gl_LIBRARY="<PowerVR_SDK>/lib/libGLESv2.lib"
* GLES3 (Desktop): You will have to find include-files and libraries from PowerVR / Angel SDK. Only support OSG 3.7.0 or later.
  * OPENGL_PROFILE=GLES3
  * EGL_INCLUDE_DIR="<PowerVR_SDK>/include"
  * EGL_LIBRARY="<PowerVR_SDK>/lib/libEGL.lib"
  * OPENGL_INCLUDE_DIR="<PowerVR_SDK>/include"
  * OPENGL_gl_LIBRARY="<PowerVR_SDK>/lib/libGLESv2.lib"
