#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "../PropertyInterface.h"
#include "../ImGuiComponents.h"
#include <imgui/ImGuizmo.h>
using namespace osgVerse;

class TransformPropertyItem : public PropertyItem
{
public:
    TransformPropertyItem()
    {
        _gizmoOperation = ImGuizmo::TRANSLATE;
        _gizmoMode = ImGuizmo::LOCAL;

        _methods = new RadioButtonGroup();
        _methods->buttons.push_back(RadioButtonGroup::RadioData{
            ImGuiComponentBase::TR("Translation##prop0101") });
        _methods->buttons.push_back(RadioButtonGroup::RadioData{
            ImGuiComponentBase::TR("Euler##prop0102") });
        _methods->buttons.push_back(RadioButtonGroup::RadioData{
            ImGuiComponentBase::TR("Scale##prop0103") });

        _coordinates = new RadioButtonGroup();
        _coordinates->buttons.push_back(RadioButtonGroup::RadioData{
            ImGuiComponentBase::TR("Local##prop0104") });
        _coordinates->buttons.push_back(RadioButtonGroup::RadioData{
            ImGuiComponentBase::TR("Global##prop0105") });

        _translation = new InputVectorField(ImGuiComponentBase::TR("T##prop0106"));
        _euler = new InputVectorField(ImGuiComponentBase::TR("R##prop0107"));
        _scale = new InputVectorField(ImGuiComponentBase::TR("S##prop0108"));
        _translation->vecNumber = 3; _euler->vecNumber = 3; _scale->vecNumber = 3;
    }

    virtual std::string title() const { return "Transformation"; }
    virtual bool needRefreshUI() const { return false; }
    virtual void updateTarget(ImGuiComponentBase* c) { setOrGetTargetMatrix(c != NULL); }

    osg::Matrix getInvParentMatrix()
    {
        osg::Node* n = static_cast<osg::Node*>(_target.get());
        if (n->getNumParents() > 0)
            return osg::Matrix::inverse(n->getParent(0)->getWorldMatrices()[0]);
        return osg::Matrix();
    }

    void setOrGetTargetMatrix(bool toSet)
    {
        if (_type == MatrixType)
        {
            osg::MatrixTransform* n = static_cast<osg::MatrixTransform*>(_target.get());
            if (toSet) n->setMatrix(_matrix * getInvParentMatrix());  // TODO: set transform command
            else _matrix = n->getWorldMatrices()[0];
        }
        else if (_type == PoseType)
        {
            osg::PositionAttitudeTransform* n =
                static_cast<osg::PositionAttitudeTransform*>(_target.get());
            if (toSet)
            {
                osg::Vec3 t, s; osg::Quat r, so;
                osg::Matrix(_matrix * getInvParentMatrix()).decompose(t, r, s, so);
                n->setPosition(t); n->setScale(s); n->setAttitude(r);  // TODO: set transform command
            }
            else
                _matrix = n->getWorldMatrices()[0];
        }
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        bool updated = _methods->show(mgr, content);
        if (updated)
        {
            if (_methods->value == 0) _gizmoOperation = ImGuizmo::TRANSLATE;
            else if (_methods->value == 1) _gizmoOperation = ImGuizmo::ROTATE;
            else _gizmoOperation = ImGuizmo::SCALE;
        }

        osg::Vec3f vecT, vecR, vecS; osg::Matrixf matrix = _matrix, invP;
        if (_gizmoMode == ImGuizmo::LOCAL) { invP = getInvParentMatrix(); matrix = _matrix * invP; }
        ImGuizmo::DecomposeMatrixToComponents(matrix.ptr(), vecT.ptr(), vecR.ptr(), vecS.ptr());

        _translation->setVector(vecT); _euler->setVector(vecR); _scale->setVector(vecS);
        updated |= _translation->show(mgr, content);
        updated |= _euler->show(mgr, content);
        updated |= _scale->show(mgr, content);
        if (updated)
        {
            _translation->getVector(vecT); _euler->getVector(vecR); _scale->getVector(vecS);
            ImGuizmo::RecomposeMatrixFromComponents(vecT.ptr(), vecR.ptr(), vecS.ptr(), matrix.ptr());
            if (_gizmoMode == ImGuizmo::LOCAL) _matrix = matrix * osg::Matrix::inverse(invP);
            else _matrix = matrix; updateTarget(_translation);
        }

        bool updated2 = _coordinates->show(mgr, content), update3 = false;
        if (updated2)
        {
            if (_coordinates->value == 0) _gizmoMode = ImGuizmo::LOCAL;
            else _gizmoMode = ImGuizmo::WORLD;
        }

        if (_camera.valid() && _target.valid())
        {
            osg::Matrixf view(_camera->getViewMatrix()), proj(_camera->getProjectionMatrix());
            ImGuiIO& io = ImGui::GetIO(); ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            ImGuizmo::AllowAxisFlip(false);

            update3 = ImGuizmo::Manipulate(view.ptr(), proj.ptr(), _gizmoOperation, _gizmoMode,
                _matrix.ptr(), NULL, NULL/*(useSnap ? snapVec.ptr() : NULL)*/);
            if (update3) updateTarget(_methods);
        }
        return updated || updated2 || update3;
    }

protected:
    osg::ref_ptr<RadioButtonGroup> _methods, _coordinates;
    osg::ref_ptr<InputVectorField> _translation, _euler, _scale;
    osg::ref_ptr<CheckBox> _showBox;
    osg::Matrixf _matrix;
    ImGuizmo::OPERATION _gizmoOperation;
    ImGuizmo::MODE _gizmoMode;
};

PropertyItem* createTransformPropertyItem()
{ return new TransformPropertyItem; }
