#include "../SerializerInterface.h"
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
using namespace osgVerse;

template<typename T>
class BBoxSerializerInterface : public SerializerInterface
{
public:
    BBoxSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _check = new CheckBox(TR(_property.name) + _postfix, false);
        _check->tooltip = prop.ownerClass + "::set" + prop.name + "()";
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

template<typename T>
class BSphereSerializerInterface : public SerializerInterface
{
public:
    BSphereSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _check = new CheckBox(TR(_property.name) + _postfix, false);
        _check->tooltip = prop.ownerClass + "::set" + prop.name + "()";
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
