#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "scenelogic.h"

#include <imgui/ImGuizmo.h>
#include <ctime>

class ConsoleHandler : public osg::NotifyHandler
{
public:
    ConsoleHandler(SceneLogic* logic) : _logic(logic) {}
    
    virtual void notify(osg::NotifySeverity severity, const char* message)
    {
        if (_logic.valid())
        {
            std::string msg(message);
            if (msg.empty()) return;
            else if (msg.back() == '\n') msg.back() = '\0';

            SceneLogic::NotifyData nd((int)severity, msg);
            nd.dateTime = getDateTimeTick();
            _logic->addNotify(nd);
        }
        std::cout << "Lv-" << severity << ": " << message;
    }

    std::string getDateTimeTick()
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

protected:
    osg::observer_ptr<SceneLogic> _logic;
};

SceneLogic::SceneLogic()
{
    osg::setNotifyHandler(new ConsoleHandler(this));
    osg::setNotifyLevel(osg::NOTICE);

    _logicWindow = new osgVerse::Window(TR("Scene Logic##ed03"));
    _logicWindow->pos = osg::Vec2(0.0f, 0.75f);
    _logicWindow->size = osg::Vec2(1.0f, 0.25f);
    _logicWindow->alpha = 0.9f;
    _logicWindow->useMenuBar = false;
    _logicWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar
                        | ImGuiWindowFlags_NoTitleBar;
    _logicWindow->userData = this;
}

bool SceneLogic::show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
{
    bool done = _logicWindow->show(mgr, content);
    if (done)
    {
        if (ImGui::BeginTabBar("##ed0301"))
        {
            if (ImGui::BeginTabItem(TR("Resources##ed030101").c_str()))
            {
                /*static osg::ref_ptr<osgVerse::VirtualKeyboard> vk = new osgVerse::VirtualKeyboard;
                if (vk->keyList.empty()) vk->create("../misc/system-py.db", "../misc/learn-py.db");
                vk->show(mgr, content);*/  // test only

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(TR("Timeline##ed030102").c_str()))
            {
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(TR("Spider##ed030103").c_str()))
            {
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(TR("Console##ed030103").c_str()))
            {
                const float footerToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
                ImGui::BeginChild("ConsoleRegion##ed03010300", ImVec2(0.0f, -footerToReserve),
                                  false, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f)); // Tighten spacing

                for (size_t i = 0; i < _notifyData.size(); ++i)
                {
                    ImVec4 color; bool hasColor = false;
                    const NotifyData& nd = _notifyData[i];

                    if (nd.level < osg::WARN) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); hasColor = true; }
                    else if (nd.level == osg::WARN) { color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); hasColor = true; }
                    else if (nd.level > osg::NOTICE) { color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); hasColor = true; }

                    if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::Selectable((nd.dateTime + ": " + nd.text).c_str());
                    if (hasColor) ImGui::PopStyleColor();
                }

                ImGui::PopStyleVar();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        _logicWindow->showEnd();
    }
    return done;
}
