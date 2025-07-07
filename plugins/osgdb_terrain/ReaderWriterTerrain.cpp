#include <osg/io_utils>
#include <osg/Version>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
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

    static uint16_t zigZagEncode(uint32_t n) { return (n << 1) ^ (n >> 31); }
    static uint32_t zigZagDecode(uint16_t n) { return (n >> 1) ^ (-(n & 1)); }

    void readVertices(std::istream& in)
    {
        vertices.u.resize(vertices.dataCount);
        vertices.v.resize(vertices.dataCount);
        vertices.h.resize(vertices.dataCount);
        in.read((char*)&(vertices.u[0]), sizeof(short) * vertices.u.size());
        in.read((char*)&(vertices.v[0]), sizeof(short) * vertices.v.size());
        in.read((char*)&(vertices.h[0]), sizeof(short) * vertices.h.size());

        uint32_t uValue = 0, vValue = 0, hValue = 0;
        for (unsigned int i = 0; i < vertices.dataCount; ++i)
        {
            uValue += zigZagDecode(vertices.u[i]); vertices.u[i] = uValue;
            vValue += zigZagDecode(vertices.v[i]); vertices.v[i] = vValue;
            hValue += zigZagDecode(vertices.h[i]); vertices.h[i] = hValue;
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

            in.read((char*)&dirCount, sizeof(int)); indices.west32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.west32[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.south32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.south32[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.east32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.east32[0]), sizeof(short) * dirCount);
            in.read((char*)&dirCount, sizeof(int)); indices.north32.resize(dirCount);
            if (dirCount > 0) in.read((char*)&(indices.north32[0]), sizeof(short) * dirCount);
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
            osg::Vec3f code(meshData.vertices.u[i], meshData.vertices.v[i], meshData.vertices.h[i]);
            for (int c = 0; c < 3; ++c) (*va)[i][c] = extentMin[c] + extent[c] * code[c] * INV_SHORT_MAX;
            if (cb->getFlatten()) (*va)[i][2] *= 0.0002f;  // show on 2D map
            (*ta)[i] = osg::Vec2(code[0], code[1]) * INV_SHORT_MAX;

            osg::Vec3d lla = cb->adjustLatitudeLongitudeAltitude((*va)[i], cb->getUseWebMercator());
            osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(lla);
            (*va)[i] = osg::Vec3(ecef * worldToLocal);
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
            meshData.readIndices(in, meshData.vertices.dataCount < 65536);
    }

    QuantizedMesh meshData;
};

// https://github.com/CesiumGS/quantized-mesh
class ReaderWriterTerrain : public osgDB::ReaderWriter
{
public:
    ReaderWriterTerrain()
    {
        supportsExtension("verse_terrain", "osgVerse pseudo-loader");
        supportsExtension("terrain", "Quantized-mesh terrain format");
    }

    virtual const char* className() const
    {
        return "[osgVerse] terrain format reader with quantized-mesh support";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_terrain");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return readNode(in, options);
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string fileName(path);
        std::string ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

        bool usePseudo = (ext == "verse_terrain");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getLowerCaseFileExtension(fileName);
        }

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;
        return readObject(in, options);
    }

    virtual ReadResult readNode(std::istream& fin, const Options* opt) const
    {
        osg::ref_ptr<TerrainGeometryHandler> tile = new TerrainGeometryHandler;
        tile->parseQuantizedMeshData(fin);
        if (tile->meshData.vertices.dataCount <= 0)
        { OSG_WARN << "Failed to load quantized mesh data" << std::endl; return NULL; }

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
        osg::ref_ptr<TerrainGeometryHandler> tile = new TerrainGeometryHandler;
        tile->parseQuantizedMeshData(fin);
        if (tile->meshData.vertices.dataCount <= 0)
        { OSG_WARN << "Failed to load quantized mesh data" << std::endl; return NULL; }
        return tile.get();
    }

protected:
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
