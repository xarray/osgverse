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

// Environment state (the UI draw thread writes these atomics; the main-thread
// update callback reads them and pushes them to shader uniforms)
struct EnvironmentState
{
    std::atomic<float> sunAngle;     // Sun azimuth (radians)
    std::atomic<int> oceanOpaque;    // Ocean opacity 0/1
    std::atomic<bool> goHome;        // Request view reset (executed on the main thread)
    // Feature focus request (double-click in the feature list): the UI draw thread only
    // records the target location, and the main-thread update callback performs the jump
    std::atomic<double> focusLat, focusLon;   // degrees
    std::atomic<bool> focusRequest;
    EnvironmentState()
    {
        sunAngle = 0.0f; oceanOpaque = 0; goHome = false;
        focusLat = 0.0; focusLon = 0.0; focusRequest = false;
    }
};

// ImGui main interface: layer catalog / data source status / feature detail
// card / environment and view controls. Reuses the core library
// osgVerse::ImGuiManager + ImGuiContentHandler, and marker icons reuse
// earthmark::drawMarkerIcon (the same shape library as the in-scene point sprites).
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
    // AI assistant core (filled in after the main thread wires it up; when
    // NULL/disabled the chat bar shows its disabled state)
    void setAIChat(earthai::AIChatCore* core, const std::string& providerName)
    { _aiChat = core; _aiProviderName = providerName; }
    virtual void runInternal(osgVerse::ImGuiManager* m);

protected:
    virtual ~EarthUIHandler() {}

    void drawLayerPanel(osgVerse::ImGuiManager* m);
    void drawFeatureListPanel(osgVerse::ImGuiManager* m);
    void drawHoverPopup(osgVerse::ImGuiManager* m);
    void drawDetailCard(osgVerse::ImGuiManager* m);
    void drawEnvironmentPanel(osgVerse::ImGuiManager* m);
    void drawChatPanel(osgVerse::ImGuiManager* m);

    LayerRegistry* _layers;
    earthds::DataSourceManager* _sources;
    EnvironmentState* _env;
    const earthee::AppConfig* _config;
    osgViewer::Viewer* _viewer;
    std::atomic<bool> _loadingEarth;  // Show an overlay while the earth root tile still loads in the background

    earthai::AIChatCore* _aiChat;     // Does not own the object (stack object in earth_main)
    std::string _aiProviderName;      // Current provider display name (e.g. "deepseek-chat")
    char _chatInput[1024];            // Chat input buffer
    bool _chatHistoryOpen;            // History panel expansion state (collapsed by default, map first)
    size_t _chatLastCount;            // Entry count at the last draw, used to auto-scroll to the bottom
};

// Main thread: push the EnvironmentState written by the UI to the atmosphere /
// ocean uniforms, and execute goHome
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
