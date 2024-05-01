#include "../SerializerInterface.h"
using namespace osgVerse;

class StringSerializerInterface : public SerializerInterface
{
public:
    StringSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _input = new InputField(TR(_property.name) + _postfix);
        _input->tooltip = prop.ownerClass + "::set" + prop.name + "()";
        _input->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        { _entry->setProperty(_object.get(), _property.name, _input->value); };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty()) _entry->getProperty(_object.get(), _property.name, _input->value);
        return _input->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputField> _input;
};

REGISTER_SERIALIZER_INTERFACE(RW_STRING, StringSerializerInterface)
