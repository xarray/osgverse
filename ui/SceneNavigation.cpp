#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <nanoid/nanoid.h>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include "SceneNavigation.h"

using namespace osgVerse;

SceneNavigation::SceneNavigation()
:   _transformCallback(NULL),
    _operation(ImGuizmo::TRANSLATE), _gizmoMode(ImGuizmo::WORLD)
{
    _postfix = "##" + nanoid::generate(8);
    _navigationImage = new osgVerse::ImageButton("Navigation");
    _navigationImage->size = osg::Vec2(96, 96);

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
    _transformCoord->width = 120;
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
    _manipulator->width = 120;
    _manipulator->items.push_back("Trackball");
}

bool SceneNavigation::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    done |= _manipulator->show(mgr, content); ImGui::SameLine();
    done |= _transformCoord->show(mgr, content); ImGui::SameLine();
    for (int i = 0; i < 4; ++i) { done |= _transformOp[i]->show(mgr, content); ImGui::SameLine(); }
    done |= _navigationImage->show(mgr, content);

    if (_camera.valid() && _selection.valid())
    {
        osg::Matrix matrixD, matrixParent;
        if (_transform.valid())
        {
            _transform->computeLocalToWorldMatrix(matrixD, NULL);
            if (_transform->getNumParents() > 0)
            {
                osg::MatrixList matrices = _transform->getParent(0)->getWorldMatrices();
                if (!matrices.empty()) matrixParent = matrices[0];
            }
        }
        else
        {
            osg::MatrixList matrices = _selection->getWorldMatrices();
            if (!matrices.empty()) matrixParent = matrices[0];
        }

        osg::Matrixf matrix = osg::Matrixf(matrixD * matrixParent);
        osg::Matrixf view(_camera->getViewMatrix()), proj(_camera->getProjectionMatrix());
        ImGuiIO& io = ImGui::GetIO(); ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        done = ImGuizmo::Manipulate(view.ptr(), proj.ptr(), (ImGuizmo::OPERATION)_operation,
                                    (ImGuizmo::MODE)_gizmoMode, matrix.ptr());  // TODO: snap, local/world
        if (done && _transform.valid())
        {
            matrix = matrix * osg::Matrix::inverse(matrixParent);
            if (_transform->asMatrixTransform())
                _transform->asMatrixTransform()->setMatrix(matrix);
            else if (_transform->asPositionAttitudeTransform())
            {
                osg::Vec3 pos, scale; osg::Quat rot, so; matrix.decompose(pos, rot, scale, so);
                _transform->asPositionAttitudeTransform()->setPosition(pos);
                _transform->asPositionAttitudeTransform()->setAttitude(rot);
                _transform->asPositionAttitudeTransform()->setScale(scale);
            }
            if (_transformCallback) _transformCallback(this, _transform.get());
        }
    }
    return done;
}
