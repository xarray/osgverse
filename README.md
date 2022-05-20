# osgVerse

#### Introduction
osgVerse, a complete 3D engine solution based on OpenSceneGraph.

#### Dependencies
1. Please use CMake 3.20 or higher version. (https://cmake.org/download/)
2. Please use a C++ compiler supporting C++ 14 at least. Current project is already tested on Visual Studio 2017.
3. OpenSceneGraph is always required for building osgVerse. (https://github.com/openscenegraph/OpenSceneGraph) Current project mainly depends on OSG 3.7.0, but can compile on OSG 3.1.1 or later versions.
4. Optional dependencies:
    4.1 osgEarth 3.2 or later, for earth related applications and examples. (https://github.com/gwaldron/osgearth)
    4.2 Bullet 3.17 or later, for physics support in osgVerseAnimation module and related examples. (https://github.com/bulletphysics/bullet3)
    4.3 Entwine 2.0 or later, for EPT point cloud octree constructing. (https://github.com/connormanning/entwine)
    4.4 TBD...

#### Modules
1. osgVersePipeline: modern rendering pipeline supporting PBR materials, realtime shadows, deferred lighting and effects.
2. osgVerseReaderWriter: full featured reader-writer support for FBX and GLTF formats, and for more later.
3. osgVerseAnimation: physics and character animation supports.
4. osgVerseModeling: model simplification pipeline, modeling operators, and computational geometry utilities
5. osgVerseUI: IMGUI based quick UI support, HTML based UI solution, and related utilities.
6. TBD...

#### Examples
1. osgVerse_Viewer: a scene viewer with modern rendering pipeline support.
2. osgVerse_EarthViewer: an example demostrate how to integrate osgVerse with osgEarth.
3. osgVerse_Test_FastRtt: a quick test for using newly-introduced RTT draw callback.
4. osgVerse_Test_Obb_KDop: a quick test for creating a model's obb/kdop bounding volume.
5. osgVerse_Test_ImGui: a quick test for demostrating the use of IMGUI in osg scene.
6. osg-style Plugins:
    6.1 osgdb_ept: a plugin for massive point cloud paging and rendering based on Entwine.
    6.2 osgdb_fbx: a plugin with full-featured FBX format support.
    6.3 osgdb_gltf: a plugin with full-featured GLTF & GLB format support.
    6.4 TBD...
