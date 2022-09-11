#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "PropertyInterface.h"
#include "ImGuiComponents.h"
#include <imgui/ImGuizmo.h>
using namespace osgVerse;

class BasicPropertyItem : public PropertyItem
{
public:
    BasicPropertyItem()
    {
        _name = new InputField(ImGuiComponentBase::TR("Name##prop0001"));
        _name->placeholder = ImGuiComponentBase::TR(
            (_type == NodeType) ? "Node name" : "Drawable name");
        _name->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };

        _mask = new InputValueField(ImGuiComponentBase::TR("Mask##prop0002"));
        _mask->type = InputValueField::UIntValue;
        _mask->flags = ImGuiInputTextFlags_CharsHexadecimal;
        _mask->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };
    }

    virtual std::string title() const { return (_type == NodeType) ? "Node Basics" : "Drawable Basics"; }
    virtual bool needRefreshUI() const { return true; }

    virtual void updateTarget(ImGuiComponentBase* c)
    {
        if (_type == NodeType)
        {
            osg::Node* n = static_cast<osg::Node*>(_target.get());
            if (!c)
            {
                _name->value = n->getName();
                _mask->value = n->getNodeMask();
            }
            else
            {
                if (c == _name) n->setName(((InputField*)c)->value);
                else if (c == _mask) n->setNodeMask(((InputValueField*)c)->value);
            }
        }
        else if (_type == DrawableType)
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(_target.get());
            if (!c) _name->value = d->getName();
            else if (c == _name) d->setName(((InputField*)c)->value);
        }
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        bool updated = _name->show(mgr, content);
        if (_type == NodeType) updated |= _mask->show(mgr, content);
        return updated;
    }

protected:
    osg::ref_ptr<InputField> _name;
    osg::ref_ptr<InputValueField> _mask;
};

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
            if (toSet) n->setMatrix(_matrix * getInvParentMatrix());
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
                n->setPosition(t); n->setScale(s); n->setAttitude(r);
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
            update3 = ImGuizmo::Manipulate(view.ptr(), proj.ptr(), _gizmoOperation, _gizmoMode,
                                           _matrix.ptr(), NULL, NULL/*(useSnap ? snapVec.ptr() : NULL)*/);
            if (update3) updateTarget(_methods);
        }
        return updated || updated2 || update3;
    }

protected:
    osg::ref_ptr<RadioButtonGroup> _methods, _coordinates;
    osg::ref_ptr<InputVectorField> _translation, _euler, _scale;
    osg::Matrixf _matrix;
    ImGuizmo::OPERATION _gizmoOperation;
    ImGuizmo::MODE _gizmoMode;
};

///////////////////////////

PropertyItemManager* PropertyItemManager::instance()
{
    static osg::ref_ptr<PropertyItemManager> s_instance = new PropertyItemManager;
    return s_instance.get();
}

PropertyItemManager::PropertyItemManager()
{
    _standardItemMap[BasicNodeItem] = new BasicPropertyItem;
    _standardItemMap[BasicDrawableItem] = new BasicPropertyItem;
    _standardItemMap[TransformItem] = new TransformPropertyItem;
}

PropertyItem* PropertyItemManager::getStandardItem(StandardItemType t)
{
    if (_standardItemMap.find(t) == _standardItemMap.end()) return NULL;
    else return _standardItemMap[t];
}

PropertyItem* PropertyItemManager::getExtendedItem(const std::string& t)
{
    if (_extendedItemMap.find(t) == _extendedItemMap.end()) return NULL;
    else return _extendedItemMap[t];
}
