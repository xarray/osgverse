#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <netcdf.h>

class ReaderWriterNetCDF : public osgDB::ReaderWriter
{
public:
    ReaderWriterNetCDF()
    {
        supportsExtension("verse_netcdf", "osgVerse pseudo-loader");
        supportsExtension("cdf", "CDF data file");
        supportsExtension("hdf5", "HDF5 data file");
    }

    virtual const char* className() const
    {
        return "[osgVerse] netCDF data reader";
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_netcdf");
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

        return NULL;
    }

protected:
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_netcdf, ReaderWriterNetCDF)
