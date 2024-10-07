#include "../SerializerInterface.h"
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
using namespace osgVerse;

template<typename T>
class BBoxSerializerInterface : public SerializerInterface
{
public:
    BBoxSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        _center = new InputVectorField(TR("Center") + _postfix); _center->vecNumber = 3;
        _center->tooltip = tooltip(_property, "Bounding box center");
        _center->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d c, s; _center->getVector(c); _sizes->getVector(s);
            _bbox._min = c - s * 0.5; _bbox._max = c + s * 0.5;
            if (_entry->setProperty(_object.get(), _property.name, _bbox)) doneEditing();
        };

        _sizes = new InputVectorField(TR("Size") + _postfix); _sizes->vecNumber = 3;
        _sizes->tooltip = tooltip(_property, "Bounding box extent");
        _sizes->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d c, s; _center->getVector(c); _sizes->getVector(s);
            _bbox._min = c - s * 0.5; _bbox._max = c + s * 0.5;
            if (_entry->setProperty(_object.get(), _property.name, _bbox)) doneEditing();
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            _entry->getProperty(_object.get(), _property.name, _bbox);
            _center->setVector(_bbox.center()); _sizes->setVector(_bbox._max - _bbox._min);
        }
        bool edited = _center->show(mgr, content);
        return edited | _sizes->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputVectorField> _center;
    osg::ref_ptr<InputVectorField> _sizes;
    T _bbox;
};

template<typename T>
class BSphereSerializerInterface : public SerializerInterface
{
public:
    BSphereSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _center = new InputVectorField(TR("Center") + _postfix); _center->vecNumber = 3;
        _center->tooltip = tooltip(_property, "Bounding sphere center");
        _center->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d c; _center->getVector(c); _bsphere.set(c, _radius->value);
            if (_entry->setProperty(_object.get(), _property.name, _bsphere)) doneEditing();
        };

        _radius = new InputValueField(TR("Radius") + _postfix);
        _radius->tooltip = tooltip(_property, "Bounding sphere radius");
        _radius->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d c; _center->getVector(c); _bsphere.set(c, _radius->value);
            if (_entry->setProperty(_object.get(), _property.name, _bsphere)) doneEditing();
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            _entry->getProperty(_object.get(), _property.name, _bsphere);
            _center->setVector(_bsphere.center()); _radius->value = _bsphere.radius();
        }
        bool edited = _center->show(mgr, content);
        return edited | _radius->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputVectorField> _center;
    osg::ref_ptr<InputValueField> _radius;
    T _bsphere;
};

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
typedef BBoxSerializerInterface<osg::BoundingBoxf> BBoxfSerializerInterface;
typedef BBoxSerializerInterface<osg::BoundingBoxd> BBoxdSerializerInterface;
typedef BSphereSerializerInterface<osg::BoundingSpheref> BSpherefSerializerInterface;
typedef BSphereSerializerInterface<osg::BoundingSphered> BSpheredSerializerInterface;

REGISTER_SERIALIZER_INTERFACE(BOUNDINGBOXF, BBoxfSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(BOUNDINGBOXD, BBoxdSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(BOUNDINGSPHEREF, BSpherefSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(BOUNDINGSPHERED, BSpheredSerializerInterface)
#endif
