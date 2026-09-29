#ifndef EARTH_UI_H
#define EARTH_UI_H

#include <osg/NodeCallback>
#include <osgGA/CameraManipulator>
#include <osgViewer/Viewer>
#include <pipeline/Utilities.h>
#include <ui/ImGui.h>
#include <atomic>
#include "ai_chat.h"
#include "DataSource.h"
#include "LayerRegistry.h"
#include "EarthAppConfig.h"

// 环境状态(UI draw 线程写 atomic,主线程的更新回调读取并落到 shader uniform)
struct EnvironmentState
{
    std::atomic<float> sunAngle;     // 太阳方位角(弧度)
    std::atomic<int> oceanOpaque;    // 海洋不透明度 0/1
    std::atomic<bool> goHome;        // 请求视角归位(主线程执行)
    EnvironmentState() { sunAngle = 0.0f; oceanOpaque = 0; goHome = false; }
};

// ImGui 主界面:图层目录 / 数据源状态 / 要素详情卡 / 环境与视角控制。
// 复用核心库 osgVerse::ImGuiManager + ImGuiContentHandler,标记图标复用
// earthmark::drawMarkerIcon(与场景内点精灵同一份形状库)。
class EarthUIHandler : public osgVerse::ImGuiContentHandler
{
public:
    EarthUIHandler(LayerRegistry* layers, earthds::DataSourceManager* sources,
                   EnvironmentState* env, const earthee::AppConfig* config)
    :   _layers(layers), _sources(sources), _env(env), _config(config), _viewer(NULL)
    ,   _aiChat(NULL), _chatHistoryOpen(false), _chatLastCount(0)
    {
        _loadingEarth = false;
        _chatInput[0] = '\0';
    }

    void setViewer(osgViewer::Viewer* viewer) { _viewer = viewer; }
    void setLoadingEarth(bool v) { _loadingEarth = v; }
    // AI 助手核心(主线程装配好之后回填;NULL/未启用时聊天条显示禁用态)
    void setAIChat(earthai::AIChatCore* core, const std::string& providerName)
    { _aiChat = core; _aiProviderName = providerName; }
    virtual void runInternal(osgVerse::ImGuiManager* m);

protected:
    virtual ~EarthUIHandler() {}

    void drawLayerPanel(osgVerse::ImGuiManager* m);
    void drawDetailCard(osgVerse::ImGuiManager* m);
    void drawEnvironmentPanel(osgVerse::ImGuiManager* m);
    void drawChatPanel(osgVerse::ImGuiManager* m);

    LayerRegistry* _layers;
    earthds::DataSourceManager* _sources;
    EnvironmentState* _env;
    const earthee::AppConfig* _config;
    osgViewer::Viewer* _viewer;
    std::atomic<bool> _loadingEarth;  // 地球根瓦片仍在后台加载时显示覆盖层

    earthai::AIChatCore* _aiChat;     // 不持有所有权(earth_main 栈对象)
    std::string _aiProviderName;      // 当前供应商显示名(如 "deepseek-chat")
    char _chatInput[1024];            // 聊天输入缓冲
    bool _chatHistoryOpen;            // 历史面板展开状态(默认折叠,地图优先)
    size_t _chatLastCount;            // 上次绘制时的条目数,用于自动滚动到底部
};

// 主线程:把 UI 写入的 EnvironmentState 落到大气/海洋 uniform,并执行 goHome
class EnvironmentUpdater : public osg::NodeCallback
{
public:
    EnvironmentUpdater(osgVerse::EarthAtmosphereOcean* eao, EnvironmentState* env,
                       osgGA::CameraManipulator* mani)
    :   _earthData(eao), _env(env), _manipulator(mani) {}
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

protected:
    osgVerse::EarthAtmosphereOcean* _earthData;
    EnvironmentState* _env;
    osgGA::CameraManipulator* _manipulator;
};

#endif
