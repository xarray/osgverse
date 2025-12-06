#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include "modeling/GaussianGeometry.h"
#include "readerwriter/Utilities.h"
#include "3rdparty/picojson.h"
#include "3rdparty/mio.hpp"

struct XGridsNodeChunk
{
    uint16_t col = 0, row = 0;
    std::vector<uint32_t> numSplats, byteSizes;
    std::vector<uint64_t> offsets;
};

static osg::Vec3 parseScale(const osg::Vec3& s0, const osg::Vec3& sMin, const osg::Vec3& sMax)
{
    osg::Vec3 ratio(s0[0] / 65535.0f, s0[1] / 65535.0f, s0[2] / 65535.0f);
    return osg::Vec3(sMin[0] * (1.0f - ratio[0]) + sMax[0] * ratio[0],
                     sMin[1] * (1.0f - ratio[1]) + sMax[1] * ratio[1],
                     sMin[2] * (1.0f - ratio[2]) + sMax[2] * ratio[2]);
}

static osg::Vec4 parseQuat(uint32_t value)
{
    static const int QLut[16] = { 3, 0, 1, 2,  0, 3, 1, 2, 0, 1, 3, 2, 0, 1, 2, 3 };
    static const float sqrt2 = 1.414213562373095, rsqrt2 = 0.7071067811865475;
    float x = (value & 1023) / 1023.0f, y = ((value >> 10) & 1023) / 1023.0f,
          z = ((value >> 20) & 1023) / 1023.0f, idF = ((value >> 30) & 3) / 3.0f;

    osg::Vec3 xyz = osg::Vec3(x, y, z) * sqrt2 - osg::Vec3(rsqrt2, rsqrt2, rsqrt2);
    float w = sqrt(1.0f - osg::clampBetween(xyz * xyz, 0.0f, 1.0f));
    uint32_t idx = (uint32_t)round(idF * 3.0f); osg::Vec4 q(xyz, w);
    return osg::Vec4(q[QLut[idx * 4]], q[QLut[idx * 4 + 1]], q[QLut[idx * 4 + 2]], q[QLut[idx * 4 + 3]]);
}

osg::Geometry* loadGeometryFromXGrids(const std::vector<unsigned char>& data, uint32_t numSplats,
                                      int degrees, const osg::Vec3& sMin, const osg::Vec3& sMax)
{
    osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array(numSplats), gD0 = new osg::Vec4Array(numSplats),
                                 bD0 = new osg::Vec4Array(numSplats);
    osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array(numSplats), scale = new osg::Vec3Array(numSplats);
    osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array(numSplats);
    osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray(numSplats);
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_POINTS);

    const static float kSH_C0 = 0.28209479177387814;
    for (uint32_t i = 0; i < numSplats; ++i)
    {
        float valueF[3]; uint16_t valueS[3]; uint8_t valueB[4]; uint32_t valueI = 0, index = i * 32;
        for (uint32_t n = 0; n < 3; ++n) memcpy(&valueF[n], &data[index + n * 4], sizeof(float));
        (*pos)[i].set(valueF[0], valueF[1], valueF[2]);
        for (uint32_t n = 0; n < 4; ++n) memcpy(&valueB[n], &data[index + 12 + n], sizeof(uint8_t));
        (*rD0)[i].set(((float)valueB[0] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f);
        (*gD0)[i].set(((float)valueB[1] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f);
        (*bD0)[i].set(((float)valueB[2] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f);
        (*alpha)[i] = (float)valueB[3] / 255.0f;
        for (uint32_t n = 0; n < 3; ++n) memcpy(&valueS[n], &data[index + 16 + n * 2], sizeof(uint16_t));
        (*scale)[i] = parseScale(osg::Vec3((float)valueS[0], (float)valueS[1], (float)valueS[2]), sMin, sMax);
        memcpy(&valueI, &data[index + 22], sizeof(uint32_t)); (*rot)[i] = parseQuat(valueI); de->push_back(i);
        //for (uint32_t n = 0; n < 3; ++n) memcpy(&valueS[n], &data[index + 26 + n * 2], sizeof(uint16_t));
        // TODO: normal?
    }

#if true
    osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry;
    geom->setShDegrees(degrees); geom->setPosition(pos.get());
    geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
    geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
#else
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(pos.get());
#endif
    geom->addPrimitiveSet(de.get()); return geom.release();
}

osg::ref_ptr<osg::Node> loadSplatFromXGrids(std::istream& in, const std::string& path)
{
    std::string reserved0, reserved1, err;
    picojson::value document; err = picojson::parse(document, in);
    if (!err.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to parse XGrids' LCC data: " << err << std::endl;
        return NULL;
    }

    // Read information from .lcc
    std::string name = document.get("name").get<std::string>();
    std::string description = document.get("description").get<std::string>();
    std::string fileType = document.get("fileType").get<std::string>();  // Portable / Quality
    uint32_t totalLevel = (uint32_t)document.get("totalLevel").get<double>();
    uint32_t indexDataSize = (uint32_t)document.get("indexDataSize").get<double>();
    uint32_t epsg = (uint32_t)document.get("epsg").get<double>();
    double cellLengthX = document.get("cellLengthX").get<double>();
    double cellLengthY = document.get("cellLengthY").get<double>();

    osg::BoundingBoxd worldBox; picojson::value& bbJson = document.get("boundingBox");
    picojson::array& bbMinJson = bbJson.get("min").get<picojson::array>();
    picojson::array& bbMaxJson = bbJson.get("max").get<picojson::array>();
    if (bbMinJson.size() > 2 && bbMaxJson.size() > 2)
    {
        worldBox._min.set(bbMinJson[0].get<double>(), bbMinJson[1].get<double>(), bbMinJson[2].get<double>());
        worldBox._max.set(bbMaxJson[0].get<double>(), bbMaxJson[1].get<double>(), bbMaxJson[2].get<double>());
    }

    picojson::array& ofJson = document.get("offset").get<picojson::array>();
    picojson::array& shJson = document.get("shift").get<picojson::array>();
    picojson::array& scJson = document.get("scale").get<picojson::array>();
    picojson::array& lodJson = document.get("splats").get<picojson::array>();
    picojson::array& attrJson = document.get("attributes").get<picojson::array>();
    osg::Vec3d gOffset, gShift, gScale; std::vector<uint32_t> lodNumbers;
    if (ofJson.size() > 2) gOffset.set(ofJson[0].get<double>(), ofJson[1].get<double>(), ofJson[2].get<double>());
    if (shJson.size() > 2) gShift.set(shJson[0].get<double>(), shJson[1].get<double>(), shJson[2].get<double>());
    if (scJson.size() > 2) gScale.set(scJson[0].get<double>(), scJson[1].get<double>(), scJson[2].get<double>());
    for (size_t i = 0; i < lodJson.size(); ++i) lodNumbers.push_back((uint32_t)lodJson[i].get<double>());

    osg::Vec3 scaleRange[2], envScaleRange[2], shRange[2], envShRange[2]; float alphaRange[2];
    for (size_t i = 0; i < attrJson.size(); ++i)
    {
        std::string name = attrJson[i].get("name").get<std::string>();
        picojson::array& minJson = attrJson[i].get("min").get<picojson::array>();
        picojson::array& maxJson = attrJson[i].get("max").get<picojson::array>();
        if (minJson.size() > 2 && maxJson.size() > 2)
        {
            osg::Vec3d minV(minJson[0].get<double>(), minJson[1].get<double>(), minJson[2].get<double>());
            osg::Vec3d maxV(maxJson[0].get<double>(), maxJson[1].get<double>(), maxJson[2].get<double>());
            if (name == "scale") { scaleRange[0] = minV;scaleRange[1] = maxV; }
            else if (name == "shcoef") { shRange[0] = minV; shRange[1] = maxV; }
            else if (name == "envscale") { envScaleRange[0] = minV; envScaleRange[1] = maxV; }
            else if (name == "envshcoef") { envShRange[0] = minV; envShRange[1] = maxV; }
        }
        else if (minJson.size() > 0 && maxJson.size() > 0)
        {
            if (name == "opacity")
            { alphaRange[0] = minJson[0].get<double>(); alphaRange[1] = maxJson[0].get<double>(); }
        }
    }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName(name); root->addDescription(description);
    root->setMatrix(osg::Matrix::scale(gScale) * osg::Matrix::translate(gOffset));

    // Read information from index.bin
    std::vector<unsigned char> indexData = osgVerse::loadFileData(path + "/index.bin", reserved0, reserved1);
    if (indexData.empty()) indexData = osgVerse::loadFileData(path + "/Index.bin", reserved0, reserved1);
    if (indexData.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to load XGrids' index.bin at " << path << std::endl;
        return root;
    }

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss.write((char*)indexData.data(), indexData.size());

    std::vector<XGridsNodeChunk> dataChunks; int numCols = 0, numRows = 0;
    while (!ss.eof() && !ss.fail())
    {
        XGridsNodeChunk chunk;
        ss.read((char*)&chunk.col, sizeof(uint16_t));
        ss.read((char*)&chunk.row, sizeof(uint16_t));
        if (ss.eof() || ss.fail()) break;
        if (numCols <= chunk.col) numCols = chunk.col;
        if (numRows <= chunk.row) numRows = chunk.row;

        for (size_t j = 0; j < lodNumbers.size(); ++j)
        {
            uint32_t v0 = 0; uint64_t v1 = 0;
            ss.read((char*)&v0, sizeof(uint32_t)); chunk.numSplats.push_back(v0);
            ss.read((char*)&v1, sizeof(uint64_t)); chunk.offsets.push_back(v1);
            ss.read((char*)&v0, sizeof(uint32_t)); chunk.byteSizes.push_back(v0);
        }
        dataChunks.push_back(chunk);
    }

    // Load data.bin to obtain main point cloud data
    std::string dataBinPath = path + "/data.bin";
    if (!osgDB::fileExists(dataBinPath)) dataBinPath = path + "/Data.bin";
    if (!osgDB::fileExists(dataBinPath))
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to load XGrids' data.bin at " << path << std::endl;
        return root;
    }

    std::error_code error; mio::mmap_source ro_mmap;
    ro_mmap.map(dataBinPath, error);
    if (error)
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to map XGrids' data.bin: " << error << std::endl;
        return root;
    }

    // Create LODS and read from data.bin & shcoef.bin
    uint64_t totalSize = ro_mmap.size(); int deg = (fileType == "Portable") ? 1 : 3;
    for (size_t i = 0; i < dataChunks.size(); ++i)
    {
        XGridsNodeChunk& chunk = dataChunks[i];
        osg::ref_ptr<osg::LOD> lod = new osg::LOD;
        lod->setName(std::to_string(chunk.col) + "_" + std::to_string(chunk.row));
        lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);

        osg::BoundingBox localBox;
        localBox._min = worldBox._min + osg::Vec3(cellLengthX * chunk.col, cellLengthY * chunk.row, 0.0f);
        localBox._max = localBox._min + osg::Vec3(cellLengthX, cellLengthY, worldBox.zMax() - worldBox.zMin());
        lod->setCenter(localBox.center()); lod->setRadius(localBox.radius());

        std::vector<unsigned char> data; float d = localBox.radius() * 1.2f;
        for (int i = (int)chunk.offsets.size() - 1; i >= 0; --i)
        {
            uint64_t start = chunk.offsets[i], end = chunk.byteSizes[i]; end += start;
            if (end <= start || chunk.numSplats[i] == 0) continue;
            else if (totalSize <= end) data.assign(ro_mmap.begin() + start, ro_mmap.end());
            else data.assign(ro_mmap.begin() + start, ro_mmap.begin() + end);
            
            osg::ref_ptr<osg::Geometry> geom = loadGeometryFromXGrids(
                data, chunk.numSplats[i], deg, scaleRange[0], scaleRange[1]);
            if (geom.valid())
            {
                osg::ref_ptr<osg::Geode> child = new osg::Geode;
                child->addDrawable(geom.get());
                // TODO: load SH?
                lod->addChild(child.get(), d * i, d * (i + 1));  // FIXME: better range?
            }
        }

        const osg::LOD::RangeList& ranges = lod->getRangeList();
        lod->setRange(0, ranges[0].first, FLT_MAX);
        root->addChild(lod.get());
    }
    ro_mmap.unmap(); return root;
}
