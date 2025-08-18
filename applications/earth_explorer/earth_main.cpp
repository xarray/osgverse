#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/CullFace>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/TileCallback.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include "capture_callback.h"
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#define EARTH_INTERSECTION_MASK 0xf0000000
std::set<std::string> commandList; std::mutex commandMutex;
std::string global_cityToCreate, global_volumeToLoad, global_particleToLoad;
osg::ref_ptr<osg::Texture> finalBuffer0, finalBuffer1, finalBuffer2;
float global_TargetSunAngle = -0.6f;
extern int global_particleIndex;

typedef std::pair<osg::Camera*, osg::Texture*> CameraTexturePair;
extern CameraTexturePair configureEarthAndAtmosphere(osgViewer::View& viewer, osg::Group* root, osg::Node* earth,
                                                     const std::string& mainFolder, int width, int height, bool showIM);
extern osg::Node* configureOcean(osgViewer::View& viewer, osg::Group* root, osg::Texture* sceneMaskTex,
                                 const std::string& mainFolder, int width, int height, unsigned int mask);
extern osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earth,
                                    const std::string& mainFolder, unsigned int mask);
extern osg::Node* configureInternal(osgViewer::View& viewer, osg::Node* earth,
                                    osg::Texture* sceneMaskTex, unsigned int mask);
extern osg::Node* configureVolumeData(osgViewer::View& viewer, osg::Node* earthRoot,
                                      const std::string& mainFolder, unsigned int mask);
extern void configureParticleCloud(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder,
                                   unsigned int mask, bool withGeomShader);
extern void configureUI(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder, int w, int h);

const char* finalVertCode = {
    "VERSE_VS_OUT vec4 texCoord; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* finalFragCode = {
    "uniform sampler2D sceneTexture, oceanTexture, uiTexture;\n"
    "uniform float flipForStreaming; \n"
    "VERSE_FS_IN vec4 texCoord; \n"
    "VERSE_FS_OUT vec4 fragColor;\n"

    "void main() {\n"
    "    vec2 uv = (flipForStreaming > 0.5) ? vec2(texCoord.s, 1.0 - texCoord.t) : texCoord.st; \n"
    "    vec4 sceneColor = VERSE_TEX2D(sceneTexture, uv.st);\n"
    "    vec4 oceanColor = VERSE_TEX2D(oceanTexture, uv.st);\n"
    "    vec4 uiColor = VERSE_TEX2D(uiTexture, uv.st);\n"
    "    fragColor = mix(sceneColor, oceanColor, oceanColor.a); \n"
    "    fragColor = mix(fragColor, uiColor, uiColor.a); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osg::Camera* c, osg::StateSet* ss, osg::EllipsoidModel* em, const std::string& folder)
    :   _rttCamera(c), _mainStateSets(ss), _ellipsoidModel(em),
        _mainFolder(folder), _sunAngle(-0.6f), _pressingKey(0)
    {
        osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
        worldSunDir->set(osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            osg::StateSet* ss = _mainStateSets.get();
            osg::Uniform* cameraToWorld = ss->getOrCreateUniform("cameraToWorld", osg::Uniform::FLOAT_MAT4);
            osg::Uniform* screenToCamera = ss->getOrCreateUniform("screenToCamera", osg::Uniform::FLOAT_MAT4);
            osg::Uniform* worldCameraPos = ss->getOrCreateUniform("worldCameraPos", osg::Uniform::FLOAT_VEC3);
            osg::Uniform* worldCameraLLA = ss->getOrCreateUniform("worldCameraLLA", osg::Uniform::FLOAT_VEC3);

            osg::Camera* mainCamera = view->getCamera();
            if (mainCamera)
            {
                osg::Matrixf invViewMatrix = mainCamera->getInverseViewMatrix();
                cameraToWorld->set(invViewMatrix);
                screenToCamera->set(osg::Matrixf::inverse(mainCamera->getProjectionMatrix()));

                osg::Vec3d worldCam = invViewMatrix.getTrans();
                osg::Vec3d worldLLA = osgVerse::Coordinate::convertECEFtoLLA(worldCam);
                worldCameraPos->set(osg::Vec3(worldCam));
                worldCameraLLA->set(osg::Vec3(worldLLA));
            }

            std::set<std::string> localCommandList;
            commandMutex.lock();
            commandList.swap(localCommandList);
            commandMutex.unlock();
            handleCommands(view, localCommandList);
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) _pressingKey = ea.getKey();
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP) _pressingKey = 0;

        osg::StateSet* ss = _mainStateSets.get();
        if (global_TargetSunAngle < _sunAngle)
        {
            osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
            if (_sunAngle - global_TargetSunAngle < 0.02f) _sunAngle = global_TargetSunAngle; else _sunAngle -= 0.02f;
            worldSunDir->set(osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
        }
        else if (global_TargetSunAngle > _sunAngle)
        {
            osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
            if (global_TargetSunAngle - _sunAngle < 0.02f) _sunAngle = global_TargetSunAngle; else _sunAngle += 0.02f;
            worldSunDir->set(osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
        }

        if (_pressingKey > 0)
        {
            osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
            osg::Uniform* opaqueValue = ss->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT);
            osg::Uniform* opaqueValue2 = ss->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT);

            osg::Vec3 originDir(-1.0f, 0.0f, 0.0f); osg::Vec4 clipPlane; float opaque = 1.0f;
            switch (_pressingKey)
            {
            case osgGA::GUIEventAdapter::KEY_Left:
                _sunAngle -= (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) ?  0.01f : 0.001f;
                worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                global_TargetSunAngle = _sunAngle; break;
            case osgGA::GUIEventAdapter::KEY_Right:
                _sunAngle += (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) ? 0.01f : 0.001f;
                worldSunDir->set(originDir * osg::Matrix::rotate(_sunAngle, osg::Z_AXIS));
                global_TargetSunAngle = _sunAngle; break;
            case '[': opaqueValue->get(opaque); opaqueValue->set(osg::clampAbove(opaque - 0.01f, 0.0f)); break;
            case ']': opaqueValue->get(opaque); opaqueValue->set(osg::clampBelow(opaque + 0.01f, 1.0f)); break;
            case '-': opaqueValue2->set(0.0f); break;
            case '=': opaqueValue2->set(1.0f); break;

            case 'n':
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                ss->getOrCreateUniform("clipPlane1", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                ss->getOrCreateUniform("clipPlane2", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                break;
            case 'm':
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(1.0f, 0.0f, 0.0f, 0.0f));
                ss->getOrCreateUniform("clipPlane1", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 1.0f, 0.0f, 0.0f));
                ss->getOrCreateUniform("clipPlane2", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 0.0f));
                break;
            case ',':
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->get(clipPlane);
                clipPlane = clipPlane * osg::Matrix::rotate(-0.01f, osg::Z_AXIS);
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->set(clipPlane); break;
            case '.':
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->get(clipPlane);
                clipPlane = clipPlane * osg::Matrix::rotate(0.01f, osg::Z_AXIS);
                ss->getOrCreateUniform("clipPlane0", osg::Uniform::FLOAT_VEC4)->set(clipPlane); break;
            }
        }
        return false;
    }

    void handleCommands(osgViewer::View* view, const std::set<std::string>& localCommandList)
    {
        for (std::set<std::string>::const_iterator it = localCommandList.begin();
             it != localCommandList.end(); ++it)
        {
            const std::string& cmdName = *it;
            if (cmdName.find("layers/") != std::string::npos) updateUserLayer(cmdName);
            else if (cmdName.find("vdb/") != std::string::npos) updateVolumeData(cmdName);
            else if (cmdName.find("cities/") != std::string::npos) updateCityData(cmdName);
            else if (cmdName.find("timeline/") != std::string::npos) updateTimelineData(cmdName);
            else if (cmdName.find("button/") != std::string::npos) updateButtonAction(view, cmdName);
        }
    }

    void updateStreetLayer()
    {
        osgVerse::TileManager* mgr = osgVerse::TileManager::instance(); _currentLayer = "street";
        mgr->setLayerPath(osgVerse::TileCallback::USER,
            "mbtiles://" + _mainFolder + "/" + "carto-png-lv9.mbtiles/{z}-{x}-{y}.png");
    }

    void updateCloudLayer()
    {
        osgVerse::TileManager* mgr = osgVerse::TileManager::instance(); _currentLayer = "cloud";
        mgr->setLayerPath(osgVerse::TileCallback::USER,
            "mbtiles://" + _mainFolder + "/" + "openweathermap-png-lv9.mbtiles/{z}-{x}-{y}.png");
    }

    void updateUserLayer(const std::string& name)
    {
        osgVerse::TileManager* mgr = osgVerse::TileManager::instance(); _currentLayer = name;
        if (name.empty()) mgr->setLayerPath(osgVerse::TileCallback::USER, "");
        else mgr->setLayerPath(osgVerse::TileCallback::USER, _mainFolder + "/" + name + "/{z}/{x}/{y}.png");
        _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(1.0f);
    }

    void updateVolumeData(const std::string& name)
    {
        global_volumeToLoad = name;
        _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(0.2f);
        _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->set(0.0f);
    }

    void updateCityData(const std::string& name)
    {
        global_cityToCreate = name;
        _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(1.0f);
        _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->set(1.0f);
    }

    void updateTimelineData(const std::string& name)
    {
        global_particleToLoad = name;
        if (global_particleIndex < 0)
        {
            _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(0.2f);
            _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->set(0.0f);
        }
        else
        {
            _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(1.0f);
            _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->set(1.0f);
        }
    }

    void updateButtonAction(osgViewer::View* view, const std::string& name)
    {
        if (name.find("table") != std::string::npos)
        {
            if (global_particleIndex >= 0) global_particleToLoad = "next";
        }
        else if (name.find("layers") != std::string::npos)
        {
            if (_currentLayer == "") updateStreetLayer();
            //else if (_currentLayer == "street") updateCloudLayer();
            else if (_currentLayer == "street") updateUserLayer("");
            else updateUserLayer("");
        }
        else if (name.find("show_globe") != std::string::npos)
        {
            float v; _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->get(v);
            _mainStateSets->getOrCreateUniform("globalOpaque", osg::Uniform::FLOAT)->set(v > 0.5f ? 0.0f : 1.0f);
        }
        else if (name.find("show_ocean") != std::string::npos)
        {
            float v; _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->get(v);
            _mainStateSets->getOrCreateUniform("oceanOpaque", osg::Uniform::FLOAT)->set(v > 0.5f ? 0.0f : 1.0f);
        }
        else if (name.find("home") != std::string::npos)
        {
            osgVerse::EarthManipulator* manipulator =
                static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
            if (manipulator) manipulator->home(0.0f);
        }
    }

protected:
    osg::observer_ptr<osg::StateSet> _mainStateSets;
    osg::EllipsoidModel* _ellipsoidModel;
    osg::Camera* _rttCamera;
    std::string _mainFolder, _currentLayer;
    float _sunAngle;
    int _pressingKey;
};

static std::string createCustomPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type >= osgVerse::TileCallback::USER &&
        prefix.find("mbtiles") == std::string::npos)  // FIXME: for Zhijiang data...
    {
        int newY = pow(2, z) - y - 1;
        return osgVerse::TileCallback::createPath(prefix, x, newY, z);
    }
    /*else if (type >= osgVerse::TileCallback::USER)
    {
        int newY = pow(2, z) - y - 1;
        if (z > 9 && prefix.find("carto-png") != std::string::npos)
        {
            std::string prefix2 = "https://a.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png";
            return osgVerse::TileCallback::createPath(prefix2, x, newY, z);
        }
    }*/
    else if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
        if (z > 13)
        {
            std::string prefix2 = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
        else return osgVerse::TileCallback::createPath(prefix, x, y, z);
    }
    return osgVerse::TileCallback::createPath(prefix, x, y, z);
}

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler(false));
    osgVerse::updateOsgBinaryWrappers();
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");

    std::string mainFolder = "F:"; arguments.read("--folder", mainFolder);
    std::string skirtRatio = "0.05"; arguments.read("--skirt", skirtRatio);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);
    bool withGeomShader = true; if (arguments.read("--no-geometry-shader")) withGeomShader = false;

    // Create earth
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/satellite-2017-jpg-z13.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/elevation-google-tif-z8.mbtiles/{z}-{x}-{y}.tif"
                            " OceanMask=mbtiles://" + mainFolder + "/aspect-slope-tif-z8.mbtiles/{z}-{x}-{y}.tif"
                            //" Orthophoto=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z}"
                            /*" MaximumLevel=8"*/" UseWebMercator=1 UseEarth3D=1 OriginBottomLeft=1"
                            " TileElevationScale=3 TileSkirtRatio=" + skirtRatio;
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);
    earthOptions->setPluginData("UrlPathFunction", (void*)createCustomPath);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    if (!earth) { OSG_FATAL << "Main earth scene is missing!\n"; return 1; }
    earth->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    earth->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    earth->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));

    // Create manipulator
    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(true);
    viewer.setCameraManipulator(earthManipulator.get());

    osg::Vec3d pos = osgVerse::Coordinate::convertLLAtoECEF(
        osg::Vec3d(osg::inDegrees(0.0), osg::inDegrees(120.0), 10000.0));
    earthManipulator->moveTo(pos, 0.0, 120.0);

    // Create the scene graph
    osg::ref_ptr<osg::Group> root = new osg::Group;
    CameraTexturePair camTexPair = configureEarthAndAtmosphere(
        viewer, root.get(), earth.get(), mainFolder, w, h, arguments.read("--adjuster"));

    osg::ref_ptr<osg::Camera> sceneCamera = camTexPair.first;
    osg::ref_ptr<osg::Texture> sceneTexture = camTexPair.second;
    sceneCamera->addChild(configureCityData(viewer, earth.get(), mainFolder, ~EARTH_INTERSECTION_MASK));
    sceneCamera->addChild(configureVolumeData(viewer, earth.get(), mainFolder, ~EARTH_INTERSECTION_MASK));
    sceneCamera->addChild(configureInternal(viewer, earth.get(), sceneTexture.get(), ~EARTH_INTERSECTION_MASK));
    configureOcean(viewer, root.get(), sceneTexture.get(), mainFolder, w, h, ~EARTH_INTERSECTION_MASK);
    configureParticleCloud(viewer, sceneCamera.get(), mainFolder, ~EARTH_INTERSECTION_MASK, withGeomShader);
    configureUI(viewer, root.get(), mainFolder, w, h);

    osg::StateSet* ss = root->getOrCreateStateSet();
    osg::ref_ptr<osg::Uniform> clip0 = new osg::Uniform("clipPlane0", osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    osg::ref_ptr<osg::Uniform> clip1 = new osg::Uniform("clipPlane1", osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    osg::ref_ptr<osg::Uniform> clip2 = new osg::Uniform("clipPlane2", osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    ss->addUniform(clip0.get()); ss->addUniform(clip1.get()); ss->addUniform(clip2.get());

    // Final HUD for scene rendering and streaming
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, finalVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, finalFragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Final_VS"); program->addShader(vs);
    fs->setName("Final_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME

#if 1
    osg::Camera* finalCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0f, 1.0f, true);
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, finalBuffer0.get());
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, finalBuffer1.get());
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(2, finalBuffer2.get());
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("sceneTexture", (int)0));
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("oceanTexture", (int)1));
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("uiTexture", (int)2));
    finalCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
    root->addChild(finalCamera);

    std::string streamURL = "rtmp://127.0.0.1:1935/live/stream";
    if (arguments.read("--streaming") || arguments.read("--streaming-url", streamURL))
    {
        osgDB::Registry::instance()->loadLibrary(
            osgDB::Registry::instance()->createLibraryNameForExtension("verse_ms"));
        viewer.getCamera()->setFinalDrawCallback(new CaptureCallback(streamURL, mainFolder, w, h));
        finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("flipForStreaming", 1.0f));
    }
    else
        finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("flipForStreaming", 0.0f));
#endif

    // Realize the viewer
    osg::ref_ptr<osgVerse::EarthProjectionMatrixCallback> epmcb =
        new osgVerse::EarthProjectionMatrixCallback(viewer.getCamera(), earth->getBound().center());
    epmcb->setNearFirstModeThreshold(2000.0);
    sceneCamera->setClampProjectionMatrixCallback(epmcb.get());

    viewer.addEventHandler(new EnvironmentHandler(
        sceneCamera.get(), root->getOrCreateStateSet(), earthManipulator->getEllipsoid(), mainFolder));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // Start the main loop
    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
