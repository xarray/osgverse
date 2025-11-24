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
        SaverGLTF(const osg::Node& node, std::ostream& out, const std::string& dir,
                  const std::string& imgHint, bool isBinary);
        tinygltf::Model* getModelData() { return _modelDef; }
        bool getResult() const { return _done; }

    protected:
        virtual ~SaverGLTF();

        tinygltf::Model* _modelDef;
        bool _done;
    };

    OSGVERSE_RW_EXPORT bool saveGltf(const osg::Node& node, const std::string& file,
                                     const std::string& imgHint, bool isBinary);
    OSGVERSE_RW_EXPORT bool saveGltf2(const osg::Node& node, std::ostream& out, const std::string& dir,
                                      const std::string& imgHint, bool isBinary);
}

#endif
