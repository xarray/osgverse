#include <osg/io_utils>
#include <osg/Version>
#include <osg/ValueObject>
#include <osg/AnimationPath>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ConvertUTF>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "pipeline/Utilities.h"
#include "LoadTextureKTX.h"
#include <picojson.h>

#ifdef VERSE_USE_DRACO
#   define TINYGLTF_ENABLE_DRACO
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "SaveSceneGLTF.h"
#include "Utilities.h"

namespace osgVerse
{
    SaverGLTF::SaverGLTF(osg::Node& node, std::ostream& out, const std::string& d, bool isBinary)
    {

    }

    bool saveGltf(osg::Node& node, const std::string& file, bool isBinary)
    {
        std::string workDir = osgDB::getFilePath(file);
        std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);
        if (!out)
        {
            OSG_WARN << "[SaverGLTF] file " << file << " not writable" << std::endl;
            return false;
        }

        osg::ref_ptr<SaverGLTF> saver = new SaverGLTF(node, out, workDir, isBinary);
        return true;
    }

    bool saveGltf2(osg::Node& node, std::ostream& out, const std::string& dir, bool isBinary)
    {
        osg::ref_ptr<SaverGLTF> saver = new SaverGLTF(node, out, dir, isBinary);
        return true;
    }
}
