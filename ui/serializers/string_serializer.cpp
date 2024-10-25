#include "../SerializerInterface.h"
using namespace osgVerse;

class StringSerializerInterface : public SerializerInterface
{
public:
    StringSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _input = new InputField(TR(_property.name) + _postfix);
        _input->tooltip = tooltip(_property);
        _input->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        { if (_entry->setProperty(_object.get(), _property.name, _input->value)) doneEditing(); };
    }

    virtual ItemType getType() const { return StringType; };

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty()) _entry->getProperty(_object.get(), _property.name, _input->value);
        return _input->show(mgr, content);
    }

protected:
    osg::ref_ptr<InputField> _input;
};

REGISTER_SERIALIZER_INTERFACE(STRING, StringSerializerInterface)
