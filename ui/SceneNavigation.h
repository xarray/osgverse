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
        void setSelection(osg::Node* n) { _selection = n; _transform = dynamic_cast<osg::Transform*>(n); }

        typedef std::function<void(SceneNavigation*, osg::Transform*)> TransformCallback;
        void setTransformAction(TransformCallback cb) { _transformCallback = cb; }

        /** Get the navigation cube/compass widget, to change its texture or display mode */
        NavigationCube* getNavigationCube() { return _navigationCube.get(); }

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);

    protected:
        void updateSnapItems();

        osg::observer_ptr<osg::Camera> _camera;
        osg::observer_ptr<osg::Node> _selection;
        osg::observer_ptr<osg::Transform> _transform;
        osg::ref_ptr<osgVerse::Button> _transformOp[4];
        osg::ref_ptr<osgVerse::ComboBox> _transformCoord, _manipulator, _snap;
        osg::ref_ptr<osgVerse::NavigationCube> _navigationCube;

        TransformCallback _transformCallback;
        std::vector<float> _snapValues; float _snapValue[3];
        std::string _postfix; int _snapOperation;
        unsigned int _operation, _gizmoMode;
    };
}

#endif
