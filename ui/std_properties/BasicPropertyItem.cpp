#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "../PropertyInterface.h"
#include "../CommandHandler.h"
#include "../ImGuiComponents.h"
#include <imgui/ImGuizmo.h>
using namespace osgVerse;

class BasicPropertyItem : public PropertyItem
{
public:
    BasicPropertyItem()
    {
        _name = new InputField(ImGuiComponentBase::TR("Name##prop0001"));
        _name->placeholder = ImGuiComponentBase::TR("Object name");
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
                if (c == _name)
                {
                    std::string objName = ((InputField*)c)->value;
                    CommandBuffer::instance()->add(SetValueCommand, n, std::string("n_name"), objName);
                }
                else if (c == _mask)
                {
                    unsigned int mask = ((InputValueField*)c)->value;
                    CommandBuffer::instance()->add(SetValueCommand, n, std::string("n_mask"), mask);
                }
            }
        }
        else if (_type == DrawableType)
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(_target.get());
            if (!c) _name->value = d->getName();
            else if (c == _name)
            {
                std::string objName = ((InputField*)c)->value;
                CommandBuffer::instance()->add(SetValueCommand, d, std::string("d_name"), objName);
            }
        }
    }

    osg::BoundingBox getBoundingBox()
    {
        osg::BoundingBox bb, bbW; osg::ref_ptr<osg::RefMatrix> l2w;
        if (_type == NodeType)
        {
            osg::Node* n = static_cast<osg::Node*>(_target.get());
            osg::ComputeBoundsVisitor cbv; n->accept(cbv);
            if (n->getNumParents() > 0)
                l2w = new osg::RefMatrix(n->getParent(0)->getWorldMatrices()[0]);
            bb = cbv.getBoundingBox();
        }
        else if (_type == DrawableType)
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(_target.get());
            if (d->getNumParents() > 0)
                l2w = new osg::RefMatrix(d->getParent(0)->getWorldMatrices()[0]);
#if OSG_VERSION_GREATER_THAN(3, 2, 2)
            bb = d->getBoundingBox();
#else
            bb = d->getBound();
#endif
        }

        if (!l2w) return bb;
        bbW.expandBy(bb.corner(0) * (*l2w)); bbW.expandBy(bb.corner(1) * (*l2w));
        bbW.expandBy(bb.corner(2) * (*l2w)); bbW.expandBy(bb.corner(3) * (*l2w));
        bbW.expandBy(bb.corner(4) * (*l2w)); bbW.expandBy(bb.corner(5) * (*l2w));
        bbW.expandBy(bb.corner(6) * (*l2w)); bbW.expandBy(bb.corner(7) * (*l2w)); return bbW;
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        /*if (_camera.valid() && _target.valid())
        {
            ImGuiIO& io = ImGui::GetIO(); ImDrawList* drawer = ImGui::GetBackgroundDrawList();
            osg::Matrixf vpw = _camera->getViewMatrix() * _camera->getProjectionMatrix()
                             * osg::Matrix::translate(1.0f, 1.0f, 1.0f)
                             * osg::Matrix::scale(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f, 0.5f)
                             * osg::Matrix::translate(0.0f, 0.0f, 0.0f);
            osg::BoundingBox bb = getBoundingBox();

            osg::Vec2 vMin(9999.0f, 9999.0f), vMax(-9999.0f, -9999.0f);
            for (int i = 0; i < 8; ++i)
            {
                osg::Vec3 v = bb.corner(i) * vpw; v[1] = io.DisplaySize.y - v[1];
                if (v[0] < vMin[0]) vMin[0] = v[0]; if (v[0] > vMax[0]) vMax[0] = v[0];
                if (v[1] < vMin[1]) vMin[1] = v[1]; if (v[1] > vMax[1]) vMax[1] = v[1];
            }
            drawer->AddRect(ImVec2(vMin[0], vMin[1]), ImVec2(vMax[0], vMax[1]), IM_COL32(0, 255, 0, 100));
            //ImGuizmo::DrawGrid(view.ptr(), proj.ptr(), _initMatrix.ptr(), 10.0f);
        }*/

        bool updated = _name->show(mgr, content);
        if (_type == NodeType) updated |= _mask->show(mgr, content);
        return updated;
    }

protected:
    osg::ref_ptr<InputField> _name;
    osg::ref_ptr<InputValueField> _mask;
    osg::Matrixf _initMatrix;
};

PropertyItem* createBasicPropertyItem()
{ return new BasicPropertyItem; }
