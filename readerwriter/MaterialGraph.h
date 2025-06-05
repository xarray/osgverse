#ifndef MANA_READERWRITER_MATERIALGRAPH_HPP
#define MANA_READERWRITER_MATERIALGRAPH_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT MaterialGraph : public osg::Referenced
    {
    public:
        static MaterialGraph* instance();
        bool readFromBlender(const std::string& data, osg::StateSet& ss);

        struct MaterialPin : public osg::Referenced
        {
            std::string name, type;
            std::vector<double> values;
        };

        struct MaterialNode : public osg::Referenced
        {
            std::string name, type, imagePath;
            osg::ref_ptr<osg::Texture> texture;
            std::map<std::string, osg::ref_ptr<MaterialPin>> inputs, outputs;
        };

        struct MaterialLink : public osg::Referenced
        {
            osg::ref_ptr<MaterialNode> nodeFrom, nodeTo;
            osg::observer_ptr<MaterialPin> pinFrom, pinTo;
        };

    protected:
        MaterialGraph() {}
    };
}

#endif
