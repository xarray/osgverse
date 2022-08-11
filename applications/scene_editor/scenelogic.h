#ifndef MANA_SCENELOGIC_HPP
#define MANA_SCENELOGIC_HPP

#include <osg/Node>
#include <osg/NodeVisitor>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>

class SceneLogic : public osgVerse::ImGuiComponentBase
{
public:
    SceneLogic(osg::Camera* camera, osg::MatrixTransform* mt);
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

protected:
    osg::observer_ptr<osg::Camera> _camera;
    osg::observer_ptr<osg::MatrixTransform> _sceneRoot;

    osg::ref_ptr<osgVerse::Window> _logicWindow;
};

#endif
