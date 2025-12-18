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
#include "modeling/Utilities.h"
#include "readerwriter/Utilities.h"
#include "3rdparty/picojson.h"
#include "3rdparty/mio.hpp"

namespace
{
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

    osg::Vec3 parsePacked11(unsigned int enc)
    {
        return osg::Vec3(float(enc & 2047) / 2047.0f,
                         float((enc >> 11) & 1023) / 1023.0f,
                         float((enc >> 21) & 2047) / 2047.0f);
    }

    std::vector<osg::Vec3> parseShcoef(const std::vector<unsigned int>& raw, const osg::Vec3& sMin, const osg::Vec3& sMax)
    {
        std::vector<osg::Vec3> result(raw.size());
        for (size_t i = 0; i < raw.size(); ++i)
        {
            osg::Vec3 ratio = parsePacked11(raw[i]);
            result[i] = osg::Vec3(sMin[0] * (1.0f - ratio[0]) + sMax[0] * ratio[0],
                                  sMin[1] * (1.0f - ratio[1]) + sMax[1] * ratio[1],
                                  sMin[2] * (1.0f - ratio[2]) + sMax[2] * ratio[2]);
        }
        return result;
    }
}

void applyShcoefFromXGrids(osg::Geometry* geomInput, const std::vector<unsigned char>& data, uint32_t numSplats,
                           const osg::Vec3& sMin, const osg::Vec3& sMax)
{
    osgVerse::GaussianGeometry* geom = dynamic_cast<osgVerse::GaussianGeometry*>(geomInput);
    size_t fixedSize = sizeof(int) * 16; if (!geom || !numSplats) return;

    osg::ref_ptr<osg::Vec4Array> rD0 = geom->getShRed(0), gD0 = geom->getShGreen(0), bD0 = geom->getShBlue(0);
    osg::ref_ptr<osg::Vec4Array> rD1 = new osg::Vec4Array(numSplats), gD1 = new osg::Vec4Array(numSplats),
                                 bD1 = new osg::Vec4Array(numSplats), rD2 = new osg::Vec4Array(numSplats),
                                 gD2 = new osg::Vec4Array(numSplats), bD2 = new osg::Vec4Array(numSplats),
                                 rD3 = new osg::Vec4Array(numSplats), gD3 = new osg::Vec4Array(numSplats),
                                 bD3 = new osg::Vec4Array(numSplats);
    for (uint32_t i = 0; i < numSplats; ++i)
    {
        std::vector<unsigned int> rawData(16);
        memcpy(&rawData[0], &data[i * fixedSize], fixedSize);

        std::vector<osg::Vec3> result = parseShcoef(rawData, sMin, sMax);
        if (result.size() < 15) continue;
        (*rD0)[i].y() = result[0].x(); (*gD0)[i].y() = result[0].x(); (*bD0)[i].y() = result[0].x();
        (*rD0)[i].z() = result[1].y(); (*gD0)[i].z() = result[1].y(); (*bD0)[i].z() = result[1].y();
        (*rD0)[i].w() = result[2].z(); (*gD0)[i].w() = result[2].z(); (*bD0)[i].w() = result[2].z();
        for (int k = 0, m = 0; k < 4; ++k)
        {
            m = k + 3; (*rD1)[i][k] = result[m].x(); (*gD1)[i][k] = result[m].y(); (*bD1)[i][k] = result[m].z();
            m = k + 7; (*rD2)[i][k] = result[m].x(); (*gD2)[i][k] = result[m].y(); (*bD2)[i][k] = result[m].z();
            m = k + 11; (*rD3)[i][k] = result[m].x(); (*gD3)[i][k] = result[m].y(); (*bD3)[i][k] = result[m].z();
        }
    }
    geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
    geom->setShRed(1, rD1.get()); geom->setShGreen(1, gD1.get()); geom->setShBlue(1, bD1.get());
    geom->setShRed(2, rD2.get()); geom->setShGreen(2, gD2.get()); geom->setShBlue(2, bD2.get());
    geom->setShRed(3, rD3.get()); geom->setShGreen(3, gD3.get()); geom->setShBlue(3, bD3.get());
}

osg::Geometry* loadGeometryFromXGrids(const std::vector<unsigned char>& data, uint32_t numSplats,
                                      int degrees, const osg::Vec3& sMin, const osg::Vec3& sMax)
{
    osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array(numSplats), gD0 = new osg::Vec4Array(numSplats),
                                 bD0 = new osg::Vec4Array(numSplats);
    osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array(numSplats), scale = new osg::Vec3Array(numSplats);
    osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array(numSplats);
    osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray(numSplats);

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
        memcpy(&valueI, &data[index + 22], sizeof(uint32_t)); (*rot)[i] = parseQuat(valueI);
        //for (uint32_t n = 0; n < 3; ++n) memcpy(&valueS[n], &data[index + 26 + n * 2], sizeof(uint16_t));
        // TODO: normal?
    }

    osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry;
    geom->setShDegrees(degrees); geom->setPosition(pos.get());
    geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
    geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
    return geom.release();
}

osg::Node* loadCollisionFromXGrids(std::istream& in)
{
    unsigned int magic = 0, rev = 0; in.read((char*)&magic, sizeof(unsigned int));
    unsigned int version = 0; in.read((char*)&version, sizeof(unsigned int));
    unsigned int headLength = 0; in.read((char*)&headLength, sizeof(unsigned int));
    osg::Vec3 bbMin, bbMax; float cellLengthX = 0.0f, cellLengthY = 0.0f;
    in.read((char*)bbMin.ptr(), sizeof(osg::Vec3)); in.read((char*)bbMax.ptr(), sizeof(osg::Vec3));
    in.read((char*)&cellLengthX, sizeof(float)); in.read((char*)&cellLengthY, sizeof(float));

    struct MeshInfo { uint32_t x, y, vNum, fNum, bvhSize; uint64_t offset, bytes; };
    unsigned int meshCount = 0; in.read((char*)&meshCount, sizeof(unsigned int));
    std::vector<MeshInfo> meshInfoList(meshCount);
    for (unsigned int i = 0; i < meshCount; ++i)
    {
        MeshInfo& info = meshInfoList[i];
        in.read((char*)&(info.x), sizeof(uint32_t)); in.read((char*)&(info.y), sizeof(uint32_t));
        in.read((char*)&(info.offset), sizeof(uint64_t)); in.read((char*)&(info.bytes), sizeof(uint64_t));
        in.read((char*)&(info.vNum), sizeof(uint32_t)); in.read((char*)&(info.fNum), sizeof(uint32_t));
        in.read((char*)&(info.bvhSize), sizeof(uint32_t)); in.read((char*)&rev, sizeof(uint32_t));
    }

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    for (unsigned int i = 0; i < meshCount; ++i)
    {
        const MeshInfo& info = meshInfoList[i];
        std::vector<osg::Vec3> vertices(info.vNum); std::vector<uint32_t> indices(info.fNum * 3);
        in.read((char*)vertices.data(), sizeof(osg::Vec3) * vertices.size());
        in.read((char*)indices.data(), sizeof(uint32_t) * indices.size());

        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(vertices.begin(), vertices.end());
        osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES, indices.begin(), indices.end());
        osg::Geometry* geom = osgVerse::createGeometry(va.get(), NULL, osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f), de.get());
        geom->setName(std::to_string(info.x) + "_" + std::to_string(info.y)); geode->addDrawable(geom);

        std::vector<unsigned char> bvhBuffer(info.bvhSize);
        if (!bvhBuffer.empty()) in.read((char*)bvhBuffer.data(), sizeof(char) * info.bvhSize);
    }
    return geode.release();
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
    int deg = (fileType == "Portable") ? 0 : 3; std::error_code error;
    std::string dataBinPath = path + "/data.bin", shBinPath = path + "/shcoef.bin";
    if (!osgDB::fileExists(dataBinPath)) dataBinPath = path + "/Data.bin";
    if (!osgDB::fileExists(shBinPath)) shBinPath = path + "/Shcoef.bin";
    if (!osgDB::fileExists(dataBinPath))
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to load XGrids' data.bin at "
                 << path << std::endl; return root;
    }

    mio::mmap_source ro_mmap, ro_mmap2; ro_mmap.map(dataBinPath, error);
    if (error)
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to map XGrids' data.bin: "
                 << error << std::endl; return root;
    }

    if (deg > 0 && osgDB::fileExists(shBinPath))
    {
        ro_mmap2.map(shBinPath, error);
        if (error)
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to map XGrids' shcoef.bin: "
                     << error << std::endl; deg = 0;
        }
    }

    // Create LODS and read from data.bin & shcoef.bin
    uint64_t totalSize = ro_mmap.size(), totalSize2 = (deg > 0) ? ro_mmap2.size() : 0;
    for (size_t k = 0; k < dataChunks.size(); ++k)
    {
        XGridsNodeChunk& chunk = dataChunks[k];
        osg::ref_ptr<osg::LOD> lod = new osg::LOD;
        lod->setName(std::to_string(chunk.col) + "_" + std::to_string(chunk.row));
        lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);

        osg::BoundingBox localBox;
        localBox._min = worldBox._min + osg::Vec3(cellLengthX * chunk.col, cellLengthY * chunk.row, 0.0f);
        localBox._max = localBox._min + osg::Vec3(cellLengthX, cellLengthY, worldBox.zMax() - worldBox.zMin());
        lod->setCenter(localBox.center()); lod->setRadius(localBox.radius());

        std::vector<unsigned char> data; float d = localBox.radius() * 0.4f;
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
                osg::ref_ptr<osg::Geode> child = new osg::Geode; child->addDrawable(geom.get());
                geom->setName(lod->getName() + "_LOD" + std::to_string(i));

                if (deg > 0)
                {
                    std::vector<unsigned char> data2;
                    start = chunk.offsets[i] * 2, end = chunk.byteSizes[i] * 2 + start;
                    if (totalSize2 <= end) data2.assign(ro_mmap2.begin() + start, ro_mmap2.end());
                    else data2.assign(ro_mmap2.begin() + start, ro_mmap2.begin() + end);
                    applyShcoefFromXGrids(geom.get(), data2, chunk.numSplats[i], shRange[0], shRange[1]);
                }
                static_cast<osgVerse::GaussianGeometry*>(geom.get())->finalize();
                lod->addChild(child.get(), d * pow(1.2f, i), d * pow(1.2f, i + 1));
            }
        }

        const osg::LOD::RangeList& ranges = lod->getRangeList();
        lod->setRange(0, ranges[0].first, FLT_MAX);
        lod->setRange(ranges.size() - 1, 0.0f, ranges[ranges.size() - 1].second);
        root->addChild(lod.get());
    }
    ro_mmap.unmap();
    if (deg > 0) ro_mmap2.unmap();

    // Load collision.lci
    std::string collisionPath = path + "/collision.lci";
    if (!osgDB::fileExists(collisionPath)) collisionPath = path + "/Collision.lci";
    if (osgDB::fileExists(collisionPath))
    {
        std::ifstream lciIn(collisionPath.c_str(), std::ios::in | std::ios::binary);
        if (!lciIn) return root;

        osg::ref_ptr<osg::Node> collision = loadCollisionFromXGrids(lciIn);
        if (collision.valid())
        {
            collision->setNodeMask(0);  // not renderable, only for manipulating uses
            collision->setUserValue("Collision", true);
            root->addChild(collision.get());
        }
    }
    return root;
}
