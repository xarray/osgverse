#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <nanoid/nanoid.h>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <cmath>
#include "SceneNavigation.h"

using namespace osgVerse;

SceneNavigation::SceneNavigation()
:   _transformCallback(NULL), _snapOperation(-1),
    _operation(ImGuizmo::TRANSLATE), _gizmoMode(ImGuizmo::WORLD)
{
    _postfix = "##" + nanoid::generate(8);
    for (int i = 0; i < 3; ++i) _snapValue[i] = 0.0f;

    _navigationCube = new osgVerse::NavigationCube;
    _navigationCube->size = osg::Vec2(96, 96);
    _navigationCube->tooltip = TR("Click to switch between navigation cube and compass");

    const std::string btnNames[4] = { "T", "R", "S", "U" };
    const static ImColor normalBtnColor(0.26f, 0.59f, 0.98f, 0.40f);
    for (int i = 0; i < 4; ++i)
    {
        const std::string& name = btnNames[i];
        _transformOp[i] = new osgVerse::Button(TR(name) + _postfix);
        _transformOp[i]->styleHovered = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        _transformOp[i]->styleActive = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        _transformOp[i]->styleNormal = (i == 0) ? _transformOp[i]->styleActive : normalBtnColor;
        _transformOp[i]->size = osg::Vec2(32, 24); _transformOp[i]->styled = true;
        _transformOp[i]->callback = [&, i](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        {
            for (int n = 0; n < 4; ++n) _transformOp[n]->styleNormal = normalBtnColor;
            _transformOp[i]->styleNormal = _transformOp[i]->styleActive;
            switch (i)
            {
            case 0: _operation = ImGuizmo::TRANSLATE; break;
            case 1: _operation = ImGuizmo::ROTATE; break;
            case 2: _operation = ImGuizmo::SCALE; break;
            default: _operation = ImGuizmo::UNIVERSAL; break;
            }
        };
    }

    _transformCoord = new osgVerse::ComboBox(TR("##Coordinate") + _postfix);
    _transformCoord->width = 110;
    _transformCoord->items.push_back("World");
    _transformCoord->items.push_back("Local");
    _transformCoord->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
    {
        osgVerse::ComboBox* cb = static_cast<osgVerse::ComboBox*>(me);
        switch (cb->index)
        {
        case 1: _gizmoMode = ImGuizmo::LOCAL; break;
        default: _gizmoMode = ImGuizmo::WORLD; break;
        }
    };

    _manipulator = new osgVerse::ComboBox(TR("##Manipulator") + _postfix);
    _manipulator->width = 110;
    _manipulator->items.push_back("Trackball");

    _snap = new osgVerse::ComboBox(TR("##Snap") + _postfix);
    _snap->width = 120;
}

void SceneNavigation::updateSnapItems()
{
    _snap->items.clear(); _snapValues.clear();
    _snap->items.push_back(TR("Snap Off")); _snapValues.push_back(0.0f);  // always the first item

    if (_operation == ImGuizmo::ROTATE)  // snapping angle is in degrees
    {
        const char* names[4] = { "Snap 1\xc2\xb0", "Snap 5\xc2\xb0", "Snap 15\xc2\xb0", "Snap 45\xc2\xb0" };
        const float angles[4] = { 1.0f, 5.0f, 15.0f, 45.0f };
        for (int i = 0; i < 4; ++i)
        { _snap->items.push_back(names[i]); _snapValues.push_back(angles[i]); }
    }
    else if (_operation == ImGuizmo::SCALE)
    {
        const char* names[4] = { "Snap 0.05", "Snap 0.1", "Snap 0.25", "Snap 0.5" };
        const float scales[4] = { 0.05f, 0.1f, 0.25f, 0.5f };
        for (int i = 0; i < 4; ++i)
        { _snap->items.push_back(names[i]); _snapValues.push_back(scales[i]); }
    }
    else  // translate and universal, snapping distance in world units
    {
        const char* names[5] = { "Snap 0.1", "Snap 0.5", "Snap 1", "Snap 5", "Snap 10" };
        const float units[5] = { 0.1f, 0.5f, 1.0f, 5.0f, 10.0f };
        for (int i = 0; i < 5; ++i)
        { _snap->items.push_back(names[i]); _snapValues.push_back(units[i]); }
    }
    if (_snap->index >= (int)_snap->items.size()) _snap->index = 0;
    _snapOperation = (int)_operation;
}

bool SceneNavigation::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (_snapOperation != (int)_operation) updateSnapItems();

    bool done = false;
    done |= _manipulator->show(mgr, content); ImGui::SameLine();
    done |= _transformCoord->show(mgr, content); ImGui::SameLine();
    for (int i = 0; i < 4; ++i) { done |= _transformOp[i]->show(mgr, content); ImGui::SameLine(); }
    done |= _snap->show(mgr, content); ImGui::SameLine();

    // Update the navigation cube with the current camera orientation
    if (_camera.valid() && content != NULL)
    {
        osg::Matrix view = _camera->getViewMatrix();
        _navigationCube->viewMatrix = view;

        // Camera forward is the negated 3rd row of the view matrix. The scene is Z-up, so the
        // heading is the forward direction projected on the XY plane, with +Y taken as "north":
        // looking at +Y gives 0, looking at +X (turning right) gives +90 degrees. The dial is
        // rotated by the opposite angle so that the needle, which stays fixed, points to the
        // north of the dial when the camera faces +Y
        osg::Vec3f forward(-view(2, 0), -view(2, 1), -view(2, 2)); forward.normalize();
        _navigationCube->dialAngle = -atan2f(forward.x(), forward.y());
    }
    done |= _navigationCube->show(mgr, content);

    // The gizmo is only shown for selections that can actually be transformed
    osg::MatrixTransform* mt = _transform.valid() ? _transform->asMatrixTransform() : NULL;
    osg::PositionAttitudeTransform* pat = _transform.valid() ?
        _transform->asPositionAttitudeTransform() : NULL;
    if (_camera.valid() && _selection.valid() && (mt != NULL || pat != NULL))
    {
        osg::Matrix matrixD, matrixParent;
        _transform->computeLocalToWorldMatrix(matrixD, NULL);
        if (_transform->getNumParents() > 0)
        {
            osg::MatrixList matrices = _transform->getParent(0)->getWorldMatrices();
            if (!matrices.empty()) matrixParent = matrices[0];
        }

        const float snapAmount = (_snap->index < (int)_snapValues.size()) ?
                                 _snapValues[_snap->index] : 0.0f;
        const float* snapValues = NULL;
        if (snapAmount > 0.0f)
        {
            _snapValue[0] = _snapValue[1] = _snapValue[2] = snapAmount; snapValues = _snapValue;
        }

        osg::Matrixf matrix = osg::Matrixf(matrixD * matrixParent);
        osg::Matrixf view(_camera->getViewMatrix()), proj(_camera->getProjectionMatrix());
        ImGuiIO& io = ImGui::GetIO();
        const osg::Viewport* vp = _camera->getViewport();
        if (vp != NULL)  // use the real 3D viewport, ImGui coordinates are top-left based
            ImGuizmo::SetRect(vp->x(), io.DisplaySize.y - vp->y() - vp->height(),
                              vp->width(), vp->height());
        else
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::SetOrthographic(proj(3, 3) > 0.5f);  // 1 for orthographic, 0 for perspective
        done = ImGuizmo::Manipulate(view.ptr(), proj.ptr(), (ImGuizmo::OPERATION)_operation,
                                    (ImGuizmo::MODE)_gizmoMode, matrix.ptr(), NULL, snapValues);
        if (done)
        {
            matrix = matrix * osg::Matrix::inverse(matrixParent);
            if (mt != NULL)
                mt->setMatrix(matrix);
            else  // pat != NULL
            {
                osg::Vec3 pos, scale; osg::Quat rot, so; matrix.decompose(pos, rot, scale, so);
                pat->setPosition(pos); pat->setAttitude(rot); pat->setScale(scale);
            }
            if (_transformCallback) _transformCallback(this, _transform.get());
        }
    }
    return done;
}
