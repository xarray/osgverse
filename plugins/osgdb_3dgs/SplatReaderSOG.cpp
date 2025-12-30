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

// https://developer.playcanvas.com/user-manual/gaussian-splatting/formats/sog/
namespace
{
    struct SogData
    {
        std::map<std::string, osg::ref_ptr<osg::Image>> images;
        std::vector<float> scaleCode, sh0Code, shNCode;
        osg::Vec3 meansMin, meansMax;
        size_t numDegrees, numEntries;
        double count, version;
        SogData() : numDegrees(0), numEntries(0), count(0), version(0) {}
    };

    static float signum(float x) { return float((x > 0.0f) - (x < 0.0f)); }
    static float unlog(float x) { return signum(x) * (exp(abs(x)) - 1.0f); }
    static float sigmoidInv(float x) { float e = osg::minimum(1.0 - 1e-6, osg::maximum(1e-6, (double)x)); return (float)log(e / (1.0 - e)); };

    static osg::Image* readDataImage(const std::string& path, const std::string& file, osg::Referenced* zip)
    {
        if (zip != NULL)
        {
            std::vector<unsigned char> data = osgVerse::CompressAuxiliary::extract(zip, file);
            if (data.empty()) return NULL;

            std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); ss.write((char*)data.data(), data.size());
            osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(osgDB::getFileExtension(file));
            if (!rw) rw = osgDB::Registry::instance()->getReaderWriterForExtension("verse_webp");
            if (!rw) return NULL; else return rw->readImage(ss).takeImage();
        }
        else
        {
            osg::Image* img = osgDB::readImageFile(path + "/" + file + ".verse_webp");
            if (!img) img = osgDB::readImageFile(path + "/" + file); return img;
        }
    }

    static bool parseSogMetaData(SogData& sogData, picojson::value& document, const std::string& path, osg::Referenced* zip)
    {
        if (zip != NULL)
        {
            std::vector<unsigned char> json = osgVerse::CompressAuxiliary::extract(zip, "meta.json");
            std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); ss.write((char*)json.data(), json.size());
            std::string err = picojson::parse(document, ss);
            if (!err.empty()) { OSG_WARN << "[ReaderWriter3DGS] Failed to parse PlayCanvas' SOG data: " << err << std::endl; return false; }
        }

        // Read information from meta.json
        picojson::value versionObj = document.get("version");
        picojson::value countObj = document.get("count");
        picojson::value meansObj = document.get("means");
        picojson::value scalesObj = document.get("scales");
        picojson::value quatsObj = document.get("quats");
        picojson::value sh0Obj = document.get("sh0");
        picojson::value shNObj = document.get("shN");
        sogData.version = versionObj.is<double>() ? versionObj.get<double>() : 1;
        sogData.count = countObj.is<double>() ? countObj.get<double>() : 0;
        if (sogData.version < 2) { OSG_NOTICE << "[ReaderWriter3DGS] SOG version 1 is not supported\n"; return false; }

        if (meansObj.is<picojson::object>())
        {
            picojson::array mins = meansObj.get("mins").get<picojson::array>();
            picojson::array maxs = meansObj.get("maxs").get<picojson::array>();
            if (mins.size() > 2 && maxs.size() > 2)
            {
                for (size_t i = 0; i < 3; ++i)
                { sogData.meansMin[i] = mins[i].get<double>(); sogData.meansMax[i] = maxs[i].get<double>(); }
            }

            picojson::array files = meansObj.get("files").get<picojson::array>();
            for (size_t i = 0; i < files.size(); ++i)
                sogData.images["means_" + std::to_string(i)] = readDataImage(path, files[i].get<std::string>(), zip);
        }

        if (quatsObj.is<picojson::object>())
        {
            picojson::array files = quatsObj.get("files").get<picojson::array>();
            for (size_t i = 0; i < files.size(); ++i)
                sogData.images["quats_" + std::to_string(i)] = readDataImage(path, files[i].get<std::string>(), zip);
        }

        if (scalesObj.is<picojson::object>())
        {
            picojson::array codebook = scalesObj.get("codebook").get<picojson::array>();
            for (size_t i = 0; i < codebook.size(); ++i) sogData.scaleCode.push_back(codebook[i].get<double>());

            picojson::array files = scalesObj.get("files").get<picojson::array>();
            for (size_t i = 0; i < files.size(); ++i)
                sogData.images["scales_" + std::to_string(i)] = readDataImage(path, files[i].get<std::string>(), zip);
        }

        if (sh0Obj.is<picojson::object>())
        {
            picojson::array codebook = sh0Obj.get("codebook").get<picojson::array>();
            for (size_t i = 0; i < codebook.size(); ++i) sogData.sh0Code.push_back(codebook[i].get<double>());

            picojson::array files = sh0Obj.get("files").get<picojson::array>();
            for (size_t i = 0; i < files.size(); ++i)
                sogData.images["sh0_" + std::to_string(i)] = readDataImage(path, files[i].get<std::string>(), zip);
        }

        if (shNObj.is<picojson::object>())
        {
            picojson::array codebook = shNObj.get("codebook").get<picojson::array>();
            for (size_t i = 0; i < codebook.size(); ++i) sogData.shNCode.push_back(codebook[i].get<double>());

            picojson::array files = shNObj.get("files").get<picojson::array>();
            for (size_t i = 0; i < files.size(); ++i)
                sogData.images["shN_" + std::to_string(i)] = readDataImage(path, files[i].get<std::string>(), zip);
            sogData.numDegrees = (size_t)shNObj.get("bands").get<double>();
            sogData.numEntries = (size_t)shNObj.get("count").get<double>();
        }
        return true;
    }

    static void createSogPositions(osg::Vec3Array& va, osg::Image* means_l, osg::Image* means_u,
                                   const osg::Vec3& mins, const osg::Vec3& maxs)
    {
        if (!means_l || !means_u) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'means' image missing\n"; return; }
        if (means_l->getDataType() != GL_UNSIGNED_BYTE || means_l->getPixelFormat() != GL_RGBA ||
            means_u->getDataType() != GL_UNSIGNED_BYTE || means_u->getPixelFormat() != GL_RGBA)
        { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'means' image format mismatch\n"; return; }
        
        osg::Vec4ub *ptrL = (osg::Vec4ub*)means_l->data(), *ptrU = (osg::Vec4ub*)means_u->data();
        for (size_t i = 0; i < va.size(); ++i)
        {
            osg::Vec4ub valueL = *(ptrL + i), valueU = *(ptrU + i);
            float qx = ((unsigned short)(valueU.r() << 8) | valueL.r()) / 65535.0f;
            float qy = ((unsigned short)(valueU.g() << 8) | valueL.g()) / 65535.0f;
            float qz = ((unsigned short)(valueU.b() << 8) | valueL.b()) / 65535.0f;

            osg::Vec3 pos(mins[0] * (1.0f - qx) + maxs[0] * qx, mins[1] * (1.0f - qy) + maxs[1] * qy,
                          mins[2] * (1.0f - qz) + maxs[2] * qz);
            va[i] = osg::Vec3(unlog(pos[0]), unlog(pos[1]), unlog(pos[2]));
        }
    }

    static void createSogScales(osg::Vec3Array& sa, osg::Image* scales, std::vector<float>& codes)
    {
        if (!scales) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'scales' image missing\n"; return; }
        if (scales->getDataType() != GL_UNSIGNED_BYTE || scales->getPixelFormat() != GL_RGBA)
        { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'scales' image format mismatch\n"; return; }

        osg::Vec4ub* ptr = (osg::Vec4ub*)scales->data(); codes.resize(256);
        for (size_t i = 0; i < sa.size(); ++i)
        {
            osg::Vec4ub value = *(ptr + i);
            sa[i] = osg::Vec3(exp(codes[value.r()]), exp(codes[value.g()]), exp(codes[value.b()]));
        }
    }

    static void createSogRotations(osg::Vec4Array& qa, osg::Image* quats)
    {
        if (!quats) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'quats' image missing\n"; return; }
        if (quats->getDataType() != GL_UNSIGNED_BYTE || quats->getPixelFormat() != GL_RGBA)
        { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'quats' image format mismatch\n"; return; }

        osg::Vec4ub* ptr = (osg::Vec4ub*)quats->data();
        for (size_t i = 0; i < qa.size(); ++i)
        {
            osg::Vec4ub value = *(ptr + i);
            float a = (value.r() / 255.0f - 0.5f) * 2.0f / sqrt(2.0f);
            float b = (value.g() / 255.0f - 0.5f) * 2.0f / sqrt(2.0f);
            float c = (value.b() / 255.0f - 0.5f) * 2.0f / sqrt(2.0f);
            float t = a * a + b * b + c * c;
            float d = sqrt(osg::maximum(0.0f, 1.0f - t));

            switch (value.a() - 252)
            {
            case 0: qa[i].set(d, a, b, c); break;  // omitted = x
            case 1: qa[i].set(a, d, b, c); break;  // omitted = y
            case 2: qa[i].set(a, b, d, c); break;  // omitted = z
            case 3: qa[i].set(a, b, c, d); break;  // omitted = w
            default: OSG_NOTICE << "[ReaderWriter3DGS] Bad SOG quat: " << value.a() << "\n"; break;
            }
        }
    }

    static void createSogColors0(osg::Vec4Array& r0, osg::Vec4Array& g0, osg::Vec4Array& b0, osg::FloatArray& a,
                                 osg::Image* sh0, std::vector<float>& codes)
    {
        if (!sh0) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'sh0' image missing\n"; return; }
        if (sh0->getDataType() != GL_UNSIGNED_BYTE || sh0->getPixelFormat() != GL_RGBA)
        { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'sh0' image format mismatch\n"; return; }

        //const static double SH_C0 = 0.28209479177387814;
        osg::Vec4ub* ptr = (osg::Vec4ub*)sh0->data(); codes.resize(256);
        for (size_t i = 0; i < a.size(); ++i)
        {
            osg::Vec4ub value = *(ptr + i); a[i] = value.a() / 255.0f;
            r0[i] = osg::Vec4(codes[value.r()], 0.0f, 0.0f, 0.0f);
            g0[i] = osg::Vec4(codes[value.g()], 0.0f, 0.0f, 0.0f);
            b0[i] = osg::Vec4(codes[value.b()], 0.0f, 0.0f, 0.0f);
        }
    }

    static void createSogColorsN(osg::Vec4Array& r0, osg::Vec4Array& g0, osg::Vec4Array& b0,
                                 osg::Vec4Array& r1, osg::Vec4Array& g1, osg::Vec4Array& b1,
                                 osg::Vec4Array& r2, osg::Vec4Array& g2, osg::Vec4Array& b2,
                                 osg::Vec4Array& r3, osg::Vec4Array& g3, osg::Vec4Array& b3,
                                 osg::Image* centroids, osg::Image* labels, std::vector<float>& codes,
                                 size_t numDegrees, size_t numEntries)
    {
        if (!centroids) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'shN_centroids' image missing\n"; return; }
        if (!labels) { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'shN_labels' image missing\n"; return; }
        if (centroids->getDataType() != GL_UNSIGNED_BYTE || centroids->getPixelFormat() != GL_RGBA ||
            labels->getDataType() != GL_UNSIGNED_BYTE || labels->getPixelFormat() != GL_RGBA)
        { OSG_NOTICE << "[ReaderWriter3DGS] SOG 'shN' image format mismatch\n"; return; }

        const static std::vector<int> coeffs = { 0, 3, 8, 15 };
        int coeff = coeffs[numDegrees]; std::vector<float> R(coeff), G(coeff), B(coeff);
        if (centroids->getOrigin() == osg::Image::BOTTOM_LEFT) centroids->flipVertical();

        osg::Vec4ub* ptrC = (osg::Vec4ub*)centroids->data();
        osg::Vec4ub* ptrL = (osg::Vec4ub*)labels->data(); codes.resize(256);
        for (size_t i = 0; i < r0.size(); ++i)
        {
            osg::Vec4ub value = *(ptrL + i);
            unsigned short q = (unsigned short)(value.g() << 8) | value.r();
            for (int j = 0; j < coeff; ++j)
            {
                const int cx = (int)(q % 64) * coeff + j, cy = (int)floor(q / 64);
                osg::Vec4ub center = *(ptrC + cy * centroids->s() + cx);
                R[j] = codes[center.r()]; G[j] = codes[center.g()]; B[j] = codes[center.b()];
            }

            r0[i] = osg::Vec4(r0[i].r(), R[0], R[1], R[2]);
            g0[i] = osg::Vec4(g0[i].r(), G[0], G[1], G[2]);
            b0[i] = osg::Vec4(b0[i].r(), B[0], B[1], B[2]);
            for (int k = 0; k < 4; ++k)
            {
                r1[i][k] = R[3 + k]; g1[i][k] = G[3 + k]; b1[i][k] = B[3 + k];
                r2[i][k] = R[7 + k]; g2[i][k] = G[7 + k]; b2[i][k] = B[7 + k];
                r3[i][k] = R[11 + k]; g3[i][k] = G[11 + k]; b3[i][k] = B[11 + k];
            }
        }
    }
}

osg::ref_ptr<osg::Node> loadSplatFromSOG(std::istream& in, const std::string& path, const std::string& ext)
{
    SogData sogData; picojson::value document;
    if (ext == "json")
    {
        std::string err = picojson::parse(document, in);
        if (!err.empty())
        {
            OSG_WARN << "[ReaderWriter3DGS] Failed to parse PlayCanvas' SOG data: " << err << std::endl;
            return NULL;
        }
        if (!parseSogMetaData(sogData, document, path, NULL)) return NULL;
    }
    else if (ext == "sog")
    {
        osg::ref_ptr<osg::Referenced> zip = osgVerse::CompressAuxiliary::createHandle(osgVerse::CompressAuxiliary::ZIP, in);
        bool done = parseSogMetaData(sogData, document, path, zip);
        osgVerse::CompressAuxiliary::destroyHandle(zip); if (!done) return NULL;
    }

    // Create data arrays from loaded images
    size_t count = sogData.count; if (!count) return NULL;
    osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array(count), scale = new osg::Vec3Array(count);
    osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array(count); osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray(count);
    osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array(count), gD0 = new osg::Vec4Array(count), bD0 = new osg::Vec4Array(count);
    createSogPositions(*pos, sogData.images["means_0"].get(), sogData.images["means_1"].get(), sogData.meansMin, sogData.meansMax);
    createSogScales(*scale, sogData.images["scales_0"].get(), sogData.scaleCode); createSogRotations(*rot, sogData.images["quats_0"].get());
    createSogColors0(*rD0, *gD0, *bD0, *alpha, sogData.images["sh0_0"].get(), sogData.sh0Code);

#if true
    osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry;
    geom->setShDegrees(sogData.numDegrees); geom->setPosition(pos.get());
    geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
    geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get()); geom->setShBlue(0, bD0.get()); geom->finalize();

    if (sogData.numDegrees == 3)  // FIXME: consider degree 1 and 2?
    {
        osg::ref_ptr<osg::Vec4Array> rD1 = new osg::Vec4Array(count), gD1 = new osg::Vec4Array(count), bD1 = new osg::Vec4Array(count),
                                     rD2 = new osg::Vec4Array(count), gD2 = new osg::Vec4Array(count), bD2 = new osg::Vec4Array(count),
                                     rD3 = new osg::Vec4Array(count), gD3 = new osg::Vec4Array(count), bD3 = new osg::Vec4Array(count);
        createSogColorsN(*rD0, *gD0, *bD0, *rD1, *gD1, *bD1, *rD2, *gD2, *bD2, *rD3, *gD3, *bD3,
                         sogData.images["shN_0"].get(), sogData.images["shN_1"].get(),
                         sogData.shNCode, sogData.numDegrees, sogData.numEntries);
        geom->setShRed(1, rD1.get()); geom->setShGreen(1, gD1.get()); geom->setShBlue(1, bD1.get());
        geom->setShRed(2, rD2.get()); geom->setShGreen(2, gD2.get()); geom->setShBlue(2, bD2.get());
        geom->setShRed(3, rD3.get()); geom->setShGreen(3, gD3.get()); geom->setShBlue(3, bD3.get());
    }
#else
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(pos.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, pos->size()));
#endif

    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable(geom.get()); return root;
}
