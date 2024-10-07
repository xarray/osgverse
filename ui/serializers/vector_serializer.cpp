#include "../SerializerInterface.h"
using namespace osgVerse;

template<typename T>
class VecSerializerInterface : public SerializerInterface
{
public:
    VecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _value = new InputVectorField(TR(_property.name) + _postfix);
        _value->tooltip = tooltip(_property);
        _value->vecNumber = T::num_components;
        _value->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            T value; _value->getVector(value);
            if (_entry->setProperty(_object.get(), _property.name, value)) doneEditing();
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            T value; _entry->getProperty(_object.get(), _property.name, value);
            _value->setVector(value);
        }
        return _value->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputVectorField> _value;
};

template<typename T>
class CharVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    CharVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    {
        VecSerializerInterface<T>::_value->type = InputValueField::IntValue;
        VecSerializerInterface<T>::_value->minValue = -128;
        VecSerializerInterface<T>::_value->maxValue = 127;
    }
};

template<typename T>
class UCharVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    UCharVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    {
        VecSerializerInterface<T>::_value->type = InputValueField::UIntValue;
        VecSerializerInterface<T>::_value->minValue = 0;
        VecSerializerInterface<T>::_value->maxValue = 255;
    }
};

template<typename T>
class ShortVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    ShortVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    {
        VecSerializerInterface<T>::_value->type = InputValueField::IntValue;
        VecSerializerInterface<T>::_value->minValue = -32768;
        VecSerializerInterface<T>::_value->maxValue = 32767;
    }
};

template<typename T>
class UShortVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    UShortVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    {
        VecSerializerInterface<T>::_value->type = InputValueField::UIntValue;
        VecSerializerInterface<T>::_value->minValue = 0;
        VecSerializerInterface<T>::_value->maxValue = 65535;
    }
};

template<typename T>
class IntVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    IntVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    { VecSerializerInterface<T>::_value->type = InputValueField::IntValue; }
};

template<typename T>
class UIntVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    UIntVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    {
        VecSerializerInterface<T>::_value->type = InputValueField::UIntValue;
        VecSerializerInterface<T>::_value->flags = ImGuiInputTextFlags_CharsHexadecimal;
    }
};

template<typename T>
class FloatVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    FloatVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    { VecSerializerInterface<T>::_value->type = InputValueField::FloatValue; }
};

template<typename T>
class DoubleVecSerializerInterface : public VecSerializerInterface<T>
{
public:
    DoubleVecSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : VecSerializerInterface<T>(obj, entry, prop)
    { VecSerializerInterface<T>::_value->type = InputValueField::DoubleValue; }
};

typedef FloatVecSerializerInterface<osg::Vec2f> Vec2fSerializerInterface;
typedef FloatVecSerializerInterface<osg::Vec3f> Vec3fSerializerInterface;
typedef FloatVecSerializerInterface<osg::Vec4f> Vec4fSerializerInterface;
typedef DoubleVecSerializerInterface<osg::Vec2d> Vec2dSerializerInterface;
typedef DoubleVecSerializerInterface<osg::Vec3d> Vec3dSerializerInterface;
typedef DoubleVecSerializerInterface<osg::Vec4d> Vec4dSerializerInterface;
REGISTER_SERIALIZER_INTERFACE(VEC2F, Vec2fSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3F, Vec3fSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4F, Vec4fSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2D, Vec2dSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3D, Vec3dSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4D, Vec4dSerializerInterface)

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
typedef CharVecSerializerInterface<osg::Vec2b> Vec2bSerializerInterface;
typedef CharVecSerializerInterface<osg::Vec3b> Vec3bSerializerInterface;
typedef CharVecSerializerInterface<osg::Vec4b> Vec4bSerializerInterface;
typedef UCharVecSerializerInterface<osg::Vec2ub> Vec2ubSerializerInterface;
typedef UCharVecSerializerInterface<osg::Vec3ub> Vec3ubSerializerInterface;
typedef UCharVecSerializerInterface<osg::Vec4ub> Vec4ubSerializerInterface;
typedef ShortVecSerializerInterface<osg::Vec2s> Vec2sSerializerInterface;
typedef ShortVecSerializerInterface<osg::Vec3s> Vec3sSerializerInterface;
typedef ShortVecSerializerInterface<osg::Vec4s> Vec4sSerializerInterface;
typedef UShortVecSerializerInterface<osg::Vec2us> Vec2usSerializerInterface;
typedef UShortVecSerializerInterface<osg::Vec3us> Vec3usSerializerInterface;
typedef UShortVecSerializerInterface<osg::Vec4us> Vec4usSerializerInterface;
typedef IntVecSerializerInterface<osg::Vec2i> Vec2iSerializerInterface;
typedef IntVecSerializerInterface<osg::Vec3i> Vec3iSerializerInterface;
typedef IntVecSerializerInterface<osg::Vec4i> Vec4iSerializerInterface;
typedef UIntVecSerializerInterface<osg::Vec2ui> Vec2uiSerializerInterface;
typedef UIntVecSerializerInterface<osg::Vec3ui> Vec3uiSerializerInterface;
typedef UIntVecSerializerInterface<osg::Vec4ui> Vec4uiSerializerInterface;

REGISTER_SERIALIZER_INTERFACE(VEC2B, Vec2bSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3B, Vec3bSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4B, Vec4bSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2UB, Vec2ubSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3UB, Vec3ubSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4UB, Vec4ubSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2S, Vec2sSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3S, Vec3sSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4S, Vec4sSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2US, Vec2usSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3US, Vec3usSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4US, Vec4usSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2I, Vec2iSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3I, Vec3iSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4I, Vec4iSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC2UI, Vec2uiSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC3UI, Vec3uiSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(VEC4UI, Vec4uiSerializerInterface)
#endif
