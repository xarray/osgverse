#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "scenelogic.h"
#include <imgui/ImGuizmo.h>

SceneLogic::SceneLogic(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _logicWindow = new osgVerse::Window(TR("Scene Logic##ed03"));
    _logicWindow->pos = osg::Vec2(0, 780);
    _logicWindow->sizeMin = osg::Vec2(1920, 300);
    _logicWindow->sizeMax = osg::Vec2(1920, 800);
    _logicWindow->alpha = 0.8f;
    _logicWindow->useMenuBar = false;
    _logicWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _logicWindow->userData = this;
}

bool SceneLogic::show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
{
    bool done = _logicWindow->show(mgr, content);
    {
    }
    _logicWindow->showEnd();
    return done;
}
