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
    static float signum(float x) { return float((x > 0.0f) - (x < 0.0f)); }
    static float unlog(float x) { return signum(x) * (exp(abs(x)) - 1.0f); }
    static float sigmoidInv(float x) { float e = osg::minimum(1.0 - 1e-6, osg::maximum(1e-6, (double)x)); return (float)log(e / (1.0 - e)); };

    static osg::Image* readDataImage(const std::string& path)
    {
        osg::Image* img = osgDB::readImageFile(path + ".verse_webp");
        if (!img) img = osgDB::readImageFile(path); return img;
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
            float qx = (unsigned short(valueU.r() << 8) | valueL.r()) / 65535.0f;
            float qy = (unsigned short(valueU.g() << 8) | valueL.g()) / 65535.0f;
            float qz = (unsigned short(valueU.b() << 8) | valueL.b()) / 65535.0f;

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

        osg::Vec4ub* ptr = (osg::Vec4ub*)scales->data(); codes.resize(255);
        for (size_t i = 0; i < sa.size(); ++i)
        {
            osg::Vec4ub value = *(ptr + i);
            sa[i] = osg::Vec3(codes[value.r()], codes[value.g()], codes[value.b()]);
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

        osg::Vec4ub* ptr = (osg::Vec4ub*)sh0->data(); codes.resize(255);
        const double SH_C0 = 0.28209479177387814;
        for (size_t i = 0; i < a.size(); ++i)
        {
            osg::Vec4ub value = *(ptr + i); a[i] = value.a() / 255.0f;
            r0[i] = osg::Vec4(codes[value.r()] * SH_C0 + 0.5f, 0.0f, 0.0f, 0.0f);
            g0[i] = osg::Vec4(codes[value.g()] * SH_C0 + 0.5f, 0.0f, 0.0f, 0.0f);
            b0[i] = osg::Vec4(codes[value.b()] * SH_C0 + 0.5f, 0.0f, 0.0f, 0.0f);
        }
    }
}

osg::ref_ptr<osg::Node> loadSplatFromSOG(std::istream& in, const std::string& path)
{
    std::string reserved0, reserved1, err;
    picojson::value document; err = picojson::parse(document, in);
    if (!err.empty())
    {
        OSG_WARN << "[ReaderWriter3DGS] Failed to parse PlayCanvas' SOG data: " << err << std::endl;
        return NULL;
    }

    // Read information from .json
    picojson::value versionObj = document.get("version");
    picojson::value countObj = document.get("count");
    picojson::value meansObj = document.get("means");
    picojson::value scalesObj = document.get("scales");
    picojson::value quatsObj = document.get("quats");
    picojson::value sh0Obj = document.get("sh0");
    picojson::value shNObj = document.get("shN");

    double version = versionObj.is<double>() ? versionObj.get<double>() : 1;
    double count = countObj.is<double>() ? countObj.get<double>() : 0;
    if (version < 2) { OSG_NOTICE << "[ReaderWriter3DGS] SOG version 1 not supported\n"; return NULL; }

    std::map<std::string, osg::ref_ptr<osg::Image>> images;
    std::vector<float> scaleCode, sh0Code; osg::Vec3 meansMin, meansMax;
    if (meansObj.is<picojson::object>())
    {
        picojson::array mins = meansObj.get("mins").get<picojson::array>();
        picojson::array maxs = meansObj.get("maxs").get<picojson::array>();
        if (mins.size() > 2 && maxs.size() > 2)
        {
            for (size_t i = 0; i < 3; ++i)
            { meansMin[i] = mins[i].get<double>(); meansMax[i] = maxs[i].get<double>(); }
        }

        picojson::array files = meansObj.get("files").get<picojson::array>();
        for (size_t i = 0; i < files.size(); ++i)
            images["means_" + std::to_string(i)] = readDataImage(path + "/" + files[i].get<std::string>());
    }

    if (quatsObj.is<picojson::object>())
    {
        picojson::array files = quatsObj.get("files").get<picojson::array>();
        for (size_t i = 0; i < files.size(); ++i)
            images["quats_" + std::to_string(i)] = readDataImage(path + "/" + files[i].get<std::string>());
    }

    if (scalesObj.is<picojson::object>())
    {
        picojson::array codebook = scalesObj.get("codebook").get<picojson::array>();
        for (size_t i = 0; i < codebook.size(); ++i) scaleCode.push_back(codebook[i].get<double>());

        picojson::array files = scalesObj.get("files").get<picojson::array>();
        for (size_t i = 0; i < files.size(); ++i)
            images["scales_" + std::to_string(i)] = readDataImage(path + "/" + files[i].get<std::string>());
    }

    if (sh0Obj.is<picojson::object>())
    {
        picojson::array codebook = sh0Obj.get("codebook").get<picojson::array>();
        for (size_t i = 0; i < codebook.size(); ++i) sh0Code.push_back(codebook[i].get<double>());

        picojson::array files = sh0Obj.get("files").get<picojson::array>();
        for (size_t i = 0; i < files.size(); ++i)
            images["sh0_" + std::to_string(i)] = readDataImage(path + "/" + files[i].get<std::string>());
    }

    // Create data arrays from loaded images
    osg::ref_ptr<osg::Vec3Array> pos = new osg::Vec3Array(count), scale = new osg::Vec3Array(count);
    osg::ref_ptr<osg::Vec4Array> rot = new osg::Vec4Array(count); osg::ref_ptr<osg::FloatArray> alpha = new osg::FloatArray(count);
    osg::ref_ptr<osg::Vec4Array> rD0 = new osg::Vec4Array(count), gD0 = new osg::Vec4Array(count), bD0 = new osg::Vec4Array(count);
    createSogPositions(*pos, images["means_0"].get(), images["means_1"].get(), meansMin, meansMax);
    createSogScales(*scale, images["scales_0"].get(), scaleCode); createSogRotations(*rot, images["quats_0"].get());
    createSogColors0(*rD0, *gD0, *bD0, *alpha, images["sh0_0"].get(), sh0Code);

#if true
    osg::ref_ptr<osgVerse::GaussianGeometry> geom = new osgVerse::GaussianGeometry;
    geom->setShDegrees(0); geom->setPosition(pos.get());
    geom->setScaleAndRotation(scale.get(), rot.get(), alpha.get());
    geom->setShRed(0, rD0.get()); geom->setShGreen(0, gD0.get());
    geom->setShBlue(0, bD0.get()); geom->finalize();
#else
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(pos.get());
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, pos->size()));
#endif

    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable(geom.get()); return root;
}
