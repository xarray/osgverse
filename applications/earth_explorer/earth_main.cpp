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
#include <readerwriter/FileCache.h>
#include <pipeline/Pipeline.h>
#include <ui/ImGui.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

#include "EarthAppConfig.h"
#include "DataSource.h"
#include "LayerRegistry.h"
#include "ai_setup.h"
#include "earth_ui.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#define EARTH_INTERSECTION_MASK 0xf0000000

extern std::vector<osg::Camera*> configureEarthRendering(
        osgViewer::View& viewer, osg::Group* root, osg::Node* earth, osgVerse::EarthAtmosphereOcean& eData,
        const std::string& mainFolder, unsigned int mask, int w, int h);
extern osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earthRoot,
                                    osgVerse::EarthAtmosphereOcean& earthRenderingUtils,
                                    const std::string& mainFolder, unsigned int mask, bool waitingMode);

// Fallback sphere when the tile source is completely unavailable: guarantees
// "a globe is visible no matter what" at startup
static osg::Node* createFallbackGlobe()
{
    osg::ref_ptr<osg::Sphere> shape = new osg::Sphere(osg::Vec3(), 6371000.0f);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(shape.get());
    drawable->setColor(osg::Vec4(0.18f, 0.32f, 0.55f, 1.0f));
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(drawable.get());
    return geode.release();
}

// Keyboard: ',' '.' adjust the sun; 'o' records an animation path frame /
// 'i' saves the path / 'p' loads and plays the path
class KeyHandler : public osgGA::GUIEventHandler
{
public:
    KeyHandler(EnvironmentState* env) : _env(env), _pressingKey(0), _pathIndex(0) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) _pressingKey = ea.getKey();
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            osgVerse::EarthManipulator* earthMani =
                dynamic_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
            if (earthMani)
            {
                if (ea.getKey() == 'o')
                { earthMani->insertControlPointFromCurrentView((float)_pathIndex); _pathIndex += 60; }
                else if (ea.getKey() == 'i')
                {
                    std::ofstream out("city_path.txt", std::ios::out);
                    const osgVerse::EarthManipulator::ControlPointSet& path = earthMani->getControlPoints();
                    for (osgVerse::EarthManipulator::ControlPointSet::const_iterator it = path.begin();
                         it != path.end(); ++it)
                    {
                        osgVerse::EarthManipulator::ControlPoint* cp = (*it).get();
                        out << cp->_time << "," << cp->_rotation << "," << cp->_tilt
                            << "," << cp->_distance << "\n";
                    }
                }
                else if (ea.getKey() == 'p')
                {
                    std::ifstream in("city_path.txt", std::ios::in); std::string line;
                    if (in)
                    {
                        earthMani->clearControlPoints();
                        while (std::getline(in, line))
                        {
                            if (line.empty() || line[0] == '#') continue;
                            std::vector<std::string> values; osgDB::split(line, values, ',');
                            if (values.size() < 4) continue;
                            osg::Quat q; std::stringstream ss(values[1]); ss >> q;
                            earthMani->insertControlPoint(new osgVerse::EarthManipulator::ControlPoint(
                                atof(values[0].c_str()), q, atof(values[3].c_str()),
                                atof(values[2].c_str())));
                        }
                        if (earthMani->isAnimationRunning()) earthMani->stopAnimation();
                        earthMani->startAnimation();
                    }
                }
            }
            _pressingKey = 0;
        }

        if (_pressingKey > 0 && _env)
        {
            if (_pressingKey == ',' || _pressingKey == '<')
                _env->sunAngle = _env->sunAngle - 0.0005f;
            else if (_pressingKey == '.' || _pressingKey == '>')
                _env->sunAngle = _env->sunAngle + 0.0005f;
        }
        return false;
    }

protected:
    EnvironmentState* _env;
    int _pressingKey, _pathIndex;
};

// Headless-verification helper: --shot <file> grabs the main framebuffer on the
// next frame and saves it as PNG (screen capture is unreliable in a
// headless/detached session, so read the framebuffer directly); --auto-exit
// quits automatically after the capture
class FrameShotCallback : public osg::Camera::DrawCallback
{
public:
    FrameShotCallback(const std::string& file, int framesAfterAttach)
    :   _file(file), _target(framesAfterAttach), _count(0), _done(false) {}

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        if (_done) return;
        if (++_count < _target) return;
        _done = true;
        osg::Camera* cam = renderInfo.getCurrentCamera();
        osg::Viewport* vp = cam ? cam->getViewport() : NULL;
        if (!vp) return;
        osg::ref_ptr<osg::Image> img = new osg::Image;
        img->readPixels((int)vp->x(), (int)vp->y(), (int)vp->width(), (int)vp->height(),
                        GL_RGB, GL_UNSIGNED_BYTE);
        // Writing the file must leave the GL thread: writeImageFile may trigger
        // plugin loading, and concurrent access to the Registry from the main
        // thread / DatabasePager would race and crash
        std::thread([img, file = _file]() { osgDB::writeImageFile(*img, file); }).detach();
        OSG_NOTICE << "[EarthExplorer] frame capture requested for " << _file << "\n";
    }

protected:
    std::string _file;
    int _target;
    mutable int _count;
    mutable bool _done;
};

class AutoExitHandler : public osgGA::GUIEventHandler
{
public:
    AutoExitHandler(int framesAfterStart) : _target(framesAfterStart), _frames(0) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() != osgGA::GUIEventAdapter::FRAME) return false;
        if (++_frames >= _target)
        {
            osgViewer::Viewer* v = dynamic_cast<osgViewer::Viewer*>(&aa);
            if (v) v->setDone(true);
        }
        return false;
    }

protected:
    int _target, _frames;
};

// Central per-frame drain on the main thread:
// 1) pending LayerRegistry work (layer toggles submitted by UI/AI actually take
//    effect here -- the apply callback touches the scene graph);
// 2) the AIChatCore agent loop (executes the model's queued tool calls, advances
//    to the next round).
// Both tool execution and scene graph updates require the main thread and must
// never be placed on the draw/worker thread.
class MainThreadDrainHandler : public osgGA::GUIEventHandler
{
public:
    MainThreadDrainHandler(LayerRegistry* layers, earthai::AIChatCore* aiChat)
    :   _layers(layers), _aiChat(aiChat) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() != osgGA::GUIEventAdapter::FRAME) return false;
        if (_layers) _layers->drainPending();
        if (_aiChat) _aiChat->drainMainThread();
        return false;
    }

protected:
    LayerRegistry* _layers;
    earthai::AIChatCore* _aiChat;
};

// Headless e2e verification hook: --ai-submit <text> automatically submits a user
// message on frame N, --ai-transcript <file> writes the session transcript to disk
// once the AI is idle (final text/error settled) and quits. A real API round takes
// 10~30s, so we cannot wait a fixed number of frames; wait for busy to clear and
// buffer a few frames, with a 240s timeout as a backstop.
class AIAutoSubmitHandler : public osgGA::GUIEventHandler
{
public:
    AIAutoSubmitHandler(earthai::AIChatCore* core, const std::string& text,
                        const std::string& transcriptFile)
    :   _core(core), _text(text), _transcriptFile(transcriptFile)
    ,   _frames(0), _submitted(false), _dumped(false), _submitFrame(0) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() != osgGA::GUIEventAdapter::FRAME || !_core) return false;
        ++_frames;
        if (!_submitted && _frames >= 90)   // wait for the manipulator/layers to be ready
        {
            _core->submit(_text);
            _submitted = true;
            _submitFrame = _frames;
            OSG_NOTICE << "[EarthAI] auto-submit: " << _text << "\n";
        }
        if (_submitted && !_dumped && !_transcriptFile.empty())
        {
            // busy is set by submit and cleared by drainMainThread on the final text/error
            bool idle = !_core->busy() && (_frames > _submitFrame + 60);
            bool timeout = (_frames > _submitFrame + 14400);   // 240s @60fps
            if (idle || timeout)
            {
                std::ofstream out(_transcriptFile.c_str(), std::ios::out);
                std::vector<earthai::ChatEntry> entries = _core->transcript();
                for (size_t i = 0; i < entries.size(); ++i)
                {
                    const char* tag = (entries[i].kind == earthai::ChatEntry::USER) ? "USER"
                        : (entries[i].kind == earthai::ChatEntry::ASSISTANT) ? "AI"
                        : (entries[i].kind == earthai::ChatEntry::TOOL_NOTE) ? "NOTE" : "ERR";
                    out << "[" << tag << "] " << entries[i].text << "\n";
                }
                _dumped = true;
                OSG_NOTICE << "[EarthAI] transcript dumped: " << _transcriptFile
                          << (timeout ? " (timeout)" : "") << "\n";
                osgViewer::Viewer* v = dynamic_cast<osgViewer::Viewer*>(&aa);
                if (v) v->setDone(true);
            }
        }
        return false;
    }

protected:
    earthai::AIChatCore* _core;
    std::string _text, _transcriptFile;
    int _frames, _submitFrame;
    bool _submitted, _dumped;
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler(false));
    osgVerse::updateOsgBinaryWrappers();
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");

    // ---- Unified config: external API / tile source / data sources all come from earth_explorer.json ----
    earthee::AppConfig cfg = earthee::loadAppConfig(arguments);

    std::string mainFolder = BASE_DIR + "/models"; arguments.read("--folder", mainFolder);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);
    bool cityWaitingTiles = true, manipulatorCanThrow = false;
    if (arguments.read("--no-wait")) cityWaitingTiles = false;
    if (arguments.read("--thrown")) manipulatorCanThrow = true;

    // ---- Earth tiles (all URL templates come from config; the default is an online public source) ----
    std::ostringstream earthURLs;
    earthURLs << " Orthophoto=" << cfg.orthophotoTemplate;
    if (!cfg.elevationTemplate.empty())
        earthURLs << " Elevation=" << cfg.elevationTemplate;
    if (!cfg.oceanMaskTemplate.empty())
        earthURLs << " OceanMask=" << cfg.oceanMaskTemplate;
    earthURLs << " UseWebMercator=" << (cfg.useWebMercator ? 1 : 0)
              << " UseEarth3D=1 OriginBottomLeft=" << (cfg.originBottomLeft ? 1 : 0)
              << " TileElevationScale=" << cfg.elevationScale
              << " TileSkirtRatio=" << cfg.skirtRatio;
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs.str());

    // ---- Attach the cache early: startup is nearly instant when the root tile hits the local cache ----
    osg::ref_ptr<osgVerse::FileCache> fileCache = new osgVerse::FileCache("earth_cache");
    osgDB::Registry::instance()->setFileCache(fileCache.get());

    // ---- View / UI first: the window and ImGui appear immediately without waiting for the network ----
    EnvironmentState envState;
    earthds::registerBuiltinSourceTypes();
    LayerRegistry layerRegistry;
    earthds::DataSourceManager dataSourceManager;

    // ---- AI assistant: instantiate the provider by config (factory pattern); a
    //      missing key/config only disables it and never affects the earth or the
    //      rest of the UI. Ownership stays on the main stack and is released explicitly on exit ----
    earthai::ToolRegistry aiToolRegistry;
    earthai::LLMProvider* aiProvider = NULL;   // AIChatCore does not own the provider
    earthai::AIChatCore* aiCore = NULL;
    std::string aiProviderName;
    if (cfg.ai.enabled && !cfg.ai.providers.empty())
    {
        const earthee::AIProviderConfig* activePC = NULL;
        for (size_t i = 0; i < cfg.ai.providers.size(); ++i)
        {
            if (cfg.ai.providers[i].name == cfg.ai.active)
            { activePC = &cfg.ai.providers[i]; break; }
        }
        if (!activePC) activePC = &cfg.ai.providers[0];  // unspecified/not found -> first one

        earthai::AIProviderSetup setup;
        setup.name = activePC->name;
        setup.type = activePC->type;
        setup.baseUrl = activePC->baseUrl;
        setup.model = activePC->model;
        setup.apiKey = earthee::resolveAIKey(cfg, *activePC);
        setup.scriptFile = activePC->scriptFile;
        setup.timeoutSeconds = activePC->timeoutSeconds;

        aiProvider = earthai::createLLMProvider(setup);
        if (aiProvider)
        {
            aiCore = new earthai::AIChatCore(aiProvider, &aiToolRegistry);
            aiCore->setMaxRounds(cfg.ai.maxRounds);
            aiCore->setSystemPrompt(cfg.ai.systemPrompt.empty()
                                    ? earthai::defaultEarthSystemPrompt()
                                    : cfg.ai.systemPrompt);
            aiProviderName = aiProvider->describe();
            OSG_NOTICE << "[EarthAI] AI assistant enabled: " << aiProviderName << "\n";
        }
        else
        {
            OSG_WARN << "[EarthAI] AI provider unavailable (missing key or wrong type), "
                        "the chat panel will show its disabled state\n";
        }
    }
    else if (cfg.ai.enabled)
    {
        OSG_WARN << "[EarthAI] no usable AI provider configured, "
                    "the chat panel will show its disabled state\n";
    }

    osg::ref_ptr<EarthUIHandler> uiHandler = new EarthUIHandler(
        &layerRegistry, &dataSourceManager, &envState, &cfg);
    uiHandler->setViewer(&viewer);
    uiHandler->setLoadingEarth(true);
    uiHandler->setAIChat(aiCore, aiProviderName);

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    // The interface is English, so ImGui's built-in ASCII font is enough. A
    // custom font can still be provided via the config ui.font field.
    if (!cfg.fontPath.empty() && osgDB::fileExists(cfg.fontPath))
        imgui->setChineseSimplifiedFont(cfg.fontPath);
    else
        OSG_NOTICE << "[EarthExplorer] using the built-in ImGui font"
                      " (set ui.font in the config to use a custom font)\n";
    imgui->initialize(uiHandler.get());
    // Note: ImGui is attached to the final composite HUD camera further below
    // (once the render pipeline exists); attaching it to the main camera here
    // would let the earth/sky composite paint over the UI.

    osgDB::DatabasePager* pager = new osgDB::DatabasePager;
    pager->setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS);
    pager->setDoPreCompile(true); pager->setUpThreads(6, 3);

    viewer.addEventHandler(new KeyHandler(&envState));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    viewer.setDatabasePager(pager);

    // ---- Headless verification: --shot <file> [--auto-exit] (callback attached once the earth is ready) ----
    std::string shotFile;
    bool autoExit = false;
    arguments.read("--shot", shotFile);
    if (arguments.read("--auto-exit")) autoExit = true;

    osg::ref_ptr<osg::Group> root = new osg::Group;
    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    viewer.setSceneData(root.get());
    viewer.getCamera()->setClearColor(osg::Vec4(0.02f, 0.03f, 0.05f, 1.0f));
    viewer.realize();

    // ---- Load the earth root tile in the background while continuing to render
    //      (the UI overlay shows a notice), avoiding network timeouts dragging the
    //      window-appearance time out to tens of seconds ----
    std::future<osg::ref_ptr<osg::Node>> earthTask = std::async(std::launch::async, [&]()
    {
        return osg::ref_ptr<osg::Node>(osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get()));
    });
    while (earthTask.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
        viewer.frame();

    osg::ref_ptr<osg::Node> earth = earthTask.get();
    if (!earth)
    {
        OSG_WARN << "[EarthExplorer] earth tile source unavailable, using the fallback sphere "
                    "(check the globe.orthophoto/elevation config and the network connection)\n";
        earth = createFallbackGlobe();
    }
    uiHandler->setLoadingEarth(false);

    // ---- Atmosphere / ocean / composite pipeline ----
    osgVerse::EarthAtmosphereOcean earthRenderingUtils;
    osg::ref_ptr<osg::MatrixTransform> earthRoot = new osg::MatrixTransform;
    std::vector<osg::Camera*> cameras = configureEarthRendering(
        viewer, earthRoot.get(), earth.get(), earthRenderingUtils, mainFolder,
        EARTH_INTERSECTION_MASK, w, h);

    // ---- Attach ImGui to the final composite HUD camera so the UI is drawn on
    //      top of the earth/sky/ocean composite. That camera is POST_RENDER and
    //      renders after the main camera, so the UI must be attached to it,
    //      otherwise the globe/sky would paint over the interface ----
    imgui->addToView(&viewer, cameras.empty() ? NULL : cameras.back());
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
    earthRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif

    osg::Camera* sceneCamera = cameras[0];
    sceneCamera->setLODScale(0.8f);
    sceneCamera->setNearFarRatio(0.00001);
    sceneCamera->setSmallFeatureCullingPixelSize(10.0f);

    // ---- City data (local assets, disabled by default via config; on or off never affects the globe) ----
    if (cfg.cityEnabled)
    {
        std::string cityFolder = cfg.cityFolder.empty() ? mainFolder : cfg.cityFolder;
        sceneCamera->addChild(configureCityData(
            viewer, earthRoot.get(), earthRenderingUtils, cityFolder,
            EARTH_INTERSECTION_MASK, cityWaitingTiles));
    }
    else
    {
        OSG_NOTICE << "[EarthExplorer] city data not enabled (config: city.enabled), "
                      "enable it and provide local city buildings in the config file if needed\n";
    }

    // ---- External data source framework (Factory instantiates by config; no network/no config just means no data) ----
    sceneCamera->addChild(dataSourceManager.initialize(cfg, viewer, layerRegistry));

    // ---- Manipulator (created only after the earth is ready, EnvironmentUpdater attached at the same time) ----
    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(manipulatorCanThrow);
    // The viewer is realized before the manipulator is created, so View::init()
    // (which normally calls CameraManipulator::init) has already run and the
    // manipulator's internal viewer stays NULL. Without it, ray/ellipsoid
    // picking is skipped and mouse-dragging can no longer rotate the globe.
    earthManipulator->setViewer(&viewer);
    earthRoot->addUpdateCallback(
        new EnvironmentUpdater(&earthRenderingUtils, &envState, earthManipulator.get()));

    root->addChild(earthRoot.get());
    viewer.setCameraManipulator(earthManipulator.get());

    // ---- Final AI assistant assembly (the manipulator is ready only now): register built-in tools + workspace context snapshot ----
    if (aiCore)
    {
        earthai::registerEarthAITools(aiToolRegistry, &layerRegistry,
                                      &dataSourceManager, earthManipulator.get());
        osgVerse::EarthManipulator* maniPtr = earthManipulator.get();
        LayerRegistry* layersPtr = &layerRegistry;
        earthds::DataSourceManager* sourcesPtr = &dataSourceManager;
        aiCore->setContextProvider([maniPtr, layersPtr, sourcesPtr]()
        {
            return earthai::buildEarthContextJson(maniPtr, layersPtr, sourcesPtr);
        });
    }

    // Central main-thread drain: layer pending work (fixes UI checkbox changes
    // that previously only queued without taking effect) + the AI agent loop
    viewer.addEventHandler(new MainThreadDrainHandler(&layerRegistry, aiCore));

    // ---- Headless e2e: --ai-submit <text> [--ai-transcript <file>] ----
    std::string aiSubmitText, aiTranscriptFile;
    arguments.read("--ai-submit", aiSubmitText);
    arguments.read("--ai-transcript", aiTranscriptFile);
    if (!aiSubmitText.empty() && aiCore)
        viewer.addEventHandler(new AIAutoSubmitHandler(aiCore, aiSubmitText, aiTranscriptFile));

    // ---- Headless verification hook: attach the framebuffer grab once the earth
    //      is ready. Note: this render path only calls back initial/pre/post
    //      (ImGui occupies pre/post, and CameraDrawCallback::run does not chain
    //      nested callbacks), so we use the initial slot and grab the previous
    //      frame's content ----
    if (!shotFile.empty())
    {
        osg::Camera* shotCam = viewer.getCamera();
        if (!shotCam->getInitialDrawCallback())
            shotCam->setInitialDrawCallback(new FrameShotCallback(shotFile, 60));
        else
            shotCam->addInitialDrawCallback(new FrameShotCallback(shotFile, 60));
    }
    if (autoExit) viewer.addEventHandler(new AutoExitHandler(90));

    int result = viewer.run();

    // ---- Explicit exit cleanup: wait for the render thread to really stop (the
    //      GL thread still runs ImGui's PRE/POST_DRAW callbacks, and racing the
    //      main thread's cleanup would AV), then shut ImGui down and release the
    //      scene graph, avoiding access to destroyed objects during static
    //      singleton destruction (same exit-crash fix as scene_editor) ----
    viewer.stopThreading();
    imgui->shutdown();
    viewer.setSceneData(NULL);

    // AI: destroy ChatCore first (its destructor joins the worker thread, which
    // still uses the provider), then release the Provider itself
    if (aiCore) { delete aiCore; aiCore = NULL; }
    if (aiProvider) { delete aiProvider; aiProvider = NULL; }
    return result;
}
