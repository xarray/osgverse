#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/TileCallback.h>
#include <pipeline/IncrementalCompiler.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef false  // no use and will be removed later
static const char* resizeSrc = {
    "#version 430 core \n"
    "layout (local_size_x = 16, local_size_y = 16) in; \n"
    "layout (binding = 0) uniform sampler2D sourceTexture; \n"
    "layout (binding = 1, rgba8) writeonly uniform image2D targetTexture; \n"
    "void main() { \n"
    "    ivec2 targetCoord = ivec2(gl_GlobalInvocationID.xy); \n"
    "    vec2 sourceCoord = vec2(targetCoord) * 2.0 / vec2(textureSize(sourceTexture, 0)); \n"
    "    vec4 color = texture(sourceTexture, sourceCoord); \n"
    "    imageStore(targetTexture, targetCoord, color.bbba); \n"
    "} \n"
};

class UserIncrementalCompileCallback : public osgVerse::IncrementalCompileCallback
{
public:
    UserIncrementalCompileCallback() : osgVerse::IncrementalCompileCallback()
    {
        osg::ref_ptr<osg::Program> resizeProgram = new osg::Program;
        resizeProgram->addShader(new osg::Shader(osg::Shader::COMPUTE, resizeSrc));

        _compute = new osg::DispatchCompute;
        _compute->setDataVariance(osg::Object::DYNAMIC);
        _compute->getOrCreateStateSet()->setAttributeAndModes(resizeProgram.get());
        _compute->getOrCreateStateSet()->addUniform(new osg::Uniform("sourceTexture", (int)0));
        _compute->getOrCreateStateSet()->addUniform(new osg::Uniform("targetTexture", (int)1));
    }

    virtual bool compile(ICO::CompileTextureOp* op, ICO::CompileInfo& compileInfo)
    {
        osg::State* state = compileInfo.getState();
        if (glCopyImageSubData == NULL) osg::setGLExtensionFuncPtr(
            glCopyImageSubData, "glCopyImageSubData", "glCopyImageSubDataEXT", "glCopyImageSubDataNV", true);

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        if (op->_texture->getImage(0))
        {
            osg::Image* img = op->_texture->getImage(0);
            tex2D->setTextureSize(img->s() / 2, img->t() / 2);
        }
        else
            tex2D->setTextureSize(op->_texture->getTextureWidth() / 2, op->_texture->getTextureHeight() / 2);
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setInternalFormat(GL_RGBA8); tex2D->setSourceFormat(GL_RGBA);
        tex2D->setSourceType(GL_UNSIGNED_BYTE);
        if (tex2D->getTextureWidth() == 0 || tex2D->getTextureHeight() == 0)
            return osgVerse::IncrementalCompileCallback::compile(op, compileInfo);

        osg::ref_ptr<osg::Texture::TextureObject> tobj =
            osg::Texture::generateTextureObject(tex2D.get(), state->getContextID(), GL_TEXTURE_2D);
        tex2D->setTextureObject(state->getContextID(), tobj.get());

        osg::ref_ptr<osg::BindImageTexture> binding = new osg::BindImageTexture(
            1, tex2D.get(), osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
        state->apply(_compute->getStateSet());
        state->applyTextureMode(0, op->_texture->getTextureTarget(), true);
        state->applyTextureAttribute(0, op->_texture.get());
        state->applyTextureMode(1, op->_texture->getTextureTarget(), true);
        state->applyTextureAttribute(1, tex2D.get());
        state->applyAttribute(binding.get());
        _compute->setComputeGroups(
            (tex2D->getTextureWidth() + 15) / 16, (tex2D->getTextureHeight() + 15) / 16, 1);
        _compute->draw(compileInfo); return true;
    }

protected:
    osg::ref_ptr<osg::DispatchCompute> _compute;
};
#endif

const char* finalVertCode = {
    "VERSE_VS_OUT vec4 texCoord; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* finalFragCode = {
    "uniform sampler2D EarthTexture, OceanTexture;\n"
    "VERSE_FS_IN vec4 texCoord; \n"
    "VERSE_FS_OUT vec4 fragColor;\n"

    "void main() {\n"
    "    vec4 sceneColor = VERSE_TEX2D(EarthTexture, texCoord.st);\n"
    "    vec4 oceanColor = VERSE_TEX2D(OceanTexture, texCoord.st);\n"
    "    fragColor = mix(sceneColor, oceanColor, oceanColor.a); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c)
{
    size_t levelPos = src.find(match); if (levelPos == std::string::npos) { c = false; return src; }
    src.replace(levelPos, match.length(), v); c = true; return src;
}

static std::string createTengxunPath(int type, const std::string& prefix, int x, int y, int z)
{
    std::string path = prefix; bool changed = false;
    path = replace(path, "{x16}", std::to_string(x / 16), changed);
    path = replace(path, "{y16}", std::to_string(y / 16), changed);
    y = (int)pow((double)z, 2.0) - y - 1; return path;
}

static std::string createMixedPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
        if (z > 4)
        {
            std::string prefix2 = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
    }
    return osgVerse::TileCallback::createPath(prefix, x, y, z);
}

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osgVerse::EarthAtmosphereOcean* e, bool b)
        : _earthData(e), _withOcean(b) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            _earthData->update(view->getCamera());
            if (_withOcean) _earthData->updateOcean(view->getCamera());
        }
        return false;
    }

protected:
    osgVerse::EarthAtmosphereOcean* _earthData;
    bool _withOcean;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");
    osgVerse::updateOsgBinaryWrappers();

    bool useOcean = arguments.read("--ocean"), useSky = arguments.read("--sky"), use2D = arguments.read("--map2d");
    std::string use2Dor3D = use2D ? "UseEarth3D=0" : "UseEarth3D=1";
#if false
    std::string earthURLs = "Orthophoto=G:/DOM_DEM/dom/{z}/{x}/{y}.jpg OriginBottomLeft=1 "
                            "Elevation=F:/DEM-China-wgs84-Mesh-12.5m/{z}/{x}/{y}.terrain ";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " " + use2Dor3D);
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-x.verse_tms", earthOptions.get());
#elif false
    std::string earthURLs = "Orthophoto=mbtiles://F:/satellite-2017-jpg-z13.mbtiles/{z}-{x}-{y}.jpg OriginBottomLeft=1 "
                            "Elevation=mbtiles://F:/elevation-google-tif-z8.mbtiles/{z}-{x}-{y}.tif UseWebMercator=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " " + use2Dor3D);
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
#elif false
    std::string earthURLs = "Orthophoto=https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z} "
                            "Elevation=https://mt1.google.com/vt/lyrs%3dt&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " " + use2Dor3D);
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
#elif false
    std::string earthURLs = "Orthophoto=http://p0.map.gtimg.com/sateTiles/{z}/{x16}/{y16}/{x}_{y}.jpg UseWebMercator=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " " + use2Dor3D);
    earthOptions->setPluginData("UrlPathFunction", (void*)createTengxunPath);
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
#else
    std::string earthURLs = "Orthophoto=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs + " " + use2Dor3D);
    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
#endif

    osg::ref_ptr<osg::Node> tiles = osgDB::readNodeFiles(arguments, new osgDB::Options("DisabledPBR=1"));
    if (useOcean || useSky)
    {
        std::string mainFolder = BASE_DIR + "/models/Earth"; arguments.read("--folder", mainFolder);
        earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                    " Elevation=mbtiles://" + mainFolder + "/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                    " OceanMask=mbtiles://" + mainFolder + "/Mask_lv3.mbtiles/{z}-{x}-{y}.tif"
                    " UseWebMercator=1 UseEarth3D=1 OriginBottomLeft=1 TileElevationScale=3 TileSkirtRatio=0.05";
        earthOptions = new osgDB::Options(earthURLs);
        earthOptions->setPluginData("UrlPathFunction", (void*)createMixedPath);
        earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    }
    if (!earth) return 1;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    // Create sky and ocean
    osgVerse::EarthAtmosphereOcean earthRenderingUtils;
    if (useOcean || useSky)
    {
        int w = 1920, h = 1080; arguments.read("--resolution", w, h);
        if (!use2D)
        {
            // Initialize earth utilities
            earthRenderingUtils.create(BASE_DIR + "/textures/transmittance.raw",
                                       BASE_DIR + "/textures/irradiance.raw",
                                       BASE_DIR + "/textures/sunglare.png",
                                       BASE_DIR + "/textures/inscatter.raw");

            // Add earth globe to RTT camera
            osg::ref_ptr<osg::Texture> earthColorBuffer =
                osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
            osg::ref_ptr<osg::Texture> earthMaskBuffer =
                osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);

            osg::Camera* earthCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
            earthCamera->attach(osg::Camera::COLOR_BUFFER0, earthColorBuffer.get(), 0, 0, false, 16, 4);
            earthCamera->attach(osg::Camera::COLOR_BUFFER1, earthMaskBuffer.get());
            earthCamera->setViewport(0, 0, w, h);
            if (tiles.valid()) earthCamera->addChild(tiles.get());
            earthCamera->addChild(earth.get());

            osg::Texture* defTex0 = osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            osg::Texture* defTex1 = osgVerse::createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
            earthRenderingUtils.applyToGlobe(earthCamera->getOrCreateStateSet(), defTex0, defTex0, defTex1,
                    osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_globe.vert.glsl"),
                    osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_globe.frag.glsl"));
            root->addChild(earthCamera);

            // Merge atmosphere with earth color and render them on a screen quad
            osg::ref_ptr<osg::Texture> mergedGlobeBuffer = earthColorBuffer;
            if (useSky)
            {
                mergedGlobeBuffer = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
                osg::Camera* skyAndGlobeCamera = osgVerse::createRTTCamera(
                    osg::Camera::COLOR_BUFFER0, mergedGlobeBuffer.get(), NULL, true);
                earthRenderingUtils.applyToAtmosphere(skyAndGlobeCamera->getOrCreateStateSet(), earthColorBuffer.get(),
                        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_sky.vert.glsl"),
                        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_sky.frag.glsl"));
                root->addChild(skyAndGlobeCamera);
            }

            // Add post-processing ocean camera if necessary
            osg::ref_ptr<osg::Texture> oceanColorBuffer = defTex1;
            if (useOcean)
            {
                osg::Geometry* grid = earthRenderingUtils.createOceanGrid(w, h);
                osg::Geode* grideGeode = new osg::Geode; grideGeode->addDrawable(grid);

                oceanColorBuffer = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
                osg::Camera* oceanCamera = osgVerse::createRTTCamera(
                    osg::Camera::COLOR_BUFFER0, oceanColorBuffer.get(), NULL, false);
                oceanCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
                oceanCamera->setProjectionMatrix(osg::Matrix::ortho2D(-1.0, 1.0, -1.0, 1.0));
                oceanCamera->setViewMatrix(osg::Matrix::identity());
                oceanCamera->addChild(grideGeode);

                float seaRoughness = 0.0f;
                earthRenderingUtils.applyToOcean(oceanCamera->getOrCreateStateSet(),
                        earthMaskBuffer.get(), earthRenderingUtils.createOceanWaves(seaRoughness),
                        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "global_ocean.vert.glsl"),
                        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "global_ocean.frag.glsl"));
                root->addChild(oceanCamera);
            }

            // Merge results to an HUD camera to show
            osg::Camera* finalCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0f, 1.0f, true);
            finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, mergedGlobeBuffer.get());
            finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, oceanColorBuffer.get());
            finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("EarthTexture", (int)0));
            finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("OceanTexture", (int)1));
            root->addChild(finalCamera);

            osg::ref_ptr<osg::Program> program = new osg::Program;
            {
                osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, finalVertCode);
                osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, finalFragCode);
                program->addShader(vs); program->addShader(fs);
                osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
                osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME
            }
            finalCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
        }
        else
        { OSG_NOTICE << "Ocean and sky rendering effects are incompatible with 2D maps\n"; return 1; }
    }
    else
        root->addChild(earth.get());

    // Create camera manipulator
    osg::ref_ptr<osgGA::CameraManipulator> camManipulator;
    if (use2D)
        camManipulator = new osgGA::TrackballManipulator;
    else
    {
        osg::ref_ptr<osgVerse::EarthManipulator> manipulator = new osgVerse::EarthManipulator;
        manipulator->setWorldNode(earth.get()); camManipulator = manipulator;
        if (tiles.valid())
        {
            osg::Vec3 dir = tiles->getBound().center(); dir.normalize();
            osg::ref_ptr<osg::MatrixTransform> tileMT = new osg::MatrixTransform;
            //tileMT->setMatrix(osg::Matrix::translate(-dir * 380.0f));
            tileMT->addChild(tiles.get());

            osg::ref_ptr<osg::EllipsoidModel> em = new osg::EllipsoidModel;
            osg::Vec3d ecef = tiles->getBound().center(), newEcef, lla;
            em->convertXYZToLatLongHeight(ecef[0], ecef[1], ecef[2], lla[0], lla[1], lla[2]);

            osg::Vec3d newLLA = osgVerse::Coordinate::convertWGS84toGCJ02(lla);
            std::cout << "WGS84: " << osg::RadiansToDegrees(lla[1]) << ", " << osg::RadiansToDegrees(lla[0]) << "; "
                      << "GCJ02: " << osg::RadiansToDegrees(newLLA[1]) << ", " << osg::RadiansToDegrees(newLLA[0]) << std::endl;
            em->convertLatLongHeightToXYZ(newLLA[0], newLLA[1], newLLA[2], newEcef[0], newEcef[1], newEcef[2]);

            osg::ref_ptr<osg::MatrixTransform> tileOffset = new osg::MatrixTransform;
            tileOffset->setMatrix(osg::Matrix::translate(newEcef - ecef));
            tileOffset->addChild(tileMT.get());
            root->addChild(tileOffset.get());

            osg::BoundingSphere bs = tiles->getBound(); double r = bs.radius() * 10.0;
            manipulator->osgGA::CameraManipulator::setHomePosition(bs.center() + osg::Z_AXIS * r, bs.center(), osg::Y_AXIS);
        }
    }

    //osg::ref_ptr<osgVerse::IncrementalCompiler> incrementalCompiler = new osgVerse::IncrementalCompiler;
    //incrementalCompiler->setCompileCallback(new UserIncrementalCompileCallback);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setNearFarRatio(0.00001);
    //viewer.setIncrementalCompileOperation(incrementalCompiler.get());
    if (useOcean || useSky) viewer.addEventHandler(new EnvironmentHandler(&earthRenderingUtils, useOcean));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(camManipulator.get());
    viewer.setSceneData(root.get());
    viewer.run();
    //osgDB::writeNodeFile(*earth, "../earth.osg");
    return 0;
}
