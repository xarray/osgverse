#ifndef MANA_READERWRITER_SAVESCENE_GLTF_HPP
#define MANA_READERWRITER_SAVESCENE_GLTF_HPP

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <iterator>
#include <fstream>
#include <iostream>

#define TINYGLTF_USE_RAPIDJSON 1
#include "3rdparty/tiny_gltf.h"
#include "animation/PlayerAnimation.h"
#include "Export.h"

namespace osgVerse
{
    class SaverGLTF : public osg::Referenced
    {
    public:
        SaverGLTF(osg::Node& node, std::ostream& out, const std::string& d, bool isBinary);
        tinygltf::Model& getModelData() { return _modelDef; }

    protected:
        virtual ~SaverGLTF() {}

        tinygltf::Model _modelDef;
    };

    OSGVERSE_RW_EXPORT bool saveGltf(osg::Node& node, const std::string& file, bool isBinary);
    OSGVERSE_RW_EXPORT bool saveGltf2(osg::Node& node, std::ostream& out, const std::string& dir, bool isBinary);
}

#endif
