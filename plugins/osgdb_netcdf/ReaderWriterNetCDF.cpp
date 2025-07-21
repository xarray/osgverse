#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/ImageSequence>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include "animation/ParticleEngine.h"
#include <limits.h>
#include <netcdf.h>
#include <netcdf_mem.h>

typedef bool (*CheckDataFunc)(const std::string&, GLenum, const std::vector<std::string>&, const std::vector<size_t>);
typedef bool (*SetDataFunc)(const std::string&, GLenum, const std::vector<size_t>, unsigned char*);

template<typename T1, typename T2>
osg::Image* normalizeAndCopyImage(osg::Image* img, T1 refV0, T1 refV1, GLenum pixelFormat, GLenum internalFormat)
{
    T1* ptr = (T1*)img->data(); T1 minV = refV0, maxV = refV1;
    for (int y = 0; y < img->t(); ++y)
        for (int x = 0; x < img->s(); ++x)
        {
            T1 v = *(ptr + y * img->s() + x);
            if (v < minV) minV = v; if (v > maxV) maxV = v;
        }

    osg::Image* img2 = new osg::Image; img2->setName(img->getName());
    img2->allocateImage(img->s(), img->t(), 1, pixelFormat, GL_UNSIGNED_BYTE);
    img2->setInternalTextureFormat(internalFormat);

    T2* ptr2 = (T2*)img2->data();
    for (int y = 0; y < img2->t(); ++y)
        for (int x = 0; x < img2->s(); ++x)
        {
            T1 v = (*(ptr + y * img->s() + x) - minV) * 255 / (maxV - minV);
            *(ptr2 + y * img2->s() + x) = T2(v);
        }
    return img2;
}

class ReaderWriterNetCDF : public osgDB::ReaderWriter
{
public:
    ReaderWriterNetCDF()
    {
        supportsExtension("verse_netcdf", "osgVerse pseudo-loader");
        supportsExtension("cdl", "CDF text data file");
        supportsExtension("nc", "CDF classic data file");
        supportsExtension("nc4", "CDF4 data file");
        supportsExtension("hdf5", "HDF5 data file");
        supportsExtension("h5", "HDF5 data file");

        supportsOption("DimResolutionY", "Save image Y data index: default=0");
        supportsOption("DimResolutionX", "Save image X data index: default=1");
        supportsOption("DimComponents", "Save image component data index: default=2");
        supportsOption("Normalized", "Save image data and normalize every value to 0-255: default=false");
    }

    virtual const char* className() const
    { return "[osgVerse] netCDF data reader"; }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string fileName, ext; fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND; return readObject(in, options);
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string fileName, ext; fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND; return readImage(in, options);
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;
        return loadFromStream(buffer, options, 0);
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::string buffer((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;
        return loadFromStream(buffer, options, 1);
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return fileName;

        bool usePseudo = (ext == "verse_netcdf");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }
        return fileName;
    }

    ReadResult loadFromStream(const std::string& buffer, const Options* options, int type) const
    {
        int idX = 1, idY = 0, idComp = 2; bool normalized = false;
        if (options)
        {
            std::string strX = options->getPluginStringData("DimResolutionX"); if (!strX.empty()) idX = atoi(strX.c_str());
            std::string strY = options->getPluginStringData("DimResolutionY"); if (!strY.empty()) idY = atoi(strY.c_str());
            std::string strC = options->getPluginStringData("DimComponents"); if (!strC.empty()) idComp = atoi(strC.c_str());

            std::string scm = options->getPluginStringData("Normalized");
            std::transform(scm.begin(), scm.end(), scm.begin(), tolower);
            normalized = (scm == "true" || atoi(scm.c_str()) > 0);
        }

        int status = 0, ncid = 0, ndims = 0, nvars = 0, ngatts = 0, unlimdimid = 0;
        status = nc_open_mem("in-memory-hdf5", NC_NETCDF4 | NC_INMEMORY, buffer.size(), (void*)buffer.data(), &ncid);
        if (status != 0)
        {
            OSG_WARN << "[ReaderWriterNetCDF] Failed to open: " << nc_strerror(status)
                     << std::endl; return ReadResult::FILE_NOT_FOUND;
        }

        status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
        if (status != 0)
        {
            OSG_WARN << "[ReaderWriterNetCDF] Failed to parse: " << nc_strerror(status)
                     << std::endl; return ReadResult::ERROR_IN_READING_FILE;
        }

        std::vector<osg::ref_ptr<osgVerse::ParticleCloud>> clouds;
        std::vector<osg::ref_ptr<osg::Image>> images;
        for (int i = 0; i < nvars; i++)
        {
            char varname[NC_MAX_NAME + 1]; nc_type vartype = 0; size_t totalDimSize = 0;
            int varndims = 0, varnatts = 0, vardimids[NC_MAX_VAR_DIMS];
            status = nc_inq_var(ncid, i, varname, &vartype, &varndims, vardimids, &varnatts);
            if (status != 0) continue;

            osg::ref_ptr<osg::Image> image = new osg::Image; image->setName(varname);
            std::vector<std::string> dimNames(varndims); std::vector<size_t> dimLength(varndims);
            OSG_NOTICE << "Var " << i << ": Name: " << varname << ", Type = " << vartype << std::endl;

            for (int j = 0; j < varndims; j++)
            {
                char dimName[NC_MAX_NAME + 1]; nc_inq_dim(ncid, vardimids[j], dimName, &dimLength[j]);
                OSG_NOTICE << "  Dim " << j << ": Name: " << dimName << ", Length = " << dimLength[j]
                           << std::endl; dimNames[j] = dimName; totalDimSize += dimLength[j];
            }

            for (int j = 0; j < varnatts; j++)
            {
                char attName[NC_MAX_NAME + 1]; nc_inq_attname(ncid, i, j, attName);
                nc_type attType = 0; size_t attLength = 0; nc_inq_att(ncid, i, attName, &attType, &attLength);
                OSG_NOTICE << "  Attr " << j << ": Name: " << attName;
                if (attType == 2)
                {
                    char* attVarName = (char*)malloc(attLength + 1);
                    nc_get_att_text(ncid, i, attName, attVarName); attVarName[attLength] = '\0';
                    OSG_NOTICE << ", Description = " << attVarName << std::endl; free(attVarName);
                }
                else
                    OSG_NOTICE << ", Type = " << attType << ", Length = " << attLength << std::endl;
            }

            GLenum dataType = GL_UNSIGNED_BYTE; size_t typeSize = 0;
            switch (vartype)
            {
            case NC_CHAR: case NC_BYTE: case NC_UBYTE:
                dataType = GL_UNSIGNED_BYTE; typeSize = sizeof(char); break;
            case NC_SHORT: case NC_USHORT:
                dataType = GL_UNSIGNED_SHORT; typeSize = sizeof(short); break;
            case NC_INT: case NC_UINT:
                dataType = GL_UNSIGNED_INT; typeSize = sizeof(int); break;
            case NC_FLOAT:
                dataType = GL_FLOAT; typeSize = sizeof(float); break;
            case NC_DOUBLE:
                dataType = GL_DOUBLE; typeSize = sizeof(double); break;
            default: dataType = GL_NONE; break;
            }

            if (type == 0)  // particle cloud
            {
                if (options)
                {
                    CheckDataFunc checkFunc = (CheckDataFunc)options->getPluginData("CheckDataFunction");
                    SetDataFunc setFunc = (SetDataFunc)options->getPluginData("SetDataFunction");
                    if (checkFunc && setFunc)
                    {
                        osg::ref_ptr<osgVerse::ParticleCloud> pc = new osgVerse::ParticleCloud;
                        if (checkFunc(varname, dataType, dimNames, dimLength))
                        {
                            std::vector<unsigned char> buffer(totalDimSize * typeSize);
                            unsigned char* ptr = (unsigned char*)buffer.data();
                            status = nc_get_var(ncid, i, ptr);
                            if (status == 0 && setFunc(varname, dataType, dimLength, ptr))
                                clouds.push_back(pc);
                        }
                    }
                    else
                        OSG_NOTICE << "[ReaderWriterNetCDF] No check or set function set in options. "
                                   << "No particle cloud created" << std::endl;
                }
            }
            else if (type == 1)  // image
            {
                if (varndims < 2) continue; if (varndims == 2) dimLength.push_back(1);
                if (dimLength[idComp] > 4) continue;  // impossible image component number

                GLenum pixelFormat = GL_RGB, internalFormat = GL_RGB8;
                switch (vartype)
                {
                case NC_SHORT: case NC_USHORT:
                    switch (dimLength[idComp])
                    {
                    case 1: pixelFormat = GL_LUMINANCE; internalFormat = GL_LUMINANCE16; break;
                    case 2: pixelFormat = GL_LUMINANCE_ALPHA; internalFormat = GL_RG16; break;
                    case 3: pixelFormat = GL_RGB; internalFormat = GL_RGB16; break;
                    default: pixelFormat = GL_RGBA; internalFormat = GL_RGBA16; break;
                    } break;
                case NC_INT: case NC_UINT:
                    switch (dimLength[idComp])
                    {
                    case 1: pixelFormat = GL_LUMINANCE; internalFormat = GL_LUMINANCE32I_EXT; break;
                    case 2: pixelFormat = GL_LUMINANCE_ALPHA; internalFormat = GL_LUMINANCE_ALPHA32I_EXT; break;
                    case 3: pixelFormat = GL_RGB; internalFormat = GL_RGB32I_EXT; break;
                    default: pixelFormat = GL_RGBA; internalFormat = GL_RGBA32I_EXT; break;
                    } break;
                case NC_FLOAT:
                    switch (dimLength[idComp])
                    {
                    case 1: pixelFormat = GL_LUMINANCE; internalFormat = GL_LUMINANCE32F_ARB; break;
                    case 2: pixelFormat = GL_LUMINANCE_ALPHA; internalFormat = GL_LUMINANCE_ALPHA32F_ARB; break;
                    case 3: pixelFormat = GL_RGB; internalFormat = GL_RGB32F_ARB; break;
                    default: pixelFormat = GL_RGBA; internalFormat = GL_RGBA32F_ARB; break;
                    } break;
                default:
                    switch (dimLength[idComp])
                    {
                    case 1: pixelFormat = GL_LUMINANCE; internalFormat = GL_LUMINANCE8; break;
                    case 2: pixelFormat = GL_LUMINANCE_ALPHA; internalFormat = GL_LUMINANCE8_ALPHA8; break;
                    case 3: pixelFormat = GL_RGB; internalFormat = GL_RGB8; break;
                    default: pixelFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
                    } break;
                }

                image->allocateImage(dimLength[idX], dimLength[idY], 1, pixelFormat, dataType);
                image->setInternalTextureFormat(internalFormat);
                status = nc_get_var(ncid, i, image->data());
                if (status != 0)
                {
                    OSG_WARN << "[ReaderWriterNetCDF] Failed to load data: "
                             << nc_strerror(status) << std::endl; continue;
                }

                if (normalized)
                {
                    switch (internalFormat)
                    {
                    case GL_LUMINANCE16: image = normalizeAndCopyImage<short, unsigned char>(
                        image.get(), SHRT_MAX, -SHRT_MAX, pixelFormat, GL_LUMINANCE8); break;
                    case GL_LUMINANCE32I_EXT: image = normalizeAndCopyImage<int, unsigned char>(
                        image.get(), INT_MAX, -INT_MAX, pixelFormat, GL_LUMINANCE8); break;
                    case GL_LUMINANCE32F_ARB: image = normalizeAndCopyImage<float, unsigned char>(
                            image.get(), FLT_MAX, -FLT_MAX, pixelFormat, GL_LUMINANCE8); break;
                    default:
                        OSG_NOTICE << "[ReaderWriterNetCDF] Data not normalizable: " << std::hex << internalFormat
                                   << ", Components = " << dimLength[idComp] << std::dec << std::endl; break;
                    }
                }
                images.push_back(image);
            }
        }
        nc_close(ncid);

        if (images.size() > 1)
        {
            osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
            for (size_t i = 0; i < images.size(); ++i) seq->addImage(images[i]);
            return seq.get();
        }
        return images.empty() ? ReadResult::ERROR_IN_READING_FILE : ReadResult(images[0]);
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_netcdf, ReaderWriterNetCDF)
