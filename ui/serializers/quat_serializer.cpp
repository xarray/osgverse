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
        _check = new CheckBox(TR(_property.name) + _postfix, false);
        _check->tooltip = tooltip(_property);
        //_check->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        //{ _entry->setProperty(_object.get(), _property.name, _check->value); };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        //if (isDirty()) _entry->getProperty(_object.get(), _property.name, _check->value);
        return _check->show(mgr, content);
    }

protected:
    osg::ref_ptr<CheckBox> _check;
};

REGISTER_SERIALIZER_INTERFACE(QUAT, QuatSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(PLANE, PlaneSerializerInterface)
