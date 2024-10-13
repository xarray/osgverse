#ifndef MANA_UI_SCENENAVIGATION_HPP
#define MANA_UI_SCENENAVIGATION_HPP

#include "ImGui.h"
#include "ImGuiComponents.h"
#include <osg/Camera>
#include <map>

namespace osgVerse
{
    class SceneNavigation : public ImGuiComponentBase
    {
    public:
        SceneNavigation();
        void setCamera(osg::Camera* camera) { _camera = camera; }
        void setSelection(osg::Transform* t) { _transform = t; }

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);

    protected:
        osg::observer_ptr<osg::Camera> _camera;
        osg::observer_ptr<osg::Transform> _transform;
        osg::ref_ptr<osgVerse::Button> _transformOp[4];
        osg::ref_ptr<osgVerse::ComboBox> _transformCoord, _manipulator;
        osg::ref_ptr<osgVerse::ImageButton> _navigationImage;
        std::string _postfix;
        unsigned int _operation, _gizmoMode;
    };
}

#endif
