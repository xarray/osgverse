#include <osg/io_utils>
#include <osg/Version>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

//#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "tiny_gltf_loader.h"

namespace osgVerse
{
    bool LoadBinaryV1(std::vector<char>& data, const std::string& baseDir)
    {
#if false
        tinygltf::Scene scene;
        std::string err;
        tinygltf::TinyGLTFLoader loader;

        bool loaded = loader.LoadBinaryFromMemory(
            &scene, &err, (unsigned char*)data.data(), data.size(), baseDir);
        std::cout << "[LoadBinaryV1] " << loaded << ": " << err << "\n";
        // TODO
        return loaded;
#else
        return false;
#endif
    }
}
