#include <osg/io_utils>
#include <osg/Version>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/SmoothingVisitor>
#include <modeling/Math.h>
#include <readerwriter/TileCallback.h>

struct QuantizedMesh
{
    struct Header
    {
        double center[3];  // The center of the tile in ECEF
        float minimumHeight, maximumHeight;  // Height range in the area covered by this tile
        double boundingCenter[3];  // The tiles bounding sphere
        double boundRadius;  // The tiles bounding sphere radius

        // The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame
        // If this point is below the horizon, the entire tile is below the horizon
        // See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information
        double horizonOcclusion[3];
    };

    struct VertexData
    {
        unsigned int dataCount;
        std::vector<unsigned short> u, v, h;
        std::vector<unsigned int> x, y, z;
    };

    struct IndexData
    {
        unsigned int triangleCount;
        std::vector<unsigned short> triangles16;
        std::vector<unsigned int> triangles32;
        std::vector<unsigned short> west16, south16, east16, north16;
        std::vector<unsigned int> west32, south32, east32, north32;
    };

    Header header;
    VertexData vertices;
    IndexData indices;

    static uint32_t zigZagDecode(uint16_t n) { return (n >> 1) ^ (-(n & 1)); }

    void readVertices(std::istream& in)
    {
        vertices.u.resize(vertices.dataCount); vertices.x.resize(vertices.dataCount);
        vertices.v.resize(vertices.dataCount); vertices.y.resize(vertices.dataCount);
        vertices.h.resize(vertices.dataCount); vertices.z.resize(vertices.dataCount);
        in.read((char*)&(vertices.u[0]), sizeof(short) * vertices.u.size());
        in.read((char*)&(vertices.v[0]), sizeof(short) * vertices.v.size());
        in.read((char*)&(vertices.h[0]), sizeof(short) * vertices.h.size());

        uint32_t uValue = 0, vValue = 0, hValue = 0;
        for (unsigned int i = 0; i < vertices.dataCount; ++i)
        {
            uValue += zigZagDecode(vertices.u[i]); vertices.x[i] = uValue;
            vValue += zigZagDecode(vertices.v[i]); vertices.y[i] = vValue;
            hValue += zigZagDecode(vertices.h[i]); vertices.z[i] = hValue;
        }
    }

    void readIndices(std::istream& in, bool isIndex16)
    {
        unsigned int dirCount = 0;
        if (isIndex16)
        {
            indices.triangles16.resize(indices.triangleCount * 3);
            in.read((char*)&(indices.triangles16[0]), sizeof(short) * indices.triangles16.size());
            indexDecode(indices.triangles16);

            in.read((char*)&dirCount, sizeof(int)); indices.west16.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.west16[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.south16.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.south16[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.east16.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.east16[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.north16.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.north16[0]), sizeof(short) * dirCount);
        }
        else
        {
            const size_t offset = 3 * sizeof(unsigned short) * vertices.dataCount;
            if ((offset % 4) != 0)  // enforce 4-byte alignment
            { unsigned short empty = 0; in.read((char*)&empty, sizeof(short)); }

            indices.triangles32.resize(indices.triangleCount * 3);
            in.read((char*)&(indices.triangles32[0]), sizeof(int) * indices.triangles32.size());
            indexDecode(indices.triangles32);

            // Edge indices use the same width as the triangle indices (uint32 here)
            in.read((char*)&dirCount, sizeof(int)); indices.west32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.west32[0]), sizeof(int) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.south32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.south32[0]), sizeof(int) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.east32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.east32[0]), sizeof(int) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.north32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.north32[0]), sizeof(int) * dirCount);
        }
    }

    template<typename T> void indexDecode(std::vector<T>& indices)
    {
        T highest = 0;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            T code = indices[i]; indices[i] = highest - code;
            if (code == 0) ++highest;
        }
    }
};

struct TerrainGeometryHandler : public osgVerse::TileGeometryHandler
{
    virtual osg::Geometry* create(const osgVerse::TileCallback* cb, const osg::Matrix& localToWorld,
                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                  double width, double height) const
    {
        const float INV_SHORT_MAX = 1.0f / 32767.0f;
        osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
        osg::Vec3 extentMin(tileMin[0], tileMin[1], meshData.header.minimumHeight);
        osg::Vec3 extentMax(tileMax[0], tileMax[1], meshData.header.maximumHeight);
        osg::Vec3 extent = extentMax - extentMin;

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(meshData.vertices.dataCount);
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(meshData.vertices.dataCount);
        for (size_t i = 0; i < va->size(); ++i)
        {
            osg::Vec3f code(meshData.vertices.x[i], meshData.vertices.y[i], meshData.vertices.z[i]);
            for (int c = 0; c < 3; ++c) (*va)[i][c] = extentMin[c] + extent[c] * code[c] * INV_SHORT_MAX;
            if (cb && cb->getFlatten()) (*va)[i][2] *= 0.0002f;  // show on 2D map
            (*ta)[i] = osg::Vec2(code[0], code[1]) * INV_SHORT_MAX;

            if (cb)
            {
                osg::Vec3d lla = cb->adjustLatitudeLongitudeAltitude((*va)[i], cb->getUseWebMercator());
                osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(lla);
                (*va)[i] = osg::Vec3(ecef * worldToLocal);
            }
        }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va.get()); geom->setTexCoordArray(0, ta.get());
        if (!meshData.indices.triangles16.empty())
        {
            osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
            de->assign(meshData.indices.triangles16.begin(), meshData.indices.triangles16.end());
            geom->addPrimitiveSet(de.get());
        }
        else if (!meshData.indices.triangles32.empty())
        {
            osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
            de->assign(meshData.indices.triangles32.begin(), meshData.indices.triangles32.end());
            geom->addPrimitiveSet(de.get());
        }
        osgUtil::SmoothingVisitor::smooth(*geom);
        return geom.release();
    }

    void parseQuantizedMeshData(std::istream& in)
    {
        for (int i = 0; i < 3; ++i) in.read((char*)&(meshData.header.center[i]), sizeof(double));
        in.read((char*)&(meshData.header.minimumHeight), sizeof(float));
        in.read((char*)&(meshData.header.maximumHeight), sizeof(float));
        for (int i = 0; i < 3; ++i) in.read((char*)&(meshData.header.boundingCenter[i]), sizeof(double));
        in.read((char*)&(meshData.header.boundRadius), sizeof(double));
        for (int i = 0; i < 3; ++i) in.read((char*)&(meshData.header.horizonOcclusion[i]), sizeof(double));

        in.read((char*)&(meshData.vertices.dataCount), sizeof(int));
        if (meshData.vertices.dataCount > 0) meshData.readVertices(in);

        in.read((char*)&(meshData.indices.triangleCount), sizeof(int));
        if (meshData.indices.triangleCount > 2)
            meshData.readIndices(in, meshData.vertices.dataCount <= 65536);  // 32-bit only if >65536
    }

    QuantizedMesh meshData;
};

// Builds a regular terrain grid from a decoded Terrarium height image (float, meters),
// following the same handler pattern as the quantized-mesh one above
struct TerrariumGeometryHandler : public osgVerse::TileGeometryHandler
{
    virtual osg::Geometry* create(const osgVerse::TileCallback* cb, const osg::Matrix& localToWorld,
                                  const osg::Vec3d& tileMin, const osg::Vec3d& tileMax,
                                  double width, double height) const
    {
        if (!heightImage.valid()) return NULL;
        const unsigned int numRows = 16, numCols = 16;  // same grid resolution as image-based elevation
        osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
        double elevationScale = cb ? cb->getElevationScale() : 1.0;
        double invW = width / (double)(numCols - 1), invH = height / (double)(numRows - 1);

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(numRows * numCols);
        osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(numRows * numCols);
        for (unsigned int y = 0; y < numRows; ++y)
        {
            for (unsigned int x = 0; x < numCols; ++x)
            {
                unsigned int vi = x + y * numCols;
                osg::Vec2 uv((double)x * invW / width, (double)y * invH / height);
                double altitude = heightImage->getColor(uv)[0] * elevationScale;
                if (cb && cb->getFlatten()) altitude *= 0.0002;  // show on 2D map

                if (cb)
                {
                    osg::Vec3d lla = cb->adjustLatitudeLongitudeAltitude(
                        tileMin + osg::Vec3d((double)x * invW, (double)y * invH, altitude),
                        cb->getUseWebMercator());
                    osg::Vec3d ecef = cb->convertToECEF(lla);
                    (*va)[vi] = osg::Vec3(ecef * worldToLocal);
                }
                else
                {
                    (*va)[vi] = osg::Vec3(tileMin.x() + (double)x * invW,
                                          tileMin.y() + (double)y * invH, altitude);
                }
                (*ta)[vi] = uv;
            }
        }

        osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
        for (unsigned int y = 0; y < numRows - 1; ++y)
        {
            for (unsigned int x = 0; x < numCols - 1; ++x)
            {
                unsigned int vi = x + y * numCols;
                de->push_back(vi); de->push_back(vi + 1); de->push_back(vi + numCols);
                de->push_back(vi + numCols); de->push_back(vi + 1); de->push_back(vi + numCols + 1);
            }
        }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va.get()); geom->setTexCoordArray(0, ta.get());
        geom->addPrimitiveSet(de.get());
        osgUtil::SmoothingVisitor::smooth(*geom);
        return geom.release();
    }

    osg::ref_ptr<osg::Image> heightImage;  // decoded GL_R32F heights in meters
};

// Terrarium terrain tiles (AWS / Mapzen "elevation-tiles-prod"): 8-bit RGB PNG images
// encoding height in meters as (R * 256 + G + B / 256) - 32768.
// See https://github.com/tilezen/joerd/blob/master/docs/formats.md
static osg::Image* decodeTerrariumImage(const osg::Image* src)
{
    if (!src) return NULL;
    int w = src->s(), h = src->t();
    osg::ref_ptr<osg::Image> dst = new osg::Image;
    dst->allocateImage(w, h, 1, GL_RED, GL_FLOAT);  // requires GL3/GLES3+ (GL_R32F)
    dst->setInternalTextureFormat(GL_R32F);
    float* out = (float*)dst->data();
    for (int t = 0; t < h; ++t)
    {
        for (int s = 0; s < w; ++s)
        {
            // the integer-indexed getColor() normalizes the source pixel layout (RGB/RGBA)
            osg::Vec4 c = src->getColor(s, t);
            unsigned char r = (unsigned char)(c.r() * 255.0f + 0.5f);
            unsigned char g = (unsigned char)(c.g() * 255.0f + 0.5f);
            unsigned char b = (unsigned char)(c.b() * 255.0f + 0.5f);
            out[t * w + s] = ((float)r * 256.0f + (float)g + (float)b / 256.0f) - 32768.0f;
        }
    }
    return dst.release();
}

// https://github.com/CesiumGS/quantized-mesh
class ReaderWriterTerrain : public osgDB::ReaderWriter
{
public:
    ReaderWriterTerrain()
    {
        supportsExtension("verse_terrain", "osgVerse pseudo-loader");
        supportsExtension("terrain", "Quantized-mesh terrain format");
        supportsExtension("terrarium", "Terrarium PNG elevation tiles (AWS/Mapzen)");
    }

    virtual const char* className() const
    {
        return "[osgVerse] terrain format reader with quantized-mesh and terrarium support";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        return readNode(in, lOptions.get());
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        return readObject(in, lOptions.get());
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));
        return readImage(in, lOptions.get());
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        if (!isTerrariumExtension(getStreamExtension(options)))
            return ReadResult::FILE_NOT_HANDLED;  // quantized-mesh is not an image format
        osg::ref_ptr<osg::Image> heights = readTerrariumImage(fin, options);
        if (!heights.valid()) return ReadResult::ERROR_IN_READING_FILE;
        return ReadResult(heights.get());
    }

    virtual ReadResult readNode(std::istream& fin, const Options* opt) const
    {
        osg::ref_ptr<osgVerse::TileGeometryHandler> tile = readGeometryHandler(fin, opt);
        if (!tile.valid()) return ReadResult::FILE_NOT_HANDLED;

        osg::Vec3d tileMin, tileMax; double tileWidth = 0.0, tileHeight = 0.0;
        int x = opt ? atoi(opt->getPluginStringData("X").c_str()) : 0;
        int y = opt ? atoi(opt->getPluginStringData("Y").c_str()) : 0;
        int z = opt ? atoi(opt->getPluginStringData("Z").c_str()) : 0;
        computeTileExtent(x, y, z, tileMin, tileMax, tileWidth, tileHeight);

        osg::ref_ptr<osg::Geometry> geom = tile->create(
            NULL, osg::Matrix(), tileMin, tileMax, tileWidth, tileHeight);
        if (geom.valid())
        {
            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(geom.get()); return geode;
        }
        return NULL;
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        osg::ref_ptr<osgVerse::TileGeometryHandler> tile = readGeometryHandler(fin, options);
        return tile.valid() ? ReadResult(tile.get()) : ReadResult(ReadResult::FILE_NOT_HANDLED);
    }

protected:
    // Create the tile geometry handler of the format selected by the (inner) file extension
    osgVerse::TileGeometryHandler* readGeometryHandler(std::istream& fin, const Options* options) const
    {
        if (isTerrariumExtension(getStreamExtension(options)))
        {
            osg::ref_ptr<osg::Image> heights = readTerrariumImage(fin, options);
            if (!heights.valid()) return NULL;

            osg::ref_ptr<TerrariumGeometryHandler> tile = new TerrariumGeometryHandler;
            tile->heightImage = heights;
            return tile.release();
        }

        osg::ref_ptr<TerrainGeometryHandler> tile = new TerrainGeometryHandler;
        tile->parseQuantizedMeshData(fin);
        if (tile->meshData.vertices.dataCount <= 0)
        { OSG_WARN << "Failed to load quantized mesh data" << std::endl; return NULL; }
        return tile.release();
    }

    // The real (inner) file name is passed by callers such as osgdb_web via the
    // "STREAM_FILENAME" plugin data when reading from a stream (see osgdb_mesh)
    static std::string getStreamExtension(const Options* options)
    {
        std::string filename = options ? options->getPluginStringData("STREAM_FILENAME") : "";
        std::string ext = osgDB::getLowerCaseFileExtension(filename);
        size_t queryInExt = ext.find("?");  // remove query string if mixed with extension
        if (queryInExt != std::string::npos) ext = ext.substr(0, queryInExt);
        return ext;
    }

    // Terrarium data is always PNG: ".terrarium" files are PNG payloads, and a
    // pseudo path like "xxx.png.verse_terrain" routes a plain PNG to this plugin
    static bool isTerrariumExtension(const std::string& ext)
    { return (ext == "terrarium" || ext == "png"); }

    osg::Image* readTerrariumImage(std::istream& fin, const Options* options) const
    {
        // The terrarium payload is a plain PNG: decode it with the standard image
        // readers (stock osgdb_png, or the stb-based verse reader as a fallback)
        osg::ref_ptr<osg::Image> src;
        const char* imageExts[2] = { "png", "verse_image" };
        for (int i = 0; i < 2 && !src.valid(); ++i)
        {
            osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(imageExts[i]);
            if (rw == NULL) continue;
            ReadResult rr = rw->readImage(fin, options);
            if (rr.validImage()) src = rr.takeImage();
            else { fin.clear(); fin.seekg(0, std::ios::beg); }  // retry from the stream start
        }

        if (!src.valid())
        {
            OSG_WARN << "[ReaderWriterTerrain] Failed to decode terrarium PNG data" << std::endl;
            return NULL;
        }
        return decodeTerrariumImage(src.get());
    }

    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_terrain");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }
        return fileName;
    }

    void computeTileExtent(int x, int y, int z, osg::Vec3d& tileMin, osg::Vec3d& tileMax,
                           double& tileWidth, double& tileHeight, bool bottomLeft = false) const
    {
        double multiplier = pow(0.5, double(z));
        osg::Vec3d extentMin = osg::Vec3d(-180.0, -90.0, 0.0), extentMax = osg::Vec3d(180.0, 90.0, 0.0);
        tileWidth = multiplier * (extentMax.x() - extentMin.x());
        tileHeight = multiplier * (extentMax.y() - extentMin.y());
        if (!bottomLeft)
        {
            osg::Vec3d origin(extentMin.x(), extentMax.y(), extentMin.z());
            tileMin = origin + osg::Vec3d(double(x) * tileWidth, -double(y + 1) * tileHeight, 0.0);
            tileMax = origin + osg::Vec3d(double(x + 1) * tileWidth, -double(y) * tileHeight, 1.0);
        }
        else
        {
            tileMin = extentMin + osg::Vec3d(double(x) * tileWidth, double(y) * tileHeight, 0.0);
            tileMax = extentMin + osg::Vec3d(double(x + 1) * tileWidth, double(y + 1) * tileHeight, 1.0);
        }
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_terrain, ReaderWriterTerrain)
