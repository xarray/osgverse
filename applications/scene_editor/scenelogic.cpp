#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "scenelogic.h"
#include <imgui/ImGuizmo.h>

SceneLogic::SceneLogic()
{
    _logicWindow = new osgVerse::Window(TR("Scene Logic##ed03"));
    _logicWindow->pos = osg::Vec2(0, 780);
    _logicWindow->sizeMin = osg::Vec2(1920, 300);
    _logicWindow->sizeMax = osg::Vec2(1920, 800);
    _logicWindow->alpha = 0.9f;
    _logicWindow->useMenuBar = false;
    _logicWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar
                        | ImGuiWindowFlags_NoTitleBar;
    _logicWindow->userData = this;
}

bool SceneLogic::show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
{
    bool done = _logicWindow->show(mgr, content);
    {
        if (ImGui::BeginTabBar("##ed0301"))
        {
            if (ImGui::BeginTabItem(TR("Resources##ed030101").c_str()))
            {
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
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    _logicWindow->showEnd();
    return done;
}
