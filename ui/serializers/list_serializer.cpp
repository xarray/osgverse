#include "../SerializerInterface.h"
using namespace osgVerse;

class ListSerializerInterface : public SerializerInterface
{
public:
    ListSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        _check = new CheckBox(TR(_property.name) + _postfix, false);
        _check->tooltip = tooltip(_property);
        //_check->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        //{ _entry->setProperty(_object.get(), _property.name, _check->value); };
    }

    virtual ItemType getType() const { return ListType; };

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        //if (isDirty()) _entry->getProperty(_object.get(), _property.name, _check->value);
        return _check->show(mgr, content);
    }

protected:
    osg::ref_ptr<CheckBox> _check;
};

REGISTER_SERIALIZER_INTERFACE(LIST, ListSerializerInterface)
#if OSG_VERSION_GREATER_THAN(3, 4, 0)
REGISTER_SERIALIZER_INTERFACE(VECTOR, ListSerializerInterface)
#endif
