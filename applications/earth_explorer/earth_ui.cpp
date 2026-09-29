// ImGui 主界面实现。全部窗口在任何配置状态下都会显示(数据源不可用时
// 相应条目灰显/空态提示),保证"无论是否加载配置,地球 + UI 界面始终可见"。
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
    // 跨平台打开外部链接(详情卡"打开"按钮)
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
            OSG_WARN << "[EarthUI] 无法打开链接: " << url << "\n";
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
    // 地球根瓦片仍在后台加载:窗口已显示,给出居中提示
    if (_loadingEarth)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin(u8"加载中", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(u8"正在加载地球影像...");
        ImGui::TextDisabled(u8"首次运行或网络不可达时,需要等待瓦片请求超时");
        ImGui::End();
    }

    drawLayerPanel(m);
    drawDetailCard(m);
    drawEnvironmentPanel(m);
    drawChatPanel(m);
}

void EarthUIHandler::drawLayerPanel(osgVerse::ImGuiManager* m)
{
    ImGui::SetNextWindowPos(ImVec2(16.0f, 40.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 420.0f), ImGuiCond_Once);
    ImGui::Begin(u8"图层 / Layers");

    if (_config && !_config->loaded)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
                           u8"未找到配置文件,当前为内置默认配置");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(u8"搜索路径:--config / <安装目录>/misc/earth_explorer.json / 当前目录\n详见控制台 [EarthConfig] 告警");
        ImGui::Separator();
    }

    // 数据源健康统计
    if (_sources)
    {
        int ok = 0, enabled = 0;
        _sources->health(ok, enabled);
        ImGui::Text(u8"在线数据源: %d / %d", ok, enabled);
        ImGui::Separator();
    }

    // 按分组排列图层条目(快照拷贝,条目数小)
    std::vector<OverlayLayer> layers = _layers ? _layers->layersSnapshot() : std::vector<OverlayLayer>();
    if (layers.empty())
    {
        ImGui::TextDisabled(u8"(没有已注册的图层)");
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
                // 缺 Key:条目不可交互(灰显)+ 提示
                ImGui::TextDisabled("[ ]");
                ImGui::SameLine();
                earthmark::drawMarkerIcon(ImGui::GetWindowDrawList(), layer.shape,
                    ImGui::GetCursorScreenPos().x + 9.0f, ImGui::GetCursorScreenPos().y + 9.0f,
                    18.0f, toImU32(layer.iconColor));
                ImGui::Dummy(ImVec2(22.0f, 18.0f)); ImGui::SameLine();
                ImGui::TextDisabled("%s", layer.displayName.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled(u8"[缺Key]");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(u8"缺少 API Key,详见控制台 [EarthConfig] 告警");
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

                // 抓取状态指示(同一行右侧)
                if (enabled && layer.fetchStatus)
                {
                    std::string err;
                    int state = layer.fetchStatus(err);
                    ImGui::SameLine();
                    if (state == 1) ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), u8"●");
                    else if (state == 2)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f), u8"●");
                        if (ImGui::IsItemHovered() && !err.empty())
                            ImGui::SetTooltip("%s", err.c_str());
                    }
                    else if (state == 3) ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), u8"○");
                    else ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), u8"…");
                }
            }

            if (!layer.subtitle.empty() && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", layer.subtitle.c_str());
            ImGui::PopID();
        }
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
    ImGui::Begin(u8"要素详情", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("%s", sel.title.c_str());
    ImGui::Separator();
    // detail 是带 \n 的多行文本
    std::istringstream lines(sel.detail);
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty()) ImGui::TextWrapped("%s", line.c_str());
    }

    if (!sel.url.empty())
    {
        if (ImGui::Button(u8"打开链接", ImVec2(-1.0f, 0.0f))) openUrl(sel.url);
    }
    if (ImGui::Button(u8"关闭", ImVec2(-1.0f, 0.0f))) earthds::clearSelection();
    ImGui::End();
}

void EarthUIHandler::drawEnvironmentPanel(osgVerse::ImGuiManager* m)
{
    if (!_env) return;

    ImGui::SetNextWindowPos(ImVec2(16.0f, ImGui::GetIO().DisplaySize.y - 16.0f),
                            ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::Begin(u8"环境 / 视角", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    float sun = _env->sunAngle;
    if (ImGui::SliderFloat(u8"太阳方位", &sun, 0.0f, 6.2831853f)) _env->sunAngle = sun;
    bool ocean = (_env->oceanOpaque != 0);
    if (ImGui::Checkbox(u8"海洋效果", &ocean)) _env->oceanOpaque = ocean ? 1 : 0;
    if (ImGui::Button(u8"视角归位")) _env->goHome = true;

    // 相机位置(经纬高)
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
    // 底部悬浮 AI 聊天条:紧凑输入 + 发送;历史按需展开(默认折叠,地图优先)。
    // _aiChat 为空(AI 禁用/缺 key)时画禁用态,不画历史/按钮——对无 AI 场景零干扰。
    ImGuiIO& io = ImGui::GetIO();
    const float barWidth = 520.0f;
    const float barHeight = _chatHistoryOpen ? 320.0f : 0.0f;
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 12.0f),
        ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(barWidth, 0.0f), ImGuiCond_Always);
    ImGui::Begin(u8"AI 助手", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                                     | ImGuiWindowFlags_AlwaysAutoResize);

    if (!_aiChat)
    {
        ImGui::TextDisabled(u8"AI 助手未启用(配置 ai 段或设置 API Key 后重启生效)");
        ImGui::End();
        return;
    }

    if (!_aiProviderName.empty())
    {
        ImGui::TextDisabled("%s", _aiProviderName.c_str());
        ImGui::Separator();
    }

    // 会话历史(可折叠)
    if (_chatHistoryOpen)
    {
        std::vector<earthai::ChatEntry> entries = _aiChat->transcript();
        if (entries.empty())
            ImGui::TextDisabled(u8"(暂无对话。可以问:现在哪里有地震?打开灾害图层。)");
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
                    ImGui::TextWrapped(u8"你: %s", e.text.c_str());
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

    // 输入行:历史开关 + 输入框 + 发送
    {
        bool busy = _aiChat->busy();
        ImGui::PushItemWidth(90.0f);
        if (ImGui::Button(_chatHistoryOpen ? u8"收起" : u8"历史"))
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
            ImGui::TextDisabled(u8"思考中");
        }
        else
        {
            if (ImGui::Button(u8"发送") || (send && _chatInput[0] != '\0'))
            {
                std::string text(_chatInput);
                if (!text.empty()) _aiChat->submit(text);
                _chatInput[0] = '\0';
                _chatHistoryOpen = true;  // 发送后自动展开,方便看回复
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
    }
    traverse(node, nv);
}
