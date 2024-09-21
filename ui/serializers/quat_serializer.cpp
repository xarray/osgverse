#include "../SerializerInterface.h"
#include "../../modeling/Math.h"
using namespace osgVerse;

class QuatSerializerInterface : public SerializerInterface
{
public:
    QuatSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        _typeSelector = new ComboBox(TR("Type") + _postfix);
        _typeSelector->tooltip = TR("Rotation editing type");
        _typeSelector->items.push_back(TR("Euler Angles"));
        _typeSelector->items.push_back(TR("Quaternions"));
        _typeSelector->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            if (_typeSelector->index == 0)
            {
                _vector->name = TR("Euler") + _postfix; _vector->vecNumber = 3;
                _vector->tooltip = tooltip(_property, "Eulers");
            }
            else
            {
                _vector->name = TR("Quat") + _postfix; _vector->vecNumber = 4;
                _vector->tooltip = tooltip(_property, "Quaternions");
            }
            updateVectorField(_quat);
        };

        _vector = new InputVectorField(TR("Euler") + _postfix); _vector->vecNumber = 3;
        _vector->tooltip = tooltip(_property, "Eulers");
        _vector->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec4d value; _vector->getVector(value);
            if (_typeSelector->index == 0)
            {
                osg::Matrix r = osg::Matrixd::rotate(osg::inDegrees(value[2]), osg::Z_AXIS)
                              * osg::Matrixd::rotate(osg::inDegrees(value[0]), osg::X_AXIS)
                              * osg::Matrixd::rotate(osg::inDegrees(value[1]), osg::Y_AXIS);
                _quat = r.getRotate();
            }
            else
                _quat = osg::Quat(value[0], value[1], value[2], value[3]);
            _entry->setProperty(_object.get(), _property.name, _quat);
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            _entry->getProperty(_object.get(), _property.name, _quat);
            updateVectorField(_quat);
        }
        bool edited = _typeSelector->show(mgr, content);
        return edited | _vector->show(mgr, content);
    }

protected:
    void updateVectorField(const osg::Quat& q)
    {
        if (_typeSelector->index == 0)
        {
            osg::Vec3d eulers = computeHPRFromQuat(q);
            _vector->setVector(osg::Vec3d(eulers[1], eulers[2], eulers[0]));
        }
        else
            _vector->setVector(q.asVec4());
    }

    osg::ref_ptr<InputVectorField> _vector;
    osg::ref_ptr<ComboBox> _typeSelector;
    osg::Quat _quat;
};

class PlaneSerializerInterface : public SerializerInterface
{
public:
    PlaneSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        _normal = new InputVectorField(TR("N") + _postfix); _normal->vecNumber = 3;
        _normal->tooltip = tooltip(_property, "Plane Normal");
        _normal->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d n; _normal->getVector(n); _plane.set(n, -_distance->value);
            _entry->setProperty(_object.get(), _property.name, _plane);
        };

        _distance = new InputValueField(TR("D") + _postfix);
        _distance->tooltip = tooltip(_property, "Distance to origin");
        _distance->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d n; _normal->getVector(n); _plane.set(n, -_distance->value);
            _entry->setProperty(_object.get(), _property.name, _plane);
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            _entry->getProperty(_object.get(), _property.name, _plane);
            _normal->setVector(_plane.getNormal());
            _distance->value = -_plane[3];
        }
        bool edited = _normal->show(mgr, content);
        return edited | _distance->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputVectorField> _normal;
    osg::ref_ptr<InputValueField> _distance;
    osg::Plane _plane;
};

REGISTER_SERIALIZER_INTERFACE(QUAT, QuatSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(PLANE, PlaneSerializerInterface)
