#include "../SerializerInterface.h"
using namespace osgVerse;

template<typename T>
class ValueSerializerInterface : public SerializerInterface
{
public:
    ValueSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false), _showHexButton(false)
    {
        _value = new InputValueField(TR(_property.name) + _postfix);
        _value->tooltip = tooltip(_property);
        _value->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            T value = (T)_value->value;
            if (_entry->setProperty(_object.get(), _property.name, value)) doneEditing();
        };

        _toHex = new CheckBox(_postfix, false);
        _toHex->tooltip = TR("Hexadecimal");
        _toHex->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            _value->flags = (_toHex->value) ? ImGuiInputTextFlags_CharsHexadecimal
                                            : ImGuiInputTextFlags_CharsDecimal;
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            T value; _entry->getProperty(_object.get(), _property.name, value);
            _value->value = (double)value;
        }
        bool edited = _showHexButton ? _toHex->show(mgr, content) : false;
        if (_showHexButton) ImGui::SameLine();
        return edited | _value->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputValueField> _value;
    osg::ref_ptr<CheckBox> _toHex;
    bool _showHexButton;
};

class CharSerializerInterface : public ValueSerializerInterface<char>
{
public:
    CharSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop)
    {
        _value->type = InputValueField::IntValue;
        _value->minValue = -128; _value->maxValue = 127;
    }
};

class UCharSerializerInterface : public ValueSerializerInterface<unsigned char>
{
public:
    UCharSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop)
    {
        _value->type = InputValueField::UIntValue; _showHexButton = true;
        _value->minValue = 0; _value->maxValue = 255;
    }
};

class ShortSerializerInterface : public ValueSerializerInterface<short>
{
public:
    ShortSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop)
    {
        _value->type = InputValueField::IntValue;
        _value->minValue = -32768; _value->maxValue = 32767;
    }
};

class UShortSerializerInterface : public ValueSerializerInterface<unsigned short>
{
public:
    UShortSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : ValueSerializerInterface(obj, entry, prop)
    {
        _value->type = InputValueField::UIntValue; _showHexButton = true;
        _value->minValue = 0; _value->maxValue = 65535;
    }
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
    { _value->type = InputValueField::UIntValue; _showHexButton = true; }
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

REGISTER_SERIALIZER_INTERFACE(CHAR, CharSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(UCHAR, UCharSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(SHORT, ShortSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(USHORT, UShortSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(INT, IntSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(UINT, UIntSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(FLOAT, FloatSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(DOUBLE, DoubleSerializerInterface)
