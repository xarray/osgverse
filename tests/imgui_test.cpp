#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <imgui/imgui.h>
#include <ui/ImGui.h>
#include <iostream>
#include <sstream>

struct MyContentHandler : public osgVerse::ImGuiContentHandler
{
    MyContentHandler() : _selectID(-1), _rotateValue(0.0f), _rotateDir(0.0f) {}
    int _selectID; float _rotateValue, _rotateDir;

    virtual void runInternal(osgVerse::ImGuiManager* mgr)
    {
        ImTextureID icon = ImGuiTextures["icon"];
        const ImGuiViewport* view = ImGui::GetMainViewport();
        ImGui::PushFont(ImGuiFonts["LXGWWenKaiLite-Regular"]);

        int xPos = 0, yPos = 0;
        int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                  | ImGuiWindowFlags_NoCollapse;

        setupWindow(ImVec2(view->WorkPos.x + xPos, view->WorkPos.y + yPos), ImVec2(400, 250));
        if (ImGui::Begin(u8"图标状态", NULL, flags))
        {
            ImGui::SetWindowFontScale(1.2f);
            if (ImGui::Button(u8"向左转", ImVec2(120, 60))) _rotateDir = -1.0f;
            if (ImGui::Button(u8"停止", ImVec2(120, 60))) _rotateDir = 0.0f;
            if (ImGui::Button(u8"向右转", ImVec2(120, 60))) _rotateDir = 1.0f;

            imageRotated(icon, ImVec2(280, 140), ImVec2(180, 180), _rotateValue);
            _rotateValue += _rotateDir * 0.002f;
        }
        ImGui::End(); yPos += 250;

        setupWindow(ImVec2(view->WorkPos.x + xPos, view->WorkPos.y + yPos), ImVec2(400, 450));
        if (ImGui::Begin(u8"对象图层", NULL, flags))
        {
            ImGui::SetWindowFontScale(1.2f);
            if (ImGui::TreeNodeEx(u8"节点列表", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginListBox(u8"", ImVec2(360, 260)))
                {
                    for (int i = 0; i < 15; ++i)
                    {
                        if (ImGui::Selectable((u8"节点" + std::to_string(i)).c_str(), i == _selectID))
                        { _selectID = i; }
                    }
                    ImGui::EndListBox();
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx(u8"用户列表", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginListBox(u8"", ImVec2(360, 120)))
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        ImGui::Selectable((u8"用户" + std::to_string(i)).c_str(), false);
                        ImGui::SameLine(200); ImGui::Text((u8"xArray" + std::to_string(i)).c_str());
                    }
                    ImGui::EndListBox();
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();

        ImGui::PopFont();
    }

    static ImVec2 imAdd(const ImVec2& lhs, const ImVec2& rhs)
    { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

    static ImVec2 imRotate(const ImVec2& v, float cos_a, float sin_a)
    { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }

    static void imageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle)
    {
        ImDrawList* dList = ImGui::GetWindowDrawList();
        float cos_a = cosf(angle), sin_a = sinf(angle);
        ImVec2 pos[4] =
        {
            imAdd(center, imRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a)),
            imAdd(center, imRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a)),
            imAdd(center, imRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)),
            imAdd(center, imRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a))
        };
        ImVec2 uvs[4] = { ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f) };
        dList->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
    }

    void setupWindow(const ImVec2& pos, const ImVec2& size, float alpha = 0.6f)
    {
        ImGui::SetNextWindowPos(pos); ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowBgAlpha(alpha);
    }
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());

    // The ImGui setup
    osg::ref_ptr<osgVerse::ImGuiManager> imgui = new osgVerse::ImGuiManager;
    imgui->setChineseSimplifiedFont("../misc/LXGWWenKaiLite-Regular.ttf");
    imgui->setGuiTexture("icon", "Images/osg128.png");
    imgui->initialize(new MyContentHandler);
    imgui->addToView(&viewer);
    
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
