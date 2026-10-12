// ImGui main interface implementation. Every window is shown in any config
// state (entries are greyed out / show an empty state when a data source is
// unavailable), guaranteeing that "regardless of whether a config is loaded,
// the earth + UI are always visible".
#include "earth_ui.h"
#include "marker_style.h"
#include <osg/io_utils>
#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <ui/ImGuiComponents.h>
#include <imgui.h>
#include <cstdio>
#include <sstream>

namespace
{
    // Cross-platform external link opener (detail card "Open" button)
    void openUrl(const std::string& url)
    {
        if (url.empty()) return;
#ifdef _WIN32
        std::string cmd = "start \"\" \"" + url + "\"";
#elif defined(__APPLE__)
        std::string cmd = "open \"" + url + "\"";
#else
        std::string cmd = "xdg-open \"" + url + "\"";
#endif
        int rc = system(cmd.c_str());
        if (rc != 0)
            OSG_WARN << "[EarthUI] Failed to open link: " << url << "\n";
    }

    unsigned int toImU32(const osg::Vec4& c)
    {
        unsigned int r = (unsigned int)(c.r() * 255.0f + 0.5f);
        unsigned int g = (unsigned int)(c.g() * 255.0f + 0.5f);
        unsigned int b = (unsigned int)(c.b() * 255.0f + 0.5f);
        unsigned int a = (unsigned int)(c.a() * 255.0f + 0.5f);
        return (a << 24) | (b << 16) | (g << 8) | r;
    }
}

void EarthUIHandler::runInternal(osgVerse::ImGuiManager* m)
{
    // Earth root tile still loading in the background: the window is already
    // visible, so show a centered notice
    if (_loadingEarth)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin(u8"Loading", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(u8"Loading earth imagery...");
        ImGui::TextDisabled(u8"On first run or when the network is unreachable, tile requests must time out first");
        ImGui::End();
    }

    drawLayerPanel(m);
    drawFeatureListPanel(m);
    drawDetailCard(m);
    drawEnvironmentPanel(m);
    drawChatPanel(m);
    drawHoverPopup(m);
}

void EarthUIHandler::drawLayerPanel(osgVerse::ImGuiManager* m)
{
    ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 420.0f), ImGuiCond_Once);
    ImGui::Begin(u8"Layers");

    if (_config && !_config->loaded)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
                           u8"Config file not found, using built-in defaults");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(u8"Search paths: --config / <install dir>/misc/earth_explorer.json / current directory\nSee the [EarthConfig] warnings in the console");
        ImGui::Separator();
    }

    // Data source health statistics
    if (_sources)
    {
        int ok = 0, enabled = 0;
        _sources->health(ok, enabled);
        ImGui::Text(u8"Online data sources: %d / %d", ok, enabled);
        ImGui::Separator();
    }

    // Lay out layer entries grouped together (snapshot copy, entry count is small)
    std::vector<OverlayLayer> layers = _layers ? _layers->layersSnapshot() : std::vector<OverlayLayer>();
    if (layers.empty())
    {
        ImGui::TextDisabled(u8"(no registered layers)");
    }
    else
    {
        std::string lastGroup;
        for (size_t i = 0; i < layers.size(); ++i)
        {
            const OverlayLayer& layer = layers[i];
            if (layer.group != lastGroup)
            {
                if (!lastGroup.empty()) ImGui::Spacing();
                ImGui::TextDisabled("%s", layer.group.c_str());
                lastGroup = layer.group;
            }

            ImGui::PushID(layer.id.c_str());
            bool enabled = layer.enabled;
            if (layer.needsKey)
            {
                // Key missing: entry is not interactive (greyed out) + hint
                ImGui::TextDisabled("[ ]");
                ImGui::SameLine();
                earthmark::drawMarkerIcon(ImGui::GetWindowDrawList(), layer.shape,
                    ImGui::GetCursorScreenPos().x + 9.0f, ImGui::GetCursorScreenPos().y + 9.0f,
                    18.0f, toImU32(layer.iconColor));
                ImGui::Dummy(ImVec2(22.0f, 18.0f)); ImGui::SameLine();
                ImGui::TextDisabled("%s", layer.displayName.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled(u8"[No Key]");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"Missing API Key, see the [EarthConfig] warnings in the console");
            }
            else
            {
                if (ImGui::Checkbox("##en", &enabled))
                {
                    if (_layers) _layers->setEnabled(layer.id, enabled);
                }
                ImGui::SameLine();
                earthmark::drawMarkerIcon(ImGui::GetWindowDrawList(), layer.shape,
                    ImGui::GetCursorScreenPos().x + 9.0f, ImGui::GetCursorScreenPos().y + 9.0f,
                    18.0f, toImU32(layer.iconColor));
                ImGui::Dummy(ImVec2(22.0f, 18.0f)); ImGui::SameLine();
                ImGui::Text("%s", layer.displayName.c_str());

                // Fetch status indicator (right side of the same row)
                if (enabled && layer.fetchStatus)
                {
                    std::string err;
                    int state = layer.fetchStatus(err);
                    ImGui::SameLine();
                    if (state == 1) ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), u8"*");
                    else if (state == 2)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f), u8"*");
                        if (ImGui::IsItemHovered() && !err.empty())
                            ImGui::SetTooltip("%s", err.c_str());
                    }
                    else if (state == 3) ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), u8"o");
                    else ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), u8"...");
                }
            }

            if (!layer.subtitle.empty() && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", layer.subtitle.c_str());
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void EarthUIHandler::drawFeatureListPanel(osgVerse::ImGuiManager* m)
{
    // Feature titles of every enabled point source, kept in sync with the loaded
    // data (the count/title hooks read the source's locked snapshot each frame).
    // Clicking a row activates that feature, showing its detail card.
    if (!_layers) return;
    std::vector<OverlayLayer> layers = _layers->layersSnapshot();

    bool any = false;
    for (size_t i = 0; i < layers.size(); ++i)
        if (layers[i].enabled && layers[i].featureCount && layers[i].featureCount() > 0)
        { any = true; break; }
    if (!any) return;

    ImGui::SetNextWindowPos(ImVec2(16.0f, 480.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"Features");

    ImGui::BeginChild("feature_list", ImVec2(0.0f, 0.0f), false);
    earthds::Selection sel = earthds::currentSelection();
    for (size_t i = 0; i < layers.size(); ++i)
    {
        const OverlayLayer& layer = layers[i];
        if (!layer.enabled || !layer.featureCount) continue;
        size_t count = layer.featureCount();
        if (count == 0) continue;

        ImGui::PushID(layer.id.c_str());
        ImGui::TextDisabled("%s (%zu)", layer.displayName.c_str(), count);
        ImGui::Indent(12.0f);
        for (size_t k = 0; k < count; ++k)
        {
            std::string title = layer.featureTitle ? layer.featureTitle(k) : std::string();
            if (title.empty()) title = "(untitled)";
            // Single selection: only the globally selected feature is shown as selected
            bool selected = (sel.valid && sel.sourceId == layer.id && sel.index == (int)k);
            ImGui::PushID((int)k);
            if (ImGui::Selectable(title.c_str(), selected))
            {
                if (layer.featureActivate) layer.featureActivate(k);
            }
            // Double click: fly the camera to that feature (deferred to the main thread)
            if (_env && layer.featureLatLon && ImGui::IsItemHovered()
                && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                double lat = 0.0, lon = 0.0;
                if (layer.featureLatLon(k, lat, lon))
                {
                    _env->focusLat = lat; _env->focusLon = lon; _env->focusRequest = true;
                }
            }
            ImGui::PopID();
        }
        ImGui::Unindent(12.0f);
        ImGui::Spacing();
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::End();
}

void EarthUIHandler::drawHoverPopup(osgVerse::ImGuiManager* m)
{
    // Popup shown while the mouse hovers a feature marker on the globe (hover
    // state is produced by mouse-move picking in the data source manager).
    earthds::HoverInfo hover = earthds::currentHover();
    if (!hover.valid) return;

    ImGuiIO& io = ImGui::GetIO();
    const float pad = 18.0f;
    float x = hover.x + pad, y = hover.y + pad;
    if (x > io.DisplaySize.x - 80.0f) x = hover.x - 280.0f;  // flip left near the right edge
    if (y > io.DisplaySize.y - 80.0f) y = hover.y - 140.0f;  // flip up near the bottom edge

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("##hover_popup", NULL,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                 | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize
                 | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
                 | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::TextUnformatted(hover.title.c_str());
    if (!hover.detail.empty())
    {
        ImGui::Separator();
        std::istringstream lines(hover.detail);
        std::string line;
        while (std::getline(lines, line))
            if (!line.empty()) ImGui::TextUnformatted(line.c_str());
    }
    ImGui::End();
}

void EarthUIHandler::drawDetailCard(osgVerse::ImGuiManager* m)
{
    earthds::Selection sel = earthds::currentSelection();
    if (!sel.valid) return;

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 336.0f, 40.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(320.0f, 0.0f), ImGuiCond_Always);
    ImGui::Begin(u8"Feature Details", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("%s", sel.title.c_str());
    ImGui::Separator();
    // detail is multi-line text separated by \n
    std::istringstream lines(sel.detail);
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty()) ImGui::TextWrapped("%s", line.c_str());
    }

    if (!sel.url.empty())
    {
        if (ImGui::Button(u8"Open Link", ImVec2(-1.0f, 0.0f))) openUrl(sel.url);
    }
    if (ImGui::Button(u8"Close", ImVec2(-1.0f, 0.0f))) earthds::clearSelection();
    ImGui::End();
}

void EarthUIHandler::drawEnvironmentPanel(osgVerse::ImGuiManager* m)
{
    if (!_env) return;

    ImGui::SetNextWindowPos(ImVec2(16.0f, ImGui::GetIO().DisplaySize.y - 16.0f),
                            ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::Begin(u8"Environment / View", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    float sun = _env->sunAngle;
    if (ImGui::SliderFloat(u8"Sun Azimuth", &sun, 0.0f, 6.2831853f)) _env->sunAngle = sun;
    bool ocean = (_env->oceanOpaque != 0);
    if (ImGui::Checkbox(u8"Ocean Effect", &ocean)) _env->oceanOpaque = ocean ? 1 : 0;
    if (ImGui::Button(u8"Reset View")) _env->goHome = true;

    // Camera position (latitude/longitude/altitude)
    if (_viewer && _viewer->getCamera())
    {
        osg::Vec3d eye = _viewer->getCamera()->getInverseViewMatrix().getTrans();
        osg::Vec3d lla = osgVerse::Coordinate::convertECEFtoLLA(eye);
        ImGui::TextDisabled("Lat %.2f, Lon %.2f, Alt %.0fkm",
                            osg::RadiansToDegrees(lla.x()), osg::RadiansToDegrees(lla.y()),
                            lla.z() * 0.001);
    }
    ImGui::End();
}

void EarthUIHandler::drawChatPanel(osgVerse::ImGuiManager* m)
{
    // Bottom floating AI chat bar: compact input + send; history expands on
    // demand (collapsed by default so the map stays front and center).
    // When _aiChat is null (AI disabled / key missing) draw the disabled state
    // and no history/buttons -- zero interference for the non-AI case.
    ImGuiIO& io = ImGui::GetIO();
    const float barWidth = 520.0f;
    const float barHeight = _chatHistoryOpen ? 320.0f : 0.0f;
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 12.0f),
        ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(barWidth, 0.0f), ImGuiCond_Always);
    ImGui::Begin(u8"AI Assistant", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                                     | ImGuiWindowFlags_AlwaysAutoResize);

    if (!_aiChat)
    {
        ImGui::TextDisabled(u8"AI assistant disabled (configure the ai section or set an API Key, then restart)");
        ImGui::End();
        return;
    }

    if (!_aiProviderName.empty())
    {
        ImGui::TextDisabled("%s", _aiProviderName.c_str());
        ImGui::Separator();
    }

    // Conversation history (collapsible)
    if (_chatHistoryOpen)
    {
        std::vector<earthai::ChatEntry> entries = _aiChat->transcript();
        if (entries.empty())
            ImGui::TextDisabled(u8"(No messages yet. Try: Where are there earthquakes right now? Enable the disaster layer.)");
        else
        {
            ImGui::BeginChild("chat_history", ImVec2(0.0f, barHeight), true);
            bool scrollToBottom = (entries.size() != _chatLastCount);
            for (size_t i = 0; i < entries.size(); ++i)
            {
                const earthai::ChatEntry& e = entries[i];
                if (e.kind == earthai::ChatEntry::USER)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.9f, 1.0f, 1.0f));
                    ImGui::TextWrapped(u8"You: %s", e.text.c_str());
                    ImGui::PopStyleColor();
                }
                else if (e.kind == earthai::ChatEntry::ASSISTANT)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.6f, 1.0f));
                    ImGui::TextWrapped(u8"AI: %s", e.text.c_str());
                    ImGui::PopStyleColor();
                }
                else if (e.kind == earthai::ChatEntry::TOOL_NOTE)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
                    ImGui::TextWrapped("%s", e.text.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.35f, 1.0f));
                    ImGui::TextWrapped("%s", e.text.c_str());
                    ImGui::PopStyleColor();
                }
            }
            if (scrollToBottom) ImGui::SetScrollHereY(1.0f);
            _chatLastCount = entries.size();
            ImGui::EndChild();
        }
    }

    // Input row: history toggle + input box + send
    {
        bool busy = _aiChat->busy();
        ImGui::PushItemWidth(90.0f);
        if (ImGui::Button(_chatHistoryOpen ? u8"Hide" : u8"History"))
            _chatHistoryOpen = !_chatHistoryOpen;
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGuiInputTextFlags flags = busy ? ImGuiInputTextFlags_ReadOnly : 0;
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
        bool send = ImGui::InputText("##chat_input", _chatInput,
                                     sizeof(_chatInput),
                                     flags | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (busy)
        {
            ImGui::TextDisabled(u8"Thinking");
        }
        else
        {
            if (ImGui::Button(u8"Send") || (send && _chatInput[0] != '\0'))
            {
                std::string text(_chatInput);
                if (!text.empty()) _aiChat->submit(text);
                _chatInput[0] = '\0';
                _chatHistoryOpen = true;  // auto-expand after sending so the reply is visible
            }
        }
    }
    ImGui::End();
}

void EnvironmentUpdater::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (_earthData && _env)
    {
        float sun = _env->sunAngle;
        _earthData->commonUniforms["WorldSunDir"]->set(
            osg::Vec3(-1.0f, 0.0f, 0.0f) * osg::Matrix::rotate(sun, osg::Z_AXIS));
        _earthData->commonUniforms["OceanOpaque"]->set((float)(_env->oceanOpaque ? 1 : 0));
        if (_env->goHome)
        {
            _env->goHome = false;
            if (_manipulator) _manipulator->home(0.0);
        }
        if (_env->focusRequest)
        {
            // Double-click in the feature list: recenter on the target lat/lon while
            // keeping the current eye altitude (so the zoom level is preserved)
            _env->focusRequest = false;
            osgVerse::EarthManipulator* em =
                dynamic_cast<osgVerse::EarthManipulator*>(_manipulator);
            if (em)
            {
                double alt = em->computeEyeLatLonHeight().z();
                em->setByEye(osg::inDegrees(_env->focusLat.load()),
                             osg::inDegrees(_env->focusLon.load()), alt);
            }
        }
    }
    traverse(node, nv);
}
