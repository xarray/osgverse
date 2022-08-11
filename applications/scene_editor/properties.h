#ifndef MANA_PROPERTIES_HPP
#define MANA_PROPERTIES_HPP

#include <osg/Node>
#include <osg/NodeVisitor>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>

class Properties : public osgVerse::ImGuiComponentBase
{
public:
    Properties(osg::Camera* camera, osg::MatrixTransform* mt);
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

protected:
    osg::observer_ptr<osg::Camera> _camera;
    osg::observer_ptr<osg::MatrixTransform> _sceneRoot;

    osg::ref_ptr<osgVerse::Window> _propWindow;
};

#endif
