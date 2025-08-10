#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <chrono>
#include <codecvt>
#include <iostream>
#include <array>
#include <random>

#include <backward.hpp>
#include <tinycolormap.hpp>
#include <mikktspace.h>
#include <PoissonGenerator.h>
#include <normalmap/normalmapgenerator.h>
#include <normalmap/specularmapgenerator.h>
#ifdef VERSE_WINDOWS
#   include <windows.h>
#endif

#include <modeling/Utilities.h>
#include "ShaderLibrary.h"
#include "Pipeline.h"
#include "Utilities.h"

#define BACKWARD_MESSAGE(msg, n) \
    { backward::Printer printer; backward::StackTrace st; st.load_here(n); st.skip_n_firsts(2); \
      std::stringstream ss; ss << std::string(15, '#') << "\n"; printer.print(st, ss); \
      msg += ss.str() + std::string(15, '#') + "\n"; }

#ifndef GL_GEOMETRY_SHADER
#   define GL_GEOMETRY_SHADER  0x8DD9
#endif

#if defined(VERSE_MSVC)
#   if defined(INSTALL_PATH_PREFIX)
std::string BASE_DIR(INSTALL_PATH_PREFIX);
#   else
std::string BASE_DIR("..");
#   endif
#elif defined(VERSE_WASM) || defined(VERSE_ANDROID) || defined(VERSE_IOS)
std::string BASE_DIR("/assets");
#else
std::string BASE_DIR("..");
#endif

std::string SHADER_DIR(BASE_DIR + "/shaders/");
std::string SKYBOX_DIR(BASE_DIR + "/skyboxes/");
std::string MISC_DIR(BASE_DIR + "/misc/");
static int g_argumentCount = 0;

static std::string refreshGlobalDirectories(const std::string& base)
{
    BASE_DIR = base;
    SHADER_DIR = BASE_DIR + "/shaders/";
    SKYBOX_DIR = BASE_DIR + "/skyboxes/";
    MISC_DIR = BASE_DIR + "/misc/";
    return base + std::string("/bin/");
}

static osg::Camera* createImageCamera(osg::Image* image, const osg::BoundingBox& bbox)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setViewport(0, 0, image->s(), image->t());
    camera->attach(osg::Camera::COLOR_BUFFER0, image);

    float zn = bbox.zMin(), zf = bbox.zMax(); if (zf <= zn) zf = zn + 10.0f;
    osg::Vec3 center = bbox.center(); center.z() = zf;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setProjectionMatrix(osg::Matrix::ortho(
        bbox.xMin() - center.x(), bbox.xMax() - center.x(),
        bbox.yMin() - center.y(), bbox.yMax() - center.y(), zn, zf));
    camera->setViewMatrix(osg::Matrix::lookAt(center, bbox.center(), osg::Y_AXIS));
    return camera.release();
}

static void setupOffscreenCamera(osg::Camera* camera, osg::Image* image)
{
    camera->setViewport(0, 0, image->s(), image->t());
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0;
    traits->width = image->s(); traits->height = image->t();
    traits->doubleBuffer = false; traits->pbuffer = true;
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    camera->setGraphicsContext(gc);
}

class MyReadFileCallback : public osgDB::ReadFileCallback
{
public:
    virtual osgDB::ReaderWriter::ReadResult openArchive(
            const std::string& f, osgDB::ReaderWriter::ArchiveStatus status,
            unsigned int indexBlockSizeHint, const osgDB::Options* useObjectCache)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::openArchive(file, status, indexBlockSizeHint, useObjectCache);
    }

    virtual osgDB::ReaderWriter::ReadResult readObject(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readObject(file, opt);
    }

    virtual osgDB::ReaderWriter::ReadResult readImage(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readImage(file, opt);
    }

    virtual osgDB::ReaderWriter::ReadResult readHeightField(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readHeightField(file, opt);
    }

    virtual osgDB::ReaderWriter::ReadResult readNode(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readNode(file, opt);
    }

    virtual osgDB::ReaderWriter::ReadResult readShader(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readShader(file, opt);
    }
#if OSG_VERSION_GREATER_THAN(3, 5, 0)
    virtual osgDB::ReaderWriter::ReadResult readScript(const std::string& f, const osgDB::Options* opt)
    {
        std::string file = osgVerse::Utf8StringValidator::check(f) ? f
                         : osgDB::convertStringFromCurrentCodePageToUTF8(f);
        return osgDB::ReadFileCallback::readScript(file, opt);
    }
#endif

protected:
    virtual ~MyReadFileCallback() {}
};

namespace osgVerse
{
    std::string Utf8StringValidator::convert(const std::string& s)
    {
        return osgVerse::Utf8StringValidator::check(s) ? s :
               osgDB::convertStringFromCurrentCodePageToUTF8(s);
    }

    std::wstring Utf8StringValidator::convertW(const std::string& s)
    {
        return osgVerse::Utf8StringValidator::check(s) ? osgDB::convertUTF8toUTF16(s) :
               osgDB::convertUTF8toUTF16(osgDB::convertStringFromCurrentCodePageToUTF8(s));
    }

    osg::ArgumentParser globalInitialize(int argc, char** argv)
    {
        setlocale(LC_ALL, ".UTF8");
        osg::setNotifyLevel(osg::NOTICE);
        osgDB::Registry::instance()->setReadFileCallback(new MyReadFileCallback);
        if (argv && argc > 0)
        {
            std::string path = osgDB::getFilePath(argv[0]);
            osgDB::setCurrentWorkingDirectory(path);
        }

#if defined(VERSE_WASM) || defined(VERSE_ANDROID) || defined(VERSE_IOS)
        // anything to do here?
        OSG_NOTICE << "[osgVerse] WebAssembly pipeline initialization." << std::endl;
#else
        std::string workingPath = BASE_DIR + std::string("/bin/");
        if (!osgDB::fileExists(workingPath))
            workingPath = refreshGlobalDirectories("..");

        if (!osgDB::fileExists(workingPath))
        {
            OSG_FATAL << "[osgVerse] Working directory " << workingPath
                      << " not found. Following work may fail." << std::endl;
        }
        else
            OSG_NOTICE << "[osgVerse] Working directory: " << workingPath << std::endl;

        osgDB::FilePathList& filePaths = osgDB::getLibraryFilePathList();
#   ifdef OSGPLUGIN_PREFIX
        filePaths.push_back(BASE_DIR + "/" + OSGPLUGIN_PREFIX);
#   endif
        filePaths.push_back(workingPath);
#endif
        ShaderLibrary::instance()->refreshModules(BASE_DIR + std::string("/shaders/"));

        osg::ref_ptr<osgDB::Registry> regObject = osgDB::Registry::instance();
#ifdef VERSE_STATIC_BUILD
        // anything to do here?
#else
        // Pre-load libraries to register web/streaming/database protocols
        regObject->loadLibrary(regObject->createLibraryNameForExtension("verse_web"));
        regObject->loadLibrary(regObject->createLibraryNameForExtension("verse_ms"));
        regObject->loadLibrary(regObject->createLibraryNameForExtension("verse_leveldb"));
        regObject->loadLibrary(regObject->createLibraryNameForExtension("verse_mbtiles"));
#endif
        regObject->addFileExtensionAlias("ept", "verse_ept");
        regObject->addFileExtensionAlias("fbx", "verse_fbx");
        regObject->addFileExtensionAlias("ktx", "verse_ktx");
        regObject->addFileExtensionAlias("vdb", "verse_vdb");
        regObject->addFileExtensionAlias("mvt", "verse_mvt");
        regObject->addFileExtensionAlias("gltf", "verse_gltf");
        regObject->addFileExtensionAlias("glb", "verse_gltf");
        //regObject->addFileExtensionAlias("tiff", "verse_tiff");
        regObject->addFileExtensionAlias("rseq", "verse_image");
        regObject->addFileExtensionAlias("json", "verse_tiles");
        regObject->addFileExtensionAlias("s3c", "verse_tiles");
        regObject->addFileExtensionAlias("terrain", "verse_terrain");
#if defined(VERSE_WASM) || defined(VERSE_ANDROID) || defined(VERSE_IOS)
        regObject->addFileExtensionAlias("jpg", "verse_image");
        regObject->addFileExtensionAlias("jpeg", "verse_image");
        regObject->addFileExtensionAlias("png", "verse_image");
        regObject->addFileExtensionAlias("psd", "verse_image");
        regObject->addFileExtensionAlias("hdr", "verse_image");
#endif

        g_argumentCount = argc;
        if (argc == 0) return osg::ArgumentParser(NULL, NULL);
        return osg::ArgumentParser(&g_argumentCount, argv);
    }
}

typedef std::array<unsigned int, 3> Vec3ui;
static std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
static std::default_random_engine generator;

/// MikkTSpace visitor utilities
struct MikkTSpaceHelper
{
    std::vector<Vec3ui> _faceList;
    osg::Vec4Array *tangents;
    osg::Geometry* _geometry;

    bool initialize(SMikkTSpaceContext* sc, osg::Geometry* g)
    {
        sc->m_pInterface->m_getNumFaces = MikkTSpaceHelper::mikk_getNumFaces;
        sc->m_pInterface->m_getNumVerticesOfFace = MikkTSpaceHelper::mikk_getNumVerticesOfFace;
        sc->m_pInterface->m_getPosition = MikkTSpaceHelper::mikk_getPosition;
        sc->m_pInterface->m_getNormal = MikkTSpaceHelper::mikk_getNormal;
        sc->m_pInterface->m_getTexCoord = MikkTSpaceHelper::mikk_getTexCoord;
        sc->m_pInterface->m_setTSpaceBasic = MikkTSpaceHelper::mikk_setTSpaceBasic;
        sc->m_pInterface->m_setTSpace = NULL; sc->m_pUserData = this; _geometry = g;

        osg::Vec3Array* va = vArray(); osg::Vec3Array* na = nArray();
        osg::Vec2Array* ta = tArray();
        if (!va || !na || !ta) return false;
        if (va->size() != na->size() || va->size() != ta->size()) return false;

        tangents = new osg::Vec4Array(va->size());
        g->setVertexAttribArray(6, tangents); g->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);
        return true;
    }

    osg::Vec3Array* vArray() { return static_cast<osg::Vec3Array*>(_geometry->getVertexArray()); }
    osg::Vec3Array* nArray() { return static_cast<osg::Vec3Array*>(_geometry->getNormalArray()); }
    osg::Vec2Array* tArray() { return static_cast<osg::Vec2Array*>(_geometry->getTexCoordArray(0)); }

    void operator()(unsigned int i0, unsigned int i1, unsigned int i2)
    { _faceList.push_back(Vec3ui{i0, i1, i2}); }

    static MikkTSpaceHelper* me(const SMikkTSpaceContext* pContext)
    { return static_cast<MikkTSpaceHelper*>(pContext->m_pUserData); }

    static int mikk_getNumFaces(const SMikkTSpaceContext* pContext)
    { return (int)me(pContext)->_faceList.size(); }

    static int mikk_getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
    { return 3; }

    static void mikk_getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[],
                                 const int iFace, const int iVert)
    {
        osg::Vec3Array* vArray = me(pContext)->vArray();
        const osg::Vec3& v = vArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 3; ++i) fvPosOut[i] = v[i];
    }

    static void mikk_getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[],
                               const int iFace, const int iVert)
    {
        osg::Vec3Array* nArray = me(pContext)->nArray();
        const osg::Vec3& v = nArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 3; ++i) fvNormOut[i] = v[i];
    }

    static void mikk_getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[],
                                 const int iFace, const int iVert)
    {
        osg::Vec2Array* tArray = me(pContext)->tArray();
        const osg::Vec2& v = tArray->at(me(pContext)->_faceList[iFace][iVert]);
        for (int i = 0; i < 2; ++i) fvTexcOut[i] = v[i];
    }

    static void mikk_setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[],
                                    const float fSign, const int iFace, const int iVert)
    {
        MikkTSpaceHelper* self = me(pContext); unsigned int vIndex = self->_faceList[iFace][iVert];
        osg::Vec4 T(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
        //osg::Vec3 N = self->nArray()->at(vIndex); osg::Vec3 B = (N ^ T) * fSign;
        (*self->tangents)[vIndex] = T; //(*self->binormals)[vIndex] = B;
    }
};

namespace osgVerse
{
    std::string getNodePathID(osg::Object& obj, osg::Node* root, char sep)
    {
        std::string pathID = obj.getName();
        osg::Node* parent = NULL;

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        osg::Drawable* d = obj.asDrawable();
#else
        osg::Drawable* d = dynamic_cast<osg::Drawable*>(&obj);
#endif
        if (d != NULL)
        {
            if (d->getNumParents() > 0) parent = d->getParent(0); if (!parent) return pathID;
            if (pathID.empty()) pathID = std::to_string(parent->asGeode()->getDrawableIndex(d));
        }
        else
        {
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
            osg::Node* n = obj.asNode(); if (n == NULL) return pathID;
#else
            osg::Node* n = dynamic_cast<osg::Node*>(&obj); if (n == NULL) return pathID;
#endif
            if (n->getNumParents() > 0) parent = n->getParent(0); if (!parent) return pathID;
            if (pathID.empty()) pathID = std::to_string(parent->asGroup()->getChildIndex(n));
        }

        while (parent != NULL && parent != root)
        {
            osg::Node* current = parent; std::string currentID = current->getName();
            parent = (parent->getNumParents() > 0) ? parent->getParent(0) : NULL;
            if (parent != NULL && currentID.empty())
                currentID = std::to_string(parent->asGroup()->getChildIndex(current));
            if (currentID.empty()) currentID = "root";
            pathID = currentID + sep + pathID;
        }
        return pathID;
    }

    osg::Object* getFromPathID(const std::string& idData, osg::Object* root, char sep)
    {
        osgDB::StringList idList; osgDB::split(idData, idList, sep);
        if (root->getName() != idList[0] && idList[0] != "root") return NULL;
        
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        osg::Node* node = root ? root->asNode() : NULL;
#else
        osg::Node* node = dynamic_cast<osg::Node*>(root);
#endif
        if (node != NULL)
        {
            for (size_t i = 1; i < idList.size(); ++i)
            {
                const std::string& name = idList[i];
                osg::Group* parent = node ? node->asGroup() : NULL;
                if (parent == NULL)
                {
                    osg::Geode* parentG = node ? node->asGeode() : NULL;
                    if (parentG != NULL)
                    {
                        int index = atoi(name.c_str());
                        if (name == "0" && parentG->getNumDrawables() > 0)
                            return parentG->getDrawable(0);
                        else if (index > 0 && index < (int)parentG->getNumDrawables())
                            return parentG->getDrawable(index);
                        else
                        {
                            for (size_t j = 0; j < parentG->getNumDrawables(); ++j)
                            {
                                if (parentG->getDrawable(j)->getName() == name)
                                    return parentG->getDrawable(j);
                            }
                        }
                    }
                    return NULL;
                }

                node = NULL;
                if (name == "0")
                    { if (parent->getNumChildren() > 0) node = parent->getChild(0); }
                else
                {
                    int index = atoi(name.c_str());
                    if (index > 0 && index < (int)parent->getNumChildren())
                        node = parent->getChild(index);
                    else
                    {
                        for (size_t j = 0; j < parent->getNumChildren(); ++j)
                        {
                            if (parent->getChild(j)->getName() == name)
                            { node = parent->getChild(j); break; }
                        }
                    }
                }  // if (name == "0")
            }
            return node;
        }
        return root;
    }

    osg::Texture* generateNoises2D(int numCols, int numRows)
    {
        std::vector<osg::Vec3f> noises;
        for (int i = 0; i < numCols; ++i)
            for (int j = 0; j < numRows; ++j)
            {
                float angle = 2.0f * osg::PI * randomFloats(generator) / 8.0f;
                osg::Vec3 noise(cosf(angle), sinf(angle), randomFloats(generator));
                noises.push_back(noise);
            }

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(numCols, numRows, 1, GL_RGB, GL_FLOAT);
#ifdef VERSE_EMBEDDED_GLES2
        image->setInternalTextureFormat(GL_RGB);
#else
        image->setInternalTextureFormat(GL_RGB32F_ARB);
#endif
        memcpy(image->data(), (unsigned char*)&noises[0], image->getTotalSizeInBytes());

        osg::ref_ptr<osg::Texture2D> noiseTex = new osg::Texture2D;
        noiseTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        noiseTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        noiseTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        noiseTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        noiseTex->setImage(image.get());
        return noiseTex.release();
    }

    osg::Texture* generatePoissonDiscDistribution(int numSamples, int numRows)
    {
        std::vector<osg::Vec3f> distribution;
        for (int j = 0; j < numRows; ++j)
        {
            size_t attempts = 0; PoissonGenerator::DefaultPRNG prng;
            auto points = PoissonGenerator::GeneratePoissonPoints(numSamples * 2, prng);
            while (points.size() < numSamples && ++attempts < 100)
                points = PoissonGenerator::GeneratePoissonPoints(numSamples * 2, prng);

            for (int i = 0; i < numSamples; ++i)
                distribution.push_back(osg::Vec3(points[i].x, points[i].y, 0.0f));
        }

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(numSamples, numRows, 1, GL_RGB, GL_FLOAT);
#ifdef VERSE_EMBEDDED_GLES2
        image->setInternalTextureFormat(GL_RGB);
#else
        image->setInternalTextureFormat(GL_RGB32F_ARB);
#endif
        memcpy(image->data(), (unsigned char*)&distribution[0], image->getTotalSizeInBytes());

        osg::ref_ptr<osg::Texture> noiseTex = NULL;
        if (numRows > 1) noiseTex = new osg::Texture2D; else noiseTex = new osg::Texture1D;
        noiseTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        noiseTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        noiseTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        noiseTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        noiseTex->setImage(0, image.get());
        return noiseTex.release();
    }

    osg::Image* generateTransferFunction(int type, int resolution, int alpha)
    {
        osg::Vec4ub* values = new osg::Vec4ub[resolution];
        for (int i = 0; i < resolution; ++i)
        {
            float value = (float)i / (float)(resolution - 1);
            tinycolormap::Color c = tinycolormap::GetColor(value, (tinycolormap::ColormapType)type);
            values[i] = osg::Vec4ub((unsigned char)(c.r() * 255.0f), (unsigned char)(c.g() * 255.0f),
                                    (unsigned char)(c.b() * 255.0f), (unsigned char)alpha);
        }

        osg::ref_ptr<osg::Image> image1D = new osg::Image;
        image1D->setImage(resolution, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
            (unsigned char*)values, osg::Image::USE_NEW_DELETE);
        return image1D.release();
    }

    osg::Texture2D* createDefaultTexture(const osg::Vec4& color)
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA);

        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
        tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        tex2D->setImage(image.get()); return tex2D.release();
    }

    osg::Texture2DArray* createDefaultTextureArray(const osg::Vec4& color, int layers)
    {
        osg::ref_ptr<osg::Texture2DArray> texArray = new osg::Texture2DArray;
        texArray->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texArray->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        texArray->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        texArray->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        for (int i = 0; i < layers; ++i)
        {
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            image->setInternalTextureFormat(GL_RGBA);

            osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
            *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
            texArray->setImage(i, image.get());
        }
        return texArray.release();
    }

    osg::Texture2D* createTexture2D(osg::Image* image, osg::Texture::WrapMode mode)
    {
        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setWrap(osg::Texture2D::WRAP_S, mode);
        tex2D->setWrap(osg::Texture2D::WRAP_T, mode);
        tex2D->setResizeNonPowerOfTwoHint(false);
        tex2D->setImage(image); return tex2D.release();
    }

    osg::Geode* createScreenQuad(const osg::Vec3& corner, float width, float height, const osg::Vec4& uvRange)
    {
        osg::Geometry* geom = osg::createTexturedQuadGeometry(
            corner, osg::Vec3(width, 0.0f, 0.0f), osg::Vec3(0.0f, height, 0.0f),
            uvRange[0], uvRange[1], uvRange[2], uvRange[3]);
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        osg::ref_ptr<osg::Geode> quad = new osg::Geode;
        quad->addDrawable(geom);

        int values = osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
        quad->getOrCreateStateSet()->setAttribute(
            new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL), values);
#endif
        quad->getOrCreateStateSet()->setMode(GL_LIGHTING, values);
        return quad.release();
    }

    osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Image* image,
                                 osg::Node* child, osg::GraphicsContext* gc)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        if (gc) camera->setGraphicsContext(gc);
        if (child) camera->addChild(child);

        if (image)
        {
            camera->setViewport(0, 0, image->s(), image->t());
            camera->attach(buffer, image);
        }
        return camera.release();
    }

    osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex,
                                 osg::GraphicsContext* gc, bool screenSpaced)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        if (gc) camera->setGraphicsContext(gc);
        if (tex)
        {
            tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
            tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
            camera->attach(buffer, tex);
        }

        if (screenSpaced)
        {
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setViewMatrix(osg::Matrix::identity());
            camera->addChild(createScreenQuad(osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)));
        }
        return camera.release();
    }

    osg::Group* createRTTCube(osg::Camera::BufferComponent buffer, osg::TextureCubeMap* tex,
                              osg::Node* child, osg::GraphicsContext* gc)
    {
        osg::ref_ptr<osg::Group> cameraRoot = new osg::Group;
        for (int i = 0; i < 6; ++i)
        {
            osg::ref_ptr<osg::Camera> camera = new osg::Camera;
            camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setReferenceFrame(osg::Transform::RELATIVE_RF);
            if (gc) camera->setGraphicsContext(gc);
            if (child) camera->addChild(child);
            cameraRoot->addChild(camera.get());

            if (tex)
            {
                tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
                tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
                camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
                camera->attach(buffer, tex, 0, i);
            }

            switch ((osg::TextureCubeMap::Face)i)
            {
            case osg::TextureCubeMap::POSITIVE_X:
                camera->setViewMatrix(osg::Matrix::rotate(-osg::PI_2, osg::Z_AXIS));
                camera->setName("POSITIVE_X"); break;
            case osg::TextureCubeMap::NEGATIVE_X:
                camera->setViewMatrix(osg::Matrix::rotate(osg::PI_2, osg::Z_AXIS));
                camera->setName("NEGATIVE_X"); break;
            case osg::TextureCubeMap::POSITIVE_Y:
                camera->setViewMatrix(osg::Matrix::rotate(-osg::PI_2, osg::X_AXIS));
                camera->setName("POSITIVE_Y"); break;
            case osg::TextureCubeMap::NEGATIVE_Y:
                camera->setViewMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
                camera->setName("NEGATIVE_Y"); break;
            case osg::TextureCubeMap::POSITIVE_Z:
                camera->setViewMatrix(osg::Matrix::identity());
                camera->setName("POSITIVE_Z"); break;
            case osg::TextureCubeMap::NEGATIVE_Z:
                camera->setViewMatrix(osg::Matrix::rotate(osg::PI, osg::Z_AXIS));
                camera->setName("NEGATIVE_Z"); break;
            default: break;
            }
        }
        return cameraRoot.release();
    }

    osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setClearMask(GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setAllowEventFocus(false);
        if (gc) camera->setGraphicsContext(gc);

        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewport(0, 0, w, h);
        camera->setProjectionMatrix(osg::Matrix::ortho(0.0, w, 0.0, h, -1.0, 1.0));
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        return camera.release();
    }

    osg::Camera* createHUDCamera(osg::GraphicsContext* gc, int w, int h, const osg::Vec3& quadPt,
                                 float quadW, float quadH, bool screenSpaced)
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setAllowEventFocus(false);
        if (gc) camera->setGraphicsContext(gc);
        camera->setViewport(0, 0, w, h);
        camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        if (screenSpaced)
        {
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setViewMatrix(osg::Matrix::identity());
            if (quadW > 0.0f && quadH > 0.0f)
                camera->addChild(createScreenQuad(quadPt, quadW, quadH, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)));
        }
        return camera.release();
    }

    void alignCameraToBox(osg::Camera* camera, const osg::BoundingBoxd& bb,
                          int resW, int resH, osg::TextureCubeMap::Face face)
    {
        double xHalf = (bb.xMax() - bb.xMin()) / 2.0, yHalf = (bb.yMax() - bb.yMin()) / 2.0;
        double zHalf = (bb.zMax() - bb.zMin()) / 2.0, h0 = 0.0, h1 = 0.0, hD = 0.0;
        osg::Vec3 dir = osg::Z_AXIS, up = osg::Y_AXIS;
        switch (face)
        {
        case osg::TextureCubeMap::POSITIVE_X:
            h0 = yHalf; h1 = zHalf; hD = xHalf; dir = osg::X_AXIS; up = osg::Z_AXIS; break;
        case osg::TextureCubeMap::NEGATIVE_X:
            h0 = yHalf; h1 = zHalf; hD = xHalf; dir = -osg::X_AXIS; up = osg::Z_AXIS; break;
        case osg::TextureCubeMap::POSITIVE_Y:
            h0 = xHalf; h1 = zHalf; hD = yHalf; dir = osg::Y_AXIS; up = osg::Z_AXIS; break;
        case osg::TextureCubeMap::NEGATIVE_Y:
            h0 = xHalf; h1 = zHalf; hD = yHalf; dir = -osg::Y_AXIS; up = osg::Z_AXIS; break;
        case osg::TextureCubeMap::NEGATIVE_Z:
            h0 = xHalf; h1 = yHalf; hD = zHalf; dir = -osg::Z_AXIS; break;
        default:  // POSITIVE_Z
            h0 = xHalf; h1 = yHalf; hD = zHalf; break;
        }

        double halfResW = (float)(resW / 2) - 0.5, halfResH = (float)(resH / 2) - 0.5;
        double halfPW = (h0 / halfResW) * 0.5, halfPH = (h1 / halfResH) * 0.5;
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrixAsLookAt(bb.center() + dir * hD, bb.center(), up);
        camera->setProjectionMatrixAsOrtho(
            -h0 - halfPW, h0 + halfPW, -h1 - halfPH, h1 + halfPH, 0.0, hD * 2.05);
        camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    }

    osg::HeightField* createHeightField(osg::Node* node, int resX, int resY, osg::View* userViewer)
    {
        osg::ComputeBoundsVisitor cbv; node->accept(cbv);
        osg::BoundingBox bbox = cbv.getBoundingBox();
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(resX, resY, 1, GL_LUMINANCE, GL_FLOAT);
        image->setInternalTextureFormat(GL_LUMINANCE32F_ARB);

        osg::ref_ptr<osg::Camera> camera = createImageCamera(image.get(), bbox);
        osg::ref_ptr<osgViewer::Viewer> viewer = dynamic_cast<osgViewer::Viewer*>(userViewer);
        if (!viewer) { viewer = new osgViewer::Viewer; setupOffscreenCamera(viewer->getCamera(), image); }
        viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        {
            const char* vsCode = {
                "VERSE_VS_OUT float depth;\n"
                "void main() {\n"
                "    depth = -(VERSE_MATRIX_MV * osg_Vertex).z;\n"
                "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
                "}\n"
            };
            const char* fsCode = {
                "VERSE_FS_IN float depth;\n"
                "VERSE_FS_OUT vec4 fragData;\n"
                "void main() {\n"
                "    fragData = vec4(depth, 1.0, 1.0, 1.0);\n"
                "    VERSE_FS_FINAL(fragData);\n"
                "}\n"
            };
            osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vsCode);
            osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, fsCode);
            int glVer = 0; int glslVer = ShaderLibrary::guessShaderVersion(glVer);

            osg::ref_ptr<osg::Program> prog = new osg::Program;
            Pipeline::createShaderDefinitions(vs, glVer, glslVer); prog->addShader(vs);
            Pipeline::createShaderDefinitions(fs, glVer, glslVer); prog->addShader(fs);
            camera->getOrCreateStateSet()->setAttributeAndModes(prog.get());

            osg::ref_ptr<osg::Group> root = new osg::Group;
            root->addChild(camera.get()); camera->addChild(node);
            viewer->setSceneData(root.get());

            HostTextureReserver reserver; root->accept(reserver); reserver.set(true);
            for (int i = 0; i < 2; ++i) viewer->frame(); reserver.set(false);
        }

        osg::ref_ptr<osg::HeightField> hf = new osg::HeightField;
        hf->allocate(resX, resY); hf->setOrigin(bbox._min);
        hf->setXInterval((bbox.xMax() - bbox.xMin()) / (float)(resX - 1));
        hf->setYInterval((bbox.yMax() - bbox.yMin()) / (float)(resY - 1));

        float* ptr = (float*)image->data(); float zRange = bbox.zMax() - bbox.zMin();
        for (int y = 0; y < resY; ++y) for (int x = 0; x < resX; ++x)
        {
            float eyeZ = *(ptr + (y * resX) + x);
            if (eyeZ <= 0.0f) hf->setHeight(x, y, 0.0f);
            else hf->setHeight(x, y, zRange - eyeZ);
        }
        return hf.release();
    }

    osg::Image* createSnapshot(osg::Node* node, int resX, int resY, osg::View* userViewer)
    {
        osg::ComputeBoundsVisitor cbv; node->accept(cbv);
        osg::BoundingBox bbox = cbv.getBoundingBox();
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(resX, resY, 1, GL_RGB, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGB8);

        osg::ref_ptr<osg::Camera> camera = createImageCamera(image.get(), bbox);
        osg::ref_ptr<osgViewer::Viewer> viewer = dynamic_cast<osgViewer::Viewer*>(userViewer);
        if (!viewer) { viewer = new osgViewer::Viewer; setupOffscreenCamera(viewer->getCamera(), image); }
        viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        {
            const char* vsCode = {
                "VERSE_VS_OUT vec2 uv;\n"
                "void main() {\n"
                "    uv = osg_MultiTexCoord0.st;\n"
                "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
                "}\n"
            };
            const char* fsCode = {
                "uniform sampler2D BaseTexture;\n"
                "VERSE_FS_IN vec2 uv;\n"
                "VERSE_FS_OUT vec4 fragData;\n"
                "void main() {\n"
                "    fragData = VERSE_TEX2D(BaseTexture, uv);\n"
                "    VERSE_FS_FINAL(fragData);\n"
                "}\n"
            };
            osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vsCode);
            osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, fsCode);
            int glVer = 0; int glslVer = ShaderLibrary::guessShaderVersion(glVer);

            osg::ref_ptr<osg::Program> prog = new osg::Program;
            Pipeline::createShaderDefinitions(vs, glVer, glslVer); prog->addShader(vs);
            Pipeline::createShaderDefinitions(fs, glVer, glslVer); prog->addShader(fs);
            camera->getOrCreateStateSet()->setAttributeAndModes(prog.get());
            camera->getOrCreateStateSet()->setTextureAttributeAndModes(
                0, createDefaultTexture(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f)));
            camera->getOrCreateStateSet()->addUniform(new osg::Uniform("BaseTexture", (int)0));

            osg::ref_ptr<osg::Group> root = new osg::Group;
            root->addChild(camera.get()); camera->addChild(node);
            viewer->setSceneData(root.get());

            HostTextureReserver reserver; root->accept(reserver); reserver.set(true);
            for (int i = 0; i < 2; ++i) viewer->frame(); reserver.set(false);
        }
        viewer->setSceneData(NULL); camera = NULL; viewer = NULL;
        return image.release();
    }

    osg::Image* createShadingResult(osg::StateSet& ss, int resX, int resY, osg::View* userViewer)
    {
        osg::ref_ptr<osg::Geode> node = createScreenQuad(osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        osg::ComputeBoundsVisitor cbv; node->accept(cbv);
        osg::BoundingBox bbox = cbv.getBoundingBox();
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(resX, resY, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA8);

        osg::ref_ptr<osg::Camera> camera = createImageCamera(image.get(), bbox);
        osg::ref_ptr<osgViewer::Viewer> viewer = dynamic_cast<osgViewer::Viewer*>(userViewer);
        if (!viewer) { viewer = new osgViewer::Viewer; setupOffscreenCamera(viewer->getCamera(), image); }
        viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        viewer->getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        {
            osg::ref_ptr<osg::Group> root = new osg::Group;
            root->addChild(camera.get()); camera->addChild(node);
            viewer->setSceneData(root.get()); node->setStateSet(new osg::StateSet(ss));
            for (int i = 0; i < 2; ++i) viewer->frame();
        }
        viewer->setSceneData(NULL); camera = NULL; viewer = NULL;
        return image.release();
    }

    TangentSpaceVisitor::TangentSpaceVisitor(const float threshold)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _angularThreshold(threshold)
    {
        _mikkiTSpace = new SMikkTSpaceContext;
        _mikkiTSpace->m_pInterface = new SMikkTSpaceInterface;
        _mikkiTSpace->m_pUserData = NULL;
    }

    TangentSpaceVisitor::~TangentSpaceVisitor()
    {
        if (_mikkiTSpace != NULL)
        {
            if (_mikkiTSpace->m_pInterface) delete _mikkiTSpace->m_pInterface;
            delete _mikkiTSpace; _mikkiTSpace = NULL;
        }
    }

    void TangentSpaceVisitor::apply(osg::Geode& node)
    {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom) apply(*geom);
        }
#endif
        traverse(node);
    }

    void TangentSpaceVisitor::apply(osg::Geometry& geom)
    {
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        if (geom.getNormalArray() == NULL) { traverse(geom); return; }
        if (geom.getNormalBinding() != osg::Geometry::BIND_PER_VERTEX) { traverse(geom); return; }

        if (geom.getVertexAttribArray(6) != NULL &&
            geom.getVertexAttribBinding(6) == osg::Geometry::BIND_PER_VERTEX)
        { traverse(geom); return; }  // already set
#else
        if (geom.getNormalArray() == NULL) return;
        if (geom.getNormalBinding() != osg::Geometry::BIND_PER_VERTEX) return;

        if (geom.getVertexAttribArray(6) != NULL &&
            geom.getVertexAttribBinding(6) == osg::Geometry::BIND_PER_VERTEX) return;
#endif

        osg::TriangleIndexFunctor<MikkTSpaceHelper> functor;
        geom.accept(functor);
        if (functor.initialize(_mikkiTSpace, &geom))
            genTangSpace(_mikkiTSpace, _angularThreshold);
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(geom);
#endif
    }

    NormalMapGenerator::NormalMapGenerator(double nStrength, double spScale, double spContrast, bool nInvert)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _nStrength(nStrength), _spScale(spScale), _spContrast(spContrast),
        _normalMapUnit(1), _specMapUnit(2), _nInvert(nInvert) {}

    void NormalMapGenerator::apply(osg::Node& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    void NormalMapGenerator::apply(osg::Geode& node)
    {
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable) apply(*drawable);
        }
#endif
        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    void NormalMapGenerator::apply(osg::Drawable& drawable)
    {
        if (drawable.getStateSet()) apply(*drawable.getStateSet());
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(drawable);
#endif
    }

    void NormalMapGenerator::apply(osg::StateSet& ss)
    {
        osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(
            ss.getTextureAttribute(0, osg::StateAttribute::TEXTURE));
        if (!tex2D) return;

        osg::Image* image = tex2D->getImage();
        if (!image || (image && !image->valid())) return;
        if ((image->getPixelFormat() != GL_RGBA && image->getPixelFormat() != GL_RGB) ||
            image->getDataType() != GL_UNSIGNED_BYTE)
        {
            OSG_NOTICE << "[NormalMapGenerator] Only support Vec3ub/Vec4ub pixels, mismatched with "
                       << image->getFileName() << std::endl; return;
        }

        double invPixel = 1.0 / 255.0;
        std::string fileName = osgDB::getSimpleFileName(image->getFileName());
        if (_normalMapUnit > 0)
        {
            osg::ref_ptr<osg::Image> nMap;
            std::string normFile = _cacheFolder + "/" + fileName + ".norm.png";
            if (!_cacheFolder.empty())
            {
                if (osgDB::fileExists(normFile))
                    nMap = osgDB::readImageFile(normFile);
            }

            if (!nMap)
            {
                NormalmapGenerator ng(IntensityMap::AVERAGE, invPixel, invPixel, invPixel, invPixel);
                nMap = ng.calculateNormalmap(image, NormalmapGenerator::PREWITT, _nStrength, _nInvert);
                if (!_cacheFolder.empty()) osgDB::writeImageFile(*nMap, normFile);
            }

            if (nMap.valid() && nMap->valid())
                ss.setTextureAttributeAndModes(_normalMapUnit, createTexture2D(nMap.get()));
        }

        if (_specMapUnit > 0)
        {
            osg::ref_ptr<osg::Image> spMap;
            std::string specFile = _cacheFolder + "/" + fileName + ".spec.png";
            if (!_cacheFolder.empty())
            {
                if (osgDB::fileExists(specFile))
                    spMap = osgDB::readImageFile(specFile);
            }

            if (!spMap)
            {
                SpecularmapGenerator spg(IntensityMap::AVERAGE, invPixel, invPixel, invPixel, invPixel);
                spMap = spg.calculateSpecmap(image, _spScale, _spContrast);
                if (!_cacheFolder.empty()) osgDB::writeImageFile(*spMap, specFile);
            }

            if (spMap.valid() && spMap->valid())
                ss.setTextureAttributeAndModes(_specMapUnit, createTexture2D(spMap.get()));
        }
        OSG_NOTICE << "Normal-map generation for " << fileName << " finished" << std::endl;
    }

    float ActiveNodeVisitor::getDistanceToEyePoint(const osg::Vec3& pos, bool useLODScale) const
    {
        if (useLODScale) return (pos - getEyeLocal()).length() * getLODScale();
        else return (pos - getEyeLocal()).length();
    }

    float ActiveNodeVisitor::getDistanceFromEyePoint(const osg::Vec3& pos, bool useLODScale) const
    {
        if (useLODScale) return (pos - getViewPointLocal()).length() * getLODScale();
        else return (pos - getViewPointLocal()).length();
    }

    float ActiveNodeVisitor::getDistanceToViewPoint(const osg::Vec3& pos, bool useLODScale) const
    {
        const osg::Matrix& matrix = *_modelviewStack.back();
        float dist = -(pos[0] * matrix(0, 2) + pos[1] * matrix(1, 2) + pos[2] * matrix(2, 2) + matrix(3, 2));
        if (useLODScale) return dist * getLODScale(); else return dist;
    }

    void ActiveNodeVisitor::setViewParameters(const osg::Matrix& mvMat, const osg::Matrix& projMat,
                                              osg::Viewport* vp, int refFrame, int refOrder, bool toReset)
    {
        if (toReset) osg::CullStack::reset();
        if (vp) pushViewport(vp);

        osg::RefMatrix* projection = 0;
        osg::RefMatrix* modelview = 0;
        if (refFrame == osg::Transform::RELATIVE_RF)
        {
            if (refOrder == osg::Camera::POST_MULTIPLY)
            {
                projection = createOrReuseMatrix(*getProjectionMatrix() * projMat);
                modelview = createOrReuseMatrix(*getModelViewMatrix() * mvMat);
            }
            else   // pre multiply
            {
                projection = createOrReuseMatrix(projMat * (*getProjectionMatrix()));
                modelview = createOrReuseMatrix(mvMat * (*getModelViewMatrix()));
            }
        }
        else
        {
            projection = createOrReuseMatrix(projMat);
            modelview = createOrReuseMatrix(mvMat);
        }
        pushProjectionMatrix(projection);
        pushModelViewMatrix(modelview, (osg::Transform::ReferenceFrame)refFrame);
    }

    void ActiveNodeVisitor::apply(osg::Camera& node)
    {
        setViewParameters(node.getViewMatrix(), node.getProjectionMatrix(), node.getViewport(),
                          node.getReferenceFrame(), node.getTransformOrder(), false);
        traverse(node);
        if (node.getViewport()) popViewport();
    }

    void Frustum::create(const osg::Matrix& modelview, const osg::Matrix& originProj,
                         double preferredNear, double preferredFar)
    {
        double znear = 0.0, zfar = 0.0, epsilon = 1e-6;
        osg::Matrixd proj = originProj;
        if (preferredNear < preferredFar)
        {
            double zdiff = preferredFar - preferredNear;
            if (fabs(proj(0, 3)) < epsilon  && fabs(proj(1, 3)) < epsilon  && fabs(proj(2, 3)) < epsilon)
            {
                double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
                proj.getOrtho(left, right, bottom, top, znear, zfar);
                if (preferredNear >= 0.0f) znear = preferredNear;
                proj = osg::Matrix::ortho(left, right, bottom, top, znear, znear + zdiff);
            }
            else
            {
                double ratio = 0.0, fovy = 0.0;
                proj.getPerspective(fovy, ratio, znear, zfar);
                if (preferredNear >= 0.0f) znear = preferredNear;
                proj = osg::Matrix::perspective(fovy, ratio, znear, znear + zdiff);
            }
        }

        osg::Matrixd clipToWorld;
        clipToWorld.invert(modelview * proj);
        corners[0].set(-1.0, -1.0, -1.0); corners[1].set(1.0, -1.0, -1.0);
        corners[2].set(1.0, 1.0, -1.0); corners[3].set(-1.0, 1.0, -1.0);
        corners[4].set(-1.0, -1.0, 1.0); corners[5].set(1.0, -1.0, 1.0);
        corners[6].set(1.0, 1.0, 1.0); corners[7].set(-1.0, 1.0, 1.0);
        for (int i = 0; i < 8; ++i) corners[i] = corners[i] * clipToWorld;

        centerNearPlane = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
        centerFarPlane = (corners[4] + corners[5] + corners[6] + corners[7]) * 0.25;
        center = (centerNearPlane + centerFarPlane) * 0.5;
        frustumDir = centerFarPlane - centerNearPlane;
        frustumDir.normalize();
    }

    Frustum::AABB Frustum::createShadowBound(
            const std::vector<osg::Vec3d>& refPoints, const osg::Matrix& worldToLocal)
    {
        osg::BoundingBoxd lightSpaceBB0, lightSpaceBB1;
        for (int i = 0; i < 8; ++i) lightSpaceBB0.expandBy(corners[i] * worldToLocal);
        if (refPoints.empty()) return AABB(lightSpaceBB0._min, lightSpaceBB0._max);

        for (size_t i = 0; i < refPoints.size(); ++i)
            lightSpaceBB1.expandBy(refPoints[i] * worldToLocal);
        if (lightSpaceBB1._min[0] > lightSpaceBB0._min[0]) lightSpaceBB0._min[0] = lightSpaceBB1._min[0];
        if (lightSpaceBB1._min[1] > lightSpaceBB0._min[1]) lightSpaceBB0._min[1] = lightSpaceBB1._min[1];
        if (lightSpaceBB1._min[2] > lightSpaceBB0._min[2]) lightSpaceBB0._min[2] = lightSpaceBB1._min[2];
        if (lightSpaceBB1._max[0] < lightSpaceBB0._max[0]) lightSpaceBB0._max[0] = lightSpaceBB1._max[0];
        if (lightSpaceBB1._max[1] < lightSpaceBB0._max[1]) lightSpaceBB0._max[1] = lightSpaceBB1._max[1];
        if (lightSpaceBB1._max[2] < lightSpaceBB0._max[2]) lightSpaceBB0._max[2] = lightSpaceBB1._max[2];
        return AABB(lightSpaceBB0._min, lightSpaceBB0._max);
    }
    
    bool QuickEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            int btn = ea.getButton(), modkey = ea.getModKeyMask();
            if (_pushCallbacks.find(btn) != _pushCallbacks.end()) _pushCallbacks[btn](btn, modkey);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            int btn = ea.getButton(), modkey = ea.getModKeyMask();
            if (_clickCallbacks.find(btn) != _clickCallbacks.end()) _clickCallbacks[btn](btn, modkey);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK)
        {
            int btn = ea.getButton(), modkey = ea.getModKeyMask();
            if (_dbClickCallbacks.find(btn) != _dbClickCallbacks.end()) _dbClickCallbacks[btn](btn, modkey);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            int key = ea.getKey();
            if (_keyCallbacks0.find(key) != _keyCallbacks0.end()) _keyCallbacks0[key](key);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            int key = ea.getKey();
            if (_keyCallbacks1.find(key) != _keyCallbacks1.end()) _keyCallbacks1[key](key);
        }
        return false;
    }

    /** ConsoleHandler **/
    static std::mutex g_syncout_mutex;
    struct SyncConsoleOut
    {
        std::unique_lock<std::mutex> _lock;
        SyncConsoleOut() : _lock(std::unique_lock<std::mutex>(g_syncout_mutex)) {}
        template<typename T> SyncConsoleOut& operator<<(const T& _t)
        { std::cout << _t; return *this; }
        SyncConsoleOut& operator<<(std::ostream& (*fp)(std::ostream&))
        { std::cout << fp; return *this; }
    };

    ConsoleHandler::ConsoleHandler(bool showStack)
        : _shaderCallback(NULL), _programCallback(NULL), _handle(NULL), _showStackTrace(showStack)
    {
#ifdef VERSE_WINDOWS
        // https://learn.microsoft.com/en-us/windows/console/console-screen-buffers
        _handle = GetStdHandle(STD_OUTPUT_HANDLE);
#else
        // https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
#endif
    }

    void ConsoleHandler::notifyLevel0(osg::NotifySeverity severity, const std::string& message)
    {
        std::string header = message.length() < 5 ? "" : "[FATAL   " + getDateTimeTick() + "] ";
#ifdef VERSE_WINDOWS
        SetConsoleTextAttribute(_handle, FOREGROUND_RED);
        SyncConsoleOut() << header << message;
        SetConsoleTextAttribute(_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(VERSE_WASM)
        SyncConsoleOut() << header << message;
#else
        SyncConsoleOut() << "\033[91m" << header << message << "\033[0m";
#endif
    }

    void ConsoleHandler::notifyLevel1(osg::NotifySeverity severity, const std::string& message)
    {
        std::string header = message.length() < 5 ? "" : "[WARNING " + getDateTimeTick() + "] ";
#ifdef VERSE_WINDOWS
        SetConsoleTextAttribute(_handle, FOREGROUND_RED | FOREGROUND_GREEN);
        SyncConsoleOut() << header << message;
        SetConsoleTextAttribute(_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(VERSE_WASM)
        SyncConsoleOut() << header << message;
#else
        SyncConsoleOut() << "\033[33m" << header << message << "\033[0m";
#endif
    }

    void ConsoleHandler::notifyLevel2(osg::NotifySeverity severity, const std::string& message)
    {
        std::string header = message.length() < 5 ? "" : "[NOTICE  " + getDateTimeTick() + "] ";
#ifdef VERSE_WINDOWS
        SyncConsoleOut() << header << message;
#elif defined(VERSE_WASM)
        SyncConsoleOut() << header << message;
#else
        SyncConsoleOut() << "\033[37m" << header << message << "\033[0m";
#endif
    }

    void ConsoleHandler::notify(osg::NotifySeverity severity, const char* message)
    {
        std::string msg(message);
        if (severity <= osg::NotifySeverity::WARN && _showStackTrace)
            BACKWARD_MESSAGE(msg, 16);

        switch (severity)
        {
        case osg::NotifySeverity::ALWAYS: notifyLevel0(severity, msg); break;
        case osg::NotifySeverity::FATAL: notifyLevel0(severity, msg); break;
        case osg::NotifySeverity::WARN: notifyLevel1(severity, convertInformation(msg)); break;
        case osg::NotifySeverity::NOTICE: notifyLevel2(severity, convertInformation(msg)); break;
        case osg::NotifySeverity::INFO: notifyLevel2(severity, msg); break;
        case osg::NotifySeverity::DEBUG_INFO: notifyLevel3(severity, msg); break;
        case osg::NotifySeverity::DEBUG_FP: notifyLevel3(severity, msg); break;
        default: notifyLevel3(severity, msg); break;
        }
    }

    std::string ConsoleHandler::convertInformation(const std::string& msg)
    {
        size_t pos0 = msg.find("Shader"), pos1 = msg.find("infolog:\n");
        if (pos0 != std::string::npos && pos1 != std::string::npos)
        {
            if (_shaderCallback != NULL)
            {
                GLenum errorShaderType = GL_NONE;
                if (msg.find("FRAGMENT") != std::string::npos) errorShaderType = GL_FRAGMENT_SHADER;
                else if (msg.find("VERTEX") != std::string::npos) errorShaderType = GL_VERTEX_SHADER;
                else if (msg.find("GEOMETRY") != std::string::npos) errorShaderType = GL_GEOMETRY_SHADER;

                size_t nStart = msg.find("\"", pos0); size_t nEnd = msg.find("\"", nStart + 1);
                if (nStart != std::string::npos) return _shaderCallback(
                    errorShaderType, msg.substr(nStart + 1, nEnd - nStart - 1), msg.substr(pos1 + 8));
            }
        }

        pos0 = msg.find("Program");
        if (pos0 != std::string::npos && pos1 != std::string::npos)
        {
            if (_programCallback != NULL)
            {
                size_t nStart = msg.find("\"", pos0); size_t nEnd = msg.find("\"", nStart + 1);
                if (nStart != std::string::npos)
                    return _programCallback(msg.substr(nStart + 1, nEnd - nStart - 1), msg.substr(pos1 + 8));
            }
        }
        return msg;
    }

    std::string ConsoleHandler::getDateTimeTick()
    {
        auto tick = std::chrono::system_clock::now();
        std::time_t posix = std::chrono::system_clock::to_time_t(tick);
        uint64_t millseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(tick.time_since_epoch()).count() -
            std::chrono::duration_cast<std::chrono::seconds>(tick.time_since_epoch()).count() * 1000;

        char buf[20], buf2[5];
        std::tm tp = *std::localtime(&posix);
        std::string dateTime{ buf, std::strftime(buf, sizeof(buf), "%F %T", &tp) };
        snprintf(buf2, 5, ".%03d", (int)millseconds);
        return dateTime + std::string(buf2);
    }
}
