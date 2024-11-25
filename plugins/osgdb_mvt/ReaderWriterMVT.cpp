#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <vtzero/vector_tile.hpp>

class ReaderWriterMVT : public osgDB::ReaderWriter
{
public:
    ReaderWriterMVT()
    {
        supportsExtension("verse_mvt", "osgVerse pseudo-loader");
        supportsExtension("mvt", "MVT vector tile file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] MVT vector tile format reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_mvt");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return readImage(in, options);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        // TODO
        osg::ref_ptr<osg::Image> image = new osg::Image;
        return image;
    }

protected:
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_mvt, ReaderWriterMVT)
