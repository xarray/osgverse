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

// 瓦片源完全不可用时的兜底球体:保证"无论如何启动都能看到一个地球"
static osg::Node* createFallbackGlobe()
{
    osg::ref_ptr<osg::Sphere> shape = new osg::Sphere(osg::Vec3(), 6371000.0f);
    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(shape.get());
    drawable->setColor(osg::Vec4(0.18f, 0.32f, 0.55f, 1.0f));
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(drawable.get());
    return geode.release();
}

// 键盘:',' '.' 调太阳;'o' 记录动画路径帧 / 'i' 保存路径 / 'p' 载入并播放路径
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

// 无头环境验证用:--shot <file> 在下一帧抓取主帧缓冲存 PNG(屏幕截图在
// 无头/断开会话中不可靠,直接读 framebuffer);--auto-exit 抓帧后自动退出
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
        // 写文件必须离开 GL 线程:writeImageFile 可能触发插件加载,
        // 与主线程 / DatabasePager 并发访问 Registry 会竞争崩溃
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

// 主线程每帧的集中 drain:
// 1) LayerRegistry 待办(UI/AI 提交的图层开关在此真正生效——apply 回调碰场景图);
// 2) AIChatCore 代理循环(执行模型排队的工具调用、推进下一轮)。
// 工具 execute 与场景图更新都要求主线程,绝不放进 draw/worker 线程。
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

// 无头 e2e 验证钩子:--ai-submit <text> 在第 N 帧自动提交一条用户消息,
// --ai-transcript <file> 在 AI 空闲(最终文本/错误落定)后把会话记录写盘并退出。
// 真实 API 一轮 10~30s,不能按固定帧数等;busy 清零后再缓冲若干帧,另有 240s 超时兜底。
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
        if (!_submitted && _frames >= 90)   // 等漫游器/图层就绪
        {
            _core->submit(_text);
            _submitted = true;
            _submitFrame = _frames;
            OSG_NOTICE << "[EarthAI] auto-submit: " << _text << "\n";
        }
        if (_submitted && !_dumped && !_transcriptFile.empty())
        {
            // busy 由 submit 置位、由 drainMainThread 在最终文本/错误时清零
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

    // ---- 统一配置:外部 API / 图源 / 数据源全部来自 earth_explorer.json ----
    earthee::AppConfig cfg = earthee::loadAppConfig(arguments);

    std::string mainFolder = BASE_DIR + "/models"; arguments.read("--folder", mainFolder);
    int w = 1920, h = 1080; arguments.read("--resolution", w, h);
    bool cityWaitingTiles = true, manipulatorCanThrow = false;
    if (arguments.read("--no-wait")) cityWaitingTiles = false;
    if (arguments.read("--thrown")) manipulatorCanThrow = true;

    // ---- 地球瓦片(全部 URL 模板来自配置;缺省时为在线公共图源)----
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

    // ---- 缓存提前挂上:根瓦片命中本地缓存时启动几乎是即时的 ----
    osg::ref_ptr<osgVerse::FileCache> fileCache = new osgVerse::FileCache("earth_cache");
    osgDB::Registry::instance()->setFileCache(fileCache.get());

    // ---- 视图 / UI 先行:窗口和 ImGui 立即出现,不等待网络 ----
    EnvironmentState envState;
    earthds::registerBuiltinSourceTypes();
    LayerRegistry layerRegistry;
    earthds::DataSourceManager dataSourceManager;

    // ---- AI 助手:按配置实例化 Provider(工厂模式);缺 key/缺配置只是禁用,
    //      绝不影响地球与其它 UI。所有权在 main 栈上,退出时显式回收 ----
    earthai::ToolRegistry aiToolRegistry;
    earthai::LLMProvider* aiProvider = NULL;   // AIChatCore 不持有所有权
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
        if (!activePC) activePC = &cfg.ai.providers[0];  // 未指定/找不到 → 第一个

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
            OSG_NOTICE << "[EarthAI] AI 助手已启用: " << aiProviderName << "\n";
        }
        else
        {
            OSG_WARN << "[EarthAI] " << u8"AI 供应商不可用(缺 key 或类型错误),"
                        u8"聊天面板将显示禁用状态\n";
        }
    }
    else if (cfg.ai.enabled)
    {
        OSG_WARN << "[EarthAI] " << u8"没有可用的 AI 供应商配置,聊天面板将显示禁用状态\n";
    }

    osg::ref_ptr<EarthUIHandler> uiHandler = new EarthUIHandler(
        &layerRegistry, &dataSourceManager, &envState, &cfg);
    uiHandler->setViewer(&viewer);
    uiHandler->setLoadingEarth(true);
    uiHandler->setAIChat(aiCore, aiProviderName);

    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    std::string fontPath = cfg.fontPath.empty() ? (MISC_DIR + "LXGWFasmartGothic.otf")
                                                : cfg.fontPath;
    if (osgDB::fileExists(fontPath)) imgui->setChineseSimplifiedFont(fontPath);
    else OSG_WARN << "[EarthExplorer] 中文字体不存在(" << fontPath << "),界面中文可能显示异常\n";
    imgui->initialize(uiHandler.get());
    imgui->addToView(&viewer);

    osgDB::DatabasePager* pager = new osgDB::DatabasePager;
    pager->setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS);
    pager->setDoPreCompile(true); pager->setUpThreads(6, 3);

    viewer.addEventHandler(new KeyHandler(&envState));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    viewer.setDatabasePager(pager);

    // ---- 无头验证:--shot <file> [--auto-exit](回调在地球就绪后挂上)----
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

    // ---- 地球根瓦片后台加载:期间持续渲染(UI 覆盖层提示),
    //      避免网络超时把窗口出现时间拖长到几十秒 ----
    std::future<osg::ref_ptr<osg::Node>> earthTask = std::async(std::launch::async, [&]()
    {
        return osg::ref_ptr<osg::Node>(osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get()));
    });
    while (earthTask.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
        viewer.frame();

    osg::ref_ptr<osg::Node> earth = earthTask.get();
    if (!earth)
    {
        OSG_WARN << "[EarthExplorer] 地球瓦片源不可用,使用兜底球体(检查配置 globe.orthophoto/elevation "
                    "与网络连接)\n";
        earth = createFallbackGlobe();
    }
    uiHandler->setLoadingEarth(false);

    // ---- 大气 / 海洋 / 合成管线 ----
    osgVerse::EarthAtmosphereOcean earthRenderingUtils;
    osg::ref_ptr<osg::MatrixTransform> earthRoot = new osg::MatrixTransform;
    std::vector<osg::Camera*> cameras = configureEarthRendering(
        viewer, earthRoot.get(), earth.get(), earthRenderingUtils, mainFolder,
        EARTH_INTERSECTION_MASK, w, h);
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
    earthRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif

    osg::Camera* sceneCamera = cameras[0];
    sceneCamera->setLODScale(0.8f);
    sceneCamera->setNearFarRatio(0.00001);
    sceneCamera->setSmallFeatureCullingPixelSize(10.0f);

    // ---- 城市数据(本地资产,配置默认关闭;开启与否都不影响地球显示)----
    if (cfg.cityEnabled)
    {
        std::string cityFolder = cfg.cityFolder.empty() ? mainFolder : cfg.cityFolder;
        sceneCamera->addChild(configureCityData(
            viewer, earthRoot.get(), earthRenderingUtils, cityFolder,
            EARTH_INTERSECTION_MASK, cityWaitingTiles));
    }
    else
    {
        OSG_NOTICE << "[EarthExplorer] 城市数据未启用(config: city.enabled)"
                      ",如需本地城市建筑请在配置文件中打开\n";
    }

    // ---- 外部数据源框架(Factory 按配置实例化;无网络/无配置只是无数据)----
    sceneCamera->addChild(dataSourceManager.initialize(cfg, viewer, layerRegistry));

    // ---- 漫游器(地球就绪后才创建,EnvironmentUpdater 同步挂接)----
    osg::ref_ptr<osgVerse::EarthManipulator> earthManipulator = new osgVerse::EarthManipulator;
    earthManipulator->setIntersectionMask(EARTH_INTERSECTION_MASK);
    earthManipulator->setWorldNode(earth.get());
    earthManipulator->setThrowAllowed(manipulatorCanThrow);
    earthRoot->addUpdateCallback(
        new EnvironmentUpdater(&earthRenderingUtils, &envState, earthManipulator.get()));

    root->addChild(earthRoot.get());
    viewer.setCameraManipulator(earthManipulator.get());

    // ---- AI 助手最终装配(漫游器此时才就绪):注册内置工具 + 工作区上下文快照 ----
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

    // 主线程集中 drain:图层待办(此前 UI 勾选只排队不生效的修复)+ AI 代理循环
    viewer.addEventHandler(new MainThreadDrainHandler(&layerRegistry, aiCore));

    // ---- 无头 e2e:--ai-submit <text> [--ai-transcript <file>] ----
    std::string aiSubmitText, aiTranscriptFile;
    arguments.read("--ai-submit", aiSubmitText);
    arguments.read("--ai-transcript", aiTranscriptFile);
    if (!aiSubmitText.empty() && aiCore)
        viewer.addEventHandler(new AIAutoSubmitHandler(aiCore, aiSubmitText, aiTranscriptFile));

    // ---- 无头验证钩子:地球就绪后挂帧缓冲抓取。
    //      注意:本渲染路径只回调 initial/pre/post 三类(ImGui 占用 pre/post,
    //      且 CameraDrawCallback::run 不链式调用嵌套回调),所以挂 initial 槽,
    //      抓到的是上一帧内容 ----
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

    // ---- 显式退出清理:先等渲染线程真正停掉(GL 线程还会执行 ImGui 的
    // PRE/POST_DRAW 回调,与主线程清理竞争会 AV),再关 ImGui、释放场景图,
    // 避免静态单例析构期访问已销毁对象(同 scene_editor 的退出崩溃修复)----
    viewer.stopThreading();
    imgui->shutdown();
    viewer.setSceneData(NULL);

    // AI:先销毁 ChatCore(析构会 join worker 线程,worker 还要用 provider),
    // 再释放 Provider 本体
    if (aiCore) { delete aiCore; aiCore = NULL; }
    if (aiProvider) { delete aiProvider; aiProvider = NULL; }
    return result;
}
