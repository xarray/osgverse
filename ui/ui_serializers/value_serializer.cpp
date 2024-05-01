#include "../SerializerInterface.h"
using namespace osgVerse;

template<typename T>
class ValueSerializerInterface : public SerializerInterface
{
public:
    ValueSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _value = new InputValueField(TR(_property.name) + _postfix);
        _value->tooltip = prop.ownerClass + "::set" + prop.name + "()";
        _value->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            T value = (T)_value->value;
            _entry->setProperty(_object.get(), _property.name, value);
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            T value; _entry->getProperty(_object.get(), _property.name, value);
            _value->value = (double)value;
        }
        return _value->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputValueField> _value;
};

class IntSerializerInterface : public ValueSerializerInterface<int>
{
public:
    IntSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop) { _value->type = InputValueField::IntValue; }
};

class UIntSerializerInterface : public ValueSerializerInterface<unsigned int>
{
public:
    UIntSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop)
    {
        _value->type = InputValueField::UIntValue;
        _value->flags = ImGuiInputTextFlags_CharsHexadecimal;
    }
};

class FloatSerializerInterface : public ValueSerializerInterface<float>
{
public:
    FloatSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop) { _value->type = InputValueField::FloatValue; }
};

class DoubleSerializerInterface : public ValueSerializerInterface<double>
{
public:
    DoubleSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop) { _value->type = InputValueField::DoubleValue; }
};

REGISTER_SERIALIZER_INTERFACE(RW_INT, IntSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(RW_UINT, UIntSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(RW_FLOAT, FloatSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(RW_DOUBLE, DoubleSerializerInterface)
