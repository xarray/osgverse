#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "properties.h"

#include <imgui/ImGuizmo.h>
#include <ui/CommandHandler.h>
using namespace osgVerse;

Properties::Properties(osg::Camera* cam, osg::MatrixTransform* mt)
    : _camera(cam), _sceneRoot(mt)
{
    _propWindow = new osgVerse::Window(TR("Properties##ed02"));
    _propWindow->pos = osg::Vec2(1600, 0);
    _propWindow->sizeMin = osg::Vec2(320, 780);
    _propWindow->sizeMax = osg::Vec2(640, 780);
    _propWindow->alpha = 0.8f;
    _propWindow->useMenuBar = true;
    _propWindow->flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    _propWindow->userData = this;
}

bool Properties::handleCommand(osgVerse::CommandData* cmd)
{
    /* Refresh properties:
       - cmd->object (parent) is the node/drawable whose properties are updated
       - cmd->value (string): to-be-updated component's name, or empty to update all
    */
    osg::Object* target = static_cast<osg::Object*>(cmd->object.get());
    printf("====== %s\n", target->className());
    return true;
}

bool Properties::show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content)
{
    bool done = _propWindow->show(mgr, content);
    {
    }
    _propWindow->showEnd();
    return done;
}
