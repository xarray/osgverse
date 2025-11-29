#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>

#include "modeling/GaussianGeometry.h"
#include "readerwriter/Utilities.h"
#include "3rdparty/picojson.h"
#include "3rdparty/mio.hpp"

struct XGridsNodeChunk
{
    uint32_t id;
    std::vector<uint32_t> numSplats, byteSizes;
    std::vector<uint64_t> offsets;
};

osg::Node* loadGeometryFromXGrids(const std::vector<unsigned char>& data, uint32_t numSplats)
{
    return NULL;  // TODO
}

osg::Node* loadSplatFromXGrids(std::istream& in, const std::string& path)
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

    picojson::array& ofJson = document.get("offset").get<picojson::array>();
    picojson::array& shJson = document.get("shift").get<picojson::array>();
    picojson::array& scJson = document.get("scale").get<picojson::array>();
    picojson::array& lodJson = document.get("splats").get<picojson::array>();
    osg::Vec3d gOffset, gShift, gScale; std::vector<uint32_t> lodNumbers;
    if (ofJson.size() > 2) gOffset.set(ofJson[0].get<double>(), ofJson[1].get<double>(), ofJson[2].get<double>());
    if (shJson.size() > 2) gShift.set(shJson[0].get<double>(), shJson[1].get<double>(), shJson[2].get<double>());
    if (scJson.size() > 2) gScale.set(scJson[0].get<double>(), scJson[1].get<double>(), scJson[2].get<double>());
    for (size_t i = 0; i < lodJson.size(); ++i) lodNumbers.push_back((uint32_t)lodJson[i].get<double>());

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName(name); root->addDescription(description);
    root->setMatrix(osg::Matrix::scale(gScale) * osg::Matrix::translate(gOffset));

    // Read information from index.bin
    std::vector<unsigned char> indexData = osgVerse::loadFileData(path + "/index.bin", reserved0, reserved1);
    if (indexData.empty()) indexData = osgVerse::loadFileData(path + "/Index.bin", reserved0, reserved1);
    if (indexData.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to load XGrids' index.bin at " << path << std::endl;
        return root.release();
    }

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss.write((char*)indexData.data(), indexData.size());

    std::vector<XGridsNodeChunk> dataChunks(indexDataSize);
    for (size_t i = 0; i < indexDataSize; ++i)
    {
        XGridsNodeChunk& chunk = dataChunks[i];
        ss.read((char*)&chunk.id, sizeof(uint32_t));
        for (size_t j = 0; j < lodNumbers.size(); ++j)
        {
            uint32_t v0 = 0; uint64_t v1 = 0;
            ss.read((char*)&v0, sizeof(uint32_t)); chunk.numSplats.push_back(v0);
            ss.read((char*)&v1, sizeof(uint64_t)); chunk.offsets.push_back(v1);
            ss.read((char*)&v0, sizeof(uint32_t)); chunk.byteSizes.push_back(v0);
        }
    }

    // Read information from data.bin and create LODs
    std::string dataBinPath = path + "/data.bin";
    if (!osgDB::fileExists(dataBinPath)) dataBinPath = path + "/Data.bin";
    if (!osgDB::fileExists(dataBinPath))
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to load XGrids' data.bin at " << path << std::endl;
        return root.release();
    }

    std::error_code error; mio::mmap_source ro_mmap;
    ro_mmap.map(dataBinPath, error);
    if (error)
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to map XGrids' data.bin: " << error << std::endl;
        return root.release();
    }

    uint64_t totalSize = ro_mmap.size();
    for (size_t i = 0; i < indexDataSize; ++i)
    {
        XGridsNodeChunk& chunk = dataChunks[i];
        osg::ref_ptr<osg::LOD> lod = new osg::LOD;
        lod->setName(std::to_string(chunk.id));
        lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);

        std::vector<unsigned char> data;
        for (int i = (int)chunk.offsets.size() - 1; i >= 0; --i)
        {
            uint64_t start = chunk.offsets[i], end = chunk.byteSizes[i]; end += start;
            if (totalSize <= end) data.assign(ro_mmap.begin() + start, ro_mmap.end());
            else data.assign(ro_mmap.begin() + start, ro_mmap.begin() + end);
            
            osg::ref_ptr<osg::Node> child = loadGeometryFromXGrids(data, chunk.numSplats[i]);
            if (child.valid())
            {
                osg::BoundingSphere bs = child->getBound(); float d = bs.radius() * 1.2f;
                lod->addChild(child.get(), d * i, d * (i + 1));  // FIXME: better range?
            }
        }
        root->addChild(lod.get());
    }
    ro_mmap.unmap();
    return root.release();
}
