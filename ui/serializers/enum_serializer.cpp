#include "../SerializerInterface.h"
using namespace osgVerse;

class EnumSerializerInterface : public SerializerInterface
{
public:
    EnumSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, false)
    {
        _combo = new ComboBox(TR(_property.name) + _postfix);
        _combo->tooltip = tooltip(_property);
        _combo->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            const std::string& value = _combo->items[_combo->index];
            _entry->setEnumProperty(_object.get(), _property.name, value);
        };

        std::vector<std::string> items = _entry->getEnumPropertyItems(_object.get(), _property.name);
        for (size_t i = 0; i < items.size(); ++i) _combo->items.push_back(items[i]);
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            std::string v = _entry->getEnumProperty(_object.get(), _property.name);
            for (size_t i = 0; i < _combo->items.size(); ++i)
            { if (v == _combo->items[i]) {_combo->index = i; break;} }
        }
        return _combo->show(mgr, content);
    }

protected:
    osg::ref_ptr<ComboBox> _combo;
};

class GLEnumSerializerInterface : public SerializerInterface
{
public:
    GLEnumSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
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

REGISTER_SERIALIZER_INTERFACE(ENUM, EnumSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(GLENUM, GLEnumSerializerInterface)
