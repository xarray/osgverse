#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>
#include "modeling/GaussianGeometry.h"
#include "modeling/Utilities.h"
#include "spz/load-spz.h"

// Ref: https://github.com/playcanvas/splat-transform/blob/main/src/readers/
osg::ref_ptr<osg::Node> loadSplatFromXGrids(std::istream& in, const std::string& path,
                                            osgVerse::GaussianGeometry::RenderMethod method);
osg::ref_ptr<osg::Node> loadSplatFromSOG(std::istream& in, const std::string& path, const std::string& ext,
                                         osgVerse::GaussianGeometry::RenderMethod method);

namespace
{
    static float halfToFloat(uint16_t h)
    {   // Simple half-float to float conversion
        uint32_t sign = (h >> 15) & 0x1;
        uint32_t exp = (h >> 10) & 0x1F;
        uint32_t mant = h & 0x3FF;
        if (exp == 0)
        {
            if (mant == 0) return sign ? -0.0f : 0.0f;
            float val = mant / 1024.0f * powf(2.0f, -14);  // Denormalized
            return sign ? -val : val;
        }
        else if (exp == 31)
        {
            if (mant == 0) return sign ? -INFINITY : INFINITY;
            return NAN;
        }

        // Normalized
        float val = (1.0f + mant / 1024.0f) * powf(2.0f, (int)exp - 15);
        return sign ? -val : val;
    }
}

class ReaderWriter3DGS : public osgDB::ReaderWriter
{
public:
    ReaderWriter3DGS()
    {
        supportsExtension("verse_3dgs", "osgVerse pseudo-loader");
        supportsExtension("ply", "PLY point cloud file");
        supportsExtension("splat", "Gaussian splat data file");
        supportsExtension("ksplat", "Mark Kellogg's splat file");
        supportsExtension("spz", "Niantic Labs' splat file");
        supportsExtension("lcc", "XGrids' splat file");
        supportsExtension("json", "PlayCanvas SOG's meta.json file");
        supportsExtension("sog", "PlayCanvas SOG's ZIP file");
        supportsOption("RenderMethod=<hint>", "Rendering method of 3D gaussian data:\n"
                       "<SSBO> render with draw-instanced and SSBO;\n"
                       "<TBO> render with draw-instanced and TBO;\n"
                       "<TEX2D> render with draw-instanced and 2D textures (very low FPS);\n"
                       "<GS> render with geometry shader.");
    }

    virtual const char* className() const
    {
        return "[osgVerse] 3D Gaussian Scattering data format reader / writer";
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_FOUND;

        osg::ref_ptr<Options> localOptions = NULL;
        if (options) localOptions = options->cloneOptions();
        else localOptions = new osgDB::Options();

        localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
        localOptions->setPluginStringData("extension", ext);
        return readNode(in, localOptions.get());
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        spz::UnpackOptions unpackOpt;  // TODO: convert coordinates
        if (options)
        {
            std::string renderHint = options->getPluginStringData("RenderMethod");
            osgVerse::GaussianGeometry::RenderMethod method = osgVerse::GaussianGeometry::INSTANCING;
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            method = osgVerse::GaussianGeometry::INSTANCING_TEX2D;
#endif
            if (renderHint == "TBO") method = osgVerse::GaussianGeometry::INSTANCING_TEXTURE;
            if (renderHint == "TEX2D") method = osgVerse::GaussianGeometry::INSTANCING_TEX2D;
            else if (renderHint == "GS") method = osgVerse::GaussianGeometry::GEOMETRY_SHADER;

            std::string prefix = options->getPluginStringData("prefix");
            std::string ext = options->getPluginStringData("extension");
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == "lcc")
            {
                osg::ref_ptr<osg::Node> node = loadSplatFromXGrids(fin, prefix, method);
                if (node.valid()) return node.get();
            }
            else if (ext == "json" || ext == "sog")
            {
                osg::ref_ptr<osg::Node> node = loadSplatFromSOG(fin, prefix, ext, method);
                if (node.valid()) return node.get();
            }

            spz::GaussianCloud cloud;
            if (ext == "ply")
            {
                cloud = spz::loadSplatFromPly(fin, prefix, unpackOpt);
                if (cloud.numPoints > 0) geode->addDrawable(fromSpz(cloud, method));
            }
            else
            {
                std::string buffer((std::istreambuf_iterator<char>(fin)),
                                   std::istreambuf_iterator<char>());
                if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

                if (ext == "spz")
                {
                    std::vector<uint8_t> dataSrc(buffer.begin(), buffer.end());
                    cloud = spz::loadSpz(dataSrc, unpackOpt);
                    if (cloud.numPoints > 0) geode->addDrawable(fromSpz(cloud, method));
                }
                else if (ext == "splat")
                {
                    osgVerse::GaussianGeometry* geom = fromSplat(buffer, method);
                    if (geom) geode->addDrawable(geom);
                }
                else if (ext == "ksplat")
                {
                    osgVerse::GaussianGeometry* geom = fromKSplat(buffer, method);
                    if (geom) geode->addDrawable(geom);
                }
            }
        }

        if (geode->getNumDrawables() > 0) return geode.get();
        else return ReadResult::FILE_NOT_HANDLED;
    }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& path, const osgDB::Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return WriteResult::FILE_NOT_HANDLED;

        if (ext == "ply")
        {
            if (spz::saveSplatToPly(sceneToSpz(node), spz::PackOptions(), fileName)) return WriteResult::FILE_SAVED;
            else WriteResult::ERROR_IN_WRITING_FILE;
        }
        else
        {
            std::ofstream out(fileName, std::ios::out | std::ios::binary);
            if (!out) return WriteResult::FILE_NOT_HANDLED;

            osg::ref_ptr<Options> localOptions = NULL;
            if (options) localOptions = options->cloneOptions();
            else localOptions = new osgDB::Options();

            localOptions->setPluginStringData("prefix", osgDB::getFilePath(path));
            localOptions->setPluginStringData("extension", ext);
            return writeNode(node, out, localOptions.get());
        }
        return WriteResult::FILE_NOT_HANDLED;
    }

    virtual WriteResult writeNode(const osg::Node& node, std::ostream& fout, const osgDB::Options* options) const
    {
        std::string ext = ""; bool success = false;
        if (options) { ext = options->getPluginStringData("extension"); }

        std::vector<unsigned char> resultData;
        if (ext == "spz") success = saveSpz(sceneToSpz(node), spz::PackOptions(), &resultData);
        // TODO: more extensions to support

        if (!resultData.empty()) fout.write((char*)resultData.data(), resultData.size());
        return success ? WriteResult::FILE_SAVED : WriteResult::ERROR_IN_WRITING_FILE;
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_3dgs");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }

    osgVerse::GaussianGeometry* fromSplat(const std::string& buffer, osgVerse::GaussianGeometry::RenderMethod m) const
    {
        osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array, scale = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array; osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray;
        osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array, gD0 = new osg::Vec4Array, bD0 = new osg::Vec4Array;

        std::stringstream ss(buffer, std::ios::in | std::ios::out | std::ios::binary);
        osg::Vec3 posValue, scaleValue; osg::Vec4 rotValue;
        const static float kSH_C0 = 0.28209479177387814;
        while (ss.good() && !ss.eof())
        {
            uint8_t rgba[4], rotation[4];
            ss.read((char*)posValue.ptr(), sizeof(float) * 3);
            ss.read((char*)scaleValue.ptr(), sizeof(float) * 3);
            ss.read((char*)rgba, sizeof(uint8_t) * 4);
            ss.read((char*)rotation, sizeof(uint8_t) * 4);

            for (int i = 0; i < 4; ++i) rotValue[i] = (rotation[i] / 255.0f) * 2.0f - 1.0f;
            rotValue.normalize(); rot->push_back(osg::Vec4(rotValue[1], rotValue[2], rotValue[3], rotValue[0]));
            pos->push_back(posValue); scale->push_back(scaleValue); alpha->push_back(rgba[3] / 255.0f);
            rD0->push_back(osg::Vec4((rgba[0] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
            gD0->push_back(osg::Vec4((rgba[1] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
            bD0->push_back(osg::Vec4((rgba[2] / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
        }
        if (pos->empty()) return NULL;

        osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry(m);
        geom->setShDegrees(0); geom->setPosition(pos.get());
        geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
        geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
        geom->finalize(); return geom.release();
    }

    osgVerse::GaussianGeometry* fromKSplat(const std::string& buffer, osgVerse::GaussianGeometry::RenderMethod m) const
    {
        static const uint32_t KSPLAT_HEADER_SIZE = 4096, KSPLAT_SECTION_HEADER_SIZE = 1024;
        static const uint32_t KSPLAT_BUCKET_STORAGE_SIZE = 12, KSPLAT_BUCKET_SIZE = 256;
        if (buffer.size() < KSPLAT_HEADER_SIZE)
            { OSG_WARN << "[ReaderWriter3DGS] KSplat buffer too small for header" << std::endl; return NULL; }

        // Parse header fields (4096 bytes)
        const uint8_t* data = (const uint8_t*)buffer.data(); uint32_t offset = 0;
        uint8_t versionMajor = data[offset++];
        uint8_t versionMinor = data[offset++]; offset += 2; // Skip unused bytes
        uint32_t maxSectionCount = *(uint32_t*)(data + 4);
        uint32_t sectionCount = *(uint32_t*)(data + 8);
        uint32_t maxSplatCount = *(uint32_t*)(data + 12);
        uint32_t splatCount = *(uint32_t*)(data + 16);
        uint16_t compressionLevel = *(uint16_t*)(data + 20);
        osg::Vec3 sceneCenter(*(float*)(data + 24), *(float*)(data + 28), *(float*)(data + 32));  // Scene center
        float minSHCoeff = *(float*)(data + 36), maxSHCoeff = *(float*)(data + 40);  // SH compression range
        if (minSHCoeff == 0.0f) minSHCoeff = -1.5f; if (maxSHCoeff == 0.0f) maxSHCoeff = 1.5f;

        // Calculate bytes per component based on compression level
        uint32_t bytesPerCenter = 0, bytesPerScale = 0, bytesPerRotation = 0, bytesPerColor = 0, bytesPerSH = 0;
        switch (compressionLevel)
        {
        case 0: // No compression
            bytesPerCenter = 12; bytesPerScale = 12; bytesPerRotation = 16;
            bytesPerColor = 4; bytesPerSH = 4; break;
        case 1: // 16-bit compression
            bytesPerCenter = 6; bytesPerScale = 6; bytesPerRotation = 8;
            bytesPerColor = 4; bytesPerSH = 2; break;
        case 2: // 8-bit SH compression
            bytesPerCenter = 6; bytesPerScale = 6; bytesPerRotation = 8;
            bytesPerColor = 4; bytesPerSH = 1; break;
        default:
            OSG_WARN << "[ReaderWriter3DGS] Unsupported KSplat compression level: " << compressionLevel << std::endl;
            return NULL;
        }

        // Prepare output arrays
        osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array, scale = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array; osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray;
        osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array, gD0 = new osg::Vec4Array, bD0 = new osg::Vec4Array;
        const static float kSH_C0 = 0.28209479177387814, scaleRange = 32767.0f;

        // Parse sections
        uint32_t currentSectionOffset = KSPLAT_HEADER_SIZE + maxSectionCount * KSPLAT_SECTION_HEADER_SIZE;
        uint32_t totalSplatsRead = 0, maxShDegree = 0; offset = KSPLAT_HEADER_SIZE;
        for (uint32_t s = 0; s < maxSectionCount && totalSplatsRead < splatCount; s++)
        {
            // Parse section header (1024 bytes)
            if (offset + KSPLAT_SECTION_HEADER_SIZE > buffer.size()) break;
            const uint8_t* secData = data + offset;
            uint32_t secMaxSplatCount = *(uint32_t*)(secData + 4);
            uint32_t bucketSize = *(uint32_t*)(secData + 8);
            uint32_t bucketCount = *(uint32_t*)(secData + 12);
            float bucketBlockSize = *(float*)(secData + 16);
            uint16_t bucketStorageSizeBytes = *(uint16_t*)(secData + 20);
            uint32_t compressionScaleRange = *(uint32_t*)(secData + 24);
            uint32_t fullBucketCount = *(uint32_t*)(secData + 32);
            uint32_t partiallyFilledBucketCount = *(uint32_t*)(secData + 36);
            uint16_t sphericalHarmonicsDegree = *(uint16_t*)(secData + 40);
            if (compressionScaleRange <= 0) compressionScaleRange = scaleRange;
            if (sphericalHarmonicsDegree > maxShDegree) maxShDegree = sphericalHarmonicsDegree;

            // Calculate bytes per splat for this section
            uint32_t shComponentCount = 0;
            if (sphericalHarmonicsDegree == 1) shComponentCount = 9;  // 3 * (4-1)
            else if (sphericalHarmonicsDegree == 2) shComponentCount = 24; // 3 * (9-1)
            else if (sphericalHarmonicsDegree == 3) shComponentCount = 45; // 3 * (16-1)

            uint32_t bytesPerSplat = bytesPerCenter + bytesPerScale + bytesPerRotation + bytesPerColor;
            if (sphericalHarmonicsDegree > 0) bytesPerSplat += shComponentCount * bytesPerSH;

            // Calculate section data offset
            uint32_t bucketsMetaDataSize = partiallyFilledBucketCount * 4;
            uint32_t bucketsStorageSize = bucketStorageSizeBytes * bucketCount + bucketsMetaDataSize;
            uint32_t sectionDataOffset = offset + KSPLAT_SECTION_HEADER_SIZE + bucketsStorageSize;
            const osg::Vec3* subCenters = (const osg::Vec3*)(data + currentSectionOffset + bucketsMetaDataSize);
            const uint32_t* partialBucketSizes = (const uint32_t*)(data + currentSectionOffset);

            // Read splats in this section
            uint32_t splatsInSection = osg::minimum(secMaxSplatCount, splatCount - totalSplatsRead);
            uint32_t fullBucketSplats = fullBucketCount * bucketSize;
            uint32_t currentPartialBucket = fullBucketCount, currentPartialBase = fullBucketSplats;
            float positionScale = bucketBlockSize * 0.5f / compressionScaleRange;
            for (uint32_t i = 0; i < splatsInSection; i++)
            {
                uint32_t splatOffset = sectionDataOffset + i * bytesPerSplat;
                if (splatOffset + bytesPerSplat > buffer.size()) break;
                const uint8_t* splatData = data + splatOffset;
                osg::Vec3 pv, sv; osg::Vec4 rotation;

                // Get bucket center
                uint32_t bucketIdx = 0, so = 0;
                if (i < fullBucketSplats)
                    bucketIdx = floor(i / bucketSize);
                else
                {
                    uint32_t currentBucketSize = partialBucketSizes[currentPartialBucket - fullBucketCount];
                    if (i >= currentPartialBase + currentBucketSize)
                        { currentPartialBucket++; currentPartialBase += currentBucketSize; }
                    bucketIdx = currentPartialBucket;
                }

                // Read position (quantized, needs dequantization)
                if (compressionLevel == 0)
                {
                    pv[0] = *(float*)(splatData + so);
                    pv[1] = *(float*)(splatData + so + 4);
                    pv[2] = *(float*)(splatData + so + 8); so += 12;
                }
                else
                {   // 16-bit quantized positions
                    osg::Vec3 offset = subCenters[bucketIdx] - osg::Vec3(1.0f, 1.0f, 1.0f) * (bucketBlockSize * 0.5f);
                    pv[0] = offset[0] + (*(uint16_t*)(splatData + so)) * positionScale;
                    pv[1] = offset[1] + (*(uint16_t*)(splatData + so + 2)) * positionScale;
                    pv[2] = offset[2] + (*(uint16_t*)(splatData + so + 4)) * positionScale; so += 6;
                }
                pos->push_back(pv);

                // Read scale
                if (compressionLevel == 0)
                {
                    sv[0] = (*(float*)(splatData + so));
                    sv[1] = (*(float*)(splatData + so + 4));
                    sv[2] = (*(float*)(splatData + so + 8)); so += 12;
                }
                else
                {   // 16-bit half-float scales
                    sv[0] = (halfToFloat(*(uint16_t*)(splatData + so)));
                    sv[1] = (halfToFloat(*(uint16_t*)(splatData + so + 2)));
                    sv[2] = (halfToFloat(*(uint16_t*)(splatData + so + 4))); so += 6;
                }
                scale->push_back(sv);

                // Read rotation (quaternion)
                if (compressionLevel == 0)
                {
                    rotation[3] = *(float*)(splatData + so);
                    rotation[0] = *(float*)(splatData + so + 4);
                    rotation[1] = *(float*)(splatData + so + 8);
                    rotation[2] = *(float*)(splatData + so + 12); so += 16;
                }
                else
                {   // 16-bit half-float rotations
                    rotation[3] = halfToFloat(*(uint16_t*)(splatData + so));
                    rotation[0] = halfToFloat(*(uint16_t*)(splatData + so + 2));
                    rotation[1] = halfToFloat(*(uint16_t*)(splatData + so + 4));
                    rotation[2] = halfToFloat(*(uint16_t*)(splatData + so + 6)); so += 8;
                }
                rotation.normalize(); rot->push_back(rotation);

                // Read color (RGBA, 4 bytes)
                uint8_t r = splatData[so], g = splatData[so + 1];
                uint8_t b = splatData[so + 2], a = splatData[so + 3]; so += 4;

                // Convert color to SH DC coefficients
                rD0->push_back(osg::Vec4((r / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
                gD0->push_back(osg::Vec4((g / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
                bD0->push_back(osg::Vec4((b / 255.0f - 0.5f) / kSH_C0, 0.0f, 0.0f, 0.0f));
                alpha->push_back(a / 255.0f);

                // FIXME: skip SH data if present (for now, we only support degree 0)
                if (sphericalHarmonicsDegree > 0) so += shComponentCount * bytesPerSH;
            }

            totalSplatsRead += splatsInSection; offset += KSPLAT_SECTION_HEADER_SIZE;
            offset += bucketsStorageSize + secMaxSplatCount * bytesPerSplat;  // Skip to next section
        }

        osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry(m);
        geom->setShDegrees(0); // Currently only support degree 0
        geom->setPosition(pos.get()); geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
        geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
        geom->finalize(); return geom.release();
    }

    osgVerse::GaussianGeometry* fromSpz(spz::GaussianCloud& c, osgVerse::GaussianGeometry::RenderMethod m) const
    {
        osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array, scale = new osg::Vec3Array;
        for (size_t i = 0; i < c.positions.size(); i += 3)
            pos->push_back(osg::Vec3(c.positions[i], c.positions[i + 1], c.positions[i + 2]));
        for (size_t i = 0; i < c.scales.size(); i += 3)
        {
            // scale is stored in logarithmic scale in plyFile
            scale->push_back(osg::Vec3(expf(c.scales[i]), expf(c.scales[i + 1]), expf(c.scales[i + 2])));
        }

        osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array;
        for (size_t i = 0; i < c.rotations.size(); i += 4)
            rot->push_back(osg::Vec4(c.rotations[i], c.rotations[i + 1], c.rotations[i + 2], c.rotations[i + 3]));

        osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray;
        for (size_t i = 0; i < c.alphas.size(); i++) { float op = c.alphas[i]; alpha->push_back(1.0f / (1.0f + expf(-op))); }

        osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array, gD0 = new osg::Vec4Array, bD0 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD1 = new osg::Vec4Array, gD1 = new osg::Vec4Array, bD1 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD2 = new osg::Vec4Array, gD2 = new osg::Vec4Array, bD2 = new osg::Vec4Array;
        osg::ref_ptr<osg::Vec4Array> rD3 = new osg::Vec4Array, gD3 = new osg::Vec4Array, bD3 = new osg::Vec4Array;
        size_t numShCoff = c.sh.size() / c.numPoints, shIndex = 0;
        for (size_t i = 0; i < c.colors.size(); i += 3, shIndex += numShCoff)
        {
            rD0->push_back(osg::Vec4(c.colors[i + 0], 0.0f, 0.0f, 0.0f));
            gD0->push_back(osg::Vec4(c.colors[i + 1], 0.0f, 0.0f, 0.0f));
            bD0->push_back(osg::Vec4(c.colors[i + 2], 0.0f, 0.0f, 0.0f));
            if (numShCoff >= 9)  // Degree 1
            {
                rD0->back().set(rD0->back()[0], c.sh[shIndex + 0], c.sh[shIndex + 3], c.sh[shIndex + 6]);
                gD0->back().set(gD0->back()[0], c.sh[shIndex + 1], c.sh[shIndex + 4], c.sh[shIndex + 7]);
                bD0->back().set(bD0->back()[0], c.sh[shIndex + 2], c.sh[shIndex + 5], c.sh[shIndex + 8]);
            }

            if (numShCoff >= 24)  // Degree 2
            {
                rD1->push_back(osg::Vec4(c.sh[shIndex + 9], c.sh[shIndex + 12], c.sh[shIndex + 15], c.sh[shIndex + 18]));
                gD1->push_back(osg::Vec4(c.sh[shIndex + 10], c.sh[shIndex + 13], c.sh[shIndex + 16], c.sh[shIndex + 19]));
                bD1->push_back(osg::Vec4(c.sh[shIndex + 11], c.sh[shIndex + 14], c.sh[shIndex + 17], c.sh[shIndex + 20]));
                rD2->push_back(osg::Vec4(c.sh[shIndex + 21], 0.0f, 0.0f, 0.0f));
                gD2->push_back(osg::Vec4(c.sh[shIndex + 22], 0.0f, 0.0f, 0.0f));
                bD2->push_back(osg::Vec4(c.sh[shIndex + 23], 0.0f, 0.0f, 0.0f));
            }

            if (numShCoff >= 45)  // Degree 3
            {
                rD2->back().set(rD2->back()[0], c.sh[shIndex + 24], c.sh[shIndex + 27], c.sh[shIndex + 30]);
                gD2->back().set(gD2->back()[0], c.sh[shIndex + 25], c.sh[shIndex + 28], c.sh[shIndex + 31]);
                bD2->back().set(bD2->back()[0], c.sh[shIndex + 26], c.sh[shIndex + 29], c.sh[shIndex + 32]);
                rD3->push_back(osg::Vec4(c.sh[shIndex + 33], c.sh[shIndex + 36], c.sh[shIndex + 39], c.sh[shIndex + 42]));
                gD3->push_back(osg::Vec4(c.sh[shIndex + 34], c.sh[shIndex + 37], c.sh[shIndex + 40], c.sh[shIndex + 43]));
                bD3->push_back(osg::Vec4(c.sh[shIndex + 35], c.sh[shIndex + 38], c.sh[shIndex + 41], c.sh[shIndex + 44]));
            }
        }

        osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry(m);
        geom->setShDegrees(c.shDegree); geom->setPosition(pos.get());
        geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
        geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get());
        if (numShCoff >= 24)
        {
            geom->setShRed(1, rD1.get()); geom->setShGreen(1, gD1.get()); geom->setShBlue(1, bD1.get());
            geom->setShRed(2, rD2.get()); geom->setShGreen(2, gD2.get()); geom->setShBlue(2, bD2.get());
            if (numShCoff >= 45)
                { geom->setShRed(3, rD3.get()); geom->setShGreen(3, gD3.get()); geom->setShBlue(3, bD3.get()); }
        }
        geom->finalize(); return geom.release();
    }

    spz::GaussianCloud sceneToSpz(const osg::Node& node) const
    {
        spz::GaussianCloud cloud; osgVerse::FindGeometryVisitor fgv(true);
        osg::Node& nonconst = const_cast<osg::Node&>(node); nonconst.accept(fgv);

        const std::vector<std::pair<osg::Geometry*, osg::Matrix>>& geomList = fgv.getGeometries();
        for (size_t i = 0; i < geomList.size(); ++i)
        {
            osgVerse::GaussianGeometry* geom =
                dynamic_cast<osgVerse::GaussianGeometry*>(geomList[i].first);
            if (!geom) continue;  // FIXME: apply matrix?

            int numSplats = geom->getNumSplats(); if (!numSplats) continue;
            float *pos3 = (float*)geom->getPosition3(), *pos4 = (float*)geom->getPosition4();
            if (pos4)
            {
                size_t curr = cloud.positions.size(); cloud.positions.resize(curr + numSplats * 3);
#pragma omp parallel for
                for (int i = 0; i < numSplats; ++i)
                { for (int k = 0; k < 3; ++k) cloud.positions[curr + i * 3 + k] = *(pos4 + i * 4 + k); }
            }
            else if (pos3)
                cloud.positions.insert(cloud.positions.end(), pos3, pos3 + numSplats * 3);

            osg::ref_ptr<osg::Vec3Array> scale = geom->getScale();
            if (scale.valid() && scale->size() == numSplats)
            {
                float* ptr = (float*)scale->getDataPointer();
                size_t curr = cloud.scales.size(); cloud.scales.resize(curr + numSplats * 3);
#pragma omp parallel for
                for (int i = 0; i < numSplats; ++i)
                { for (int k = 0; k < 3; ++k) cloud.scales[curr + i * 3 + k] = logf(*(ptr + i * 3 + k)); }
            }

            osg::ref_ptr<osg::Vec4Array> rot = geom->getRotation();
            if (rot.valid() && rot->size() == numSplats)
            {
                float* ptr = (float*)rot->getDataPointer();
                cloud.rotations.insert(cloud.rotations.end(), ptr, ptr + numSplats * 4);
            }

            osg::ref_ptr<osg::FloatArray> alpha = geom->getAlpha();
            if (alpha.valid() && alpha->size() == numSplats)
            {
                float* ptr = (float*)alpha->getDataPointer();
                size_t curr = cloud.alphas.size(); cloud.alphas.resize(curr + numSplats);
#pragma omp parallel for
                for (int i = 0; i < numSplats; ++i)
                    cloud.alphas[curr + i] = logf(ptr[i] / (1.0f - ptr[i]));
            }

            osg::ref_ptr<osg::Vec4Array> r = geom->getShRed(0);
            osg::ref_ptr<osg::Vec4Array> g = geom->getShGreen(0);
            osg::ref_ptr<osg::Vec4Array> b = geom->getShBlue(0);
            if (r.valid() && g.valid() && b.valid())
            {
                size_t curr = cloud.colors.size(); cloud.colors.resize(curr + numSplats * 3);
#pragma omp parallel for
                for (int i = 0; i < numSplats; ++i)
                {
                    cloud.colors[curr + i * 3 + 0] = (*r)[i].x();
                    cloud.colors[curr + i * 3 + 1] = (*g)[i].x();
                    cloud.colors[curr + i * 3 + 2] = (*b)[i].x();
                }
            }
            // FIXME: more sh-coeffs?
        }

        size_t numSh = cloud.sh.size() / cloud.positions.size();
        cloud.numPoints = cloud.positions.size() / 3;
        cloud.shDegree = (numSh == 15) ? 3 : (numSh == 8 ? 2 : (numSh == 3 ? 1 : 0));
        return cloud;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_3dgs, ReaderWriter3DGS)
