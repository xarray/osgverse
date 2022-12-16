#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <readerwriter/LoadSceneGLTF.h>

class TexLayoutVisitor : public osg::NodeVisitor
{
public:
    TexLayoutVisitor(const osgDB::StringList& params) { parse(params); }
    virtual void apply(osg::Drawable& node) {}  // do nothing
    virtual void apply(osg::Geometry& geometry) {}  // do nothing
    
    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) applyStateSet(*node.getStateSet());
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Drawable* d = node.getDrawable(i);
            if (d->getStateSet()) applyStateSet(*(d->getStateSet()));
        }
        if (node.getStateSet()) applyStateSet(*node.getStateSet());
        traverse(node);
    }

    void applyStateSet(osg::StateSet& ss)
    {
        // TODO
    }

protected:
    enum PbrType
    {
        DiffuseType = 'D', SpecularType = 'S', NormalType = 'N',
        MetallicType = 'M', RoughnessType = 'R', OcclusionType = 'O',
        EmissiveType = 'E', AmbientType = 'A', OmittedType = 'X'
    };

    void parse(const osgDB::StringList& params)
    {
        // TODO
    }
};

class ReaderWriterPBRLayout : public osgDB::ReaderWriter
{
public:
    ReaderWriterPBRLayout()
    {
        supportsExtension("pbrlayout", "PBR texture layout pseudo-loader");
    }

    virtual const char* className() const
    {
        return "[osgVerse] PBR texture layout pseudo-loader";
    }

    virtual ReadResult readNode(const std::string& path, const osgDB::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        std::string tmpName = osgDB::getNameLessExtension(path);
        std::size_t index = tmpName.find_last_of('.');
        if (index == std::string::npos) return osgDB::readRefNodeFile(tmpName, options);

        std::string fileName = tmpName.substr(0, index);
        std::string params = (index < tmpName.size() - 1) ? tmpName.substr(index + 1) : "";
        osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(fileName, options);
        if (!node) return ReadResult::FILE_NOT_FOUND;

        osgDB::StringList texParams; osgDB::split(params, texParams, '|');
        TexLayoutVisitor tlv(texParams); node->accept(tlv);
        return node;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(pbrlayout, ReaderWriterPBRLayout)
