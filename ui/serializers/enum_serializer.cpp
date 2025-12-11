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
            if (_entry->setEnumProperty(_object.get(), _property.name, value)) doneEditing();
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
        osgVerse::SerializerFactory* factory = osgVerse::SerializerFactory::instance();
        _glEnums = factory->getGLEnumMap(_property.name);

        _combo = new ComboBox(TR("TypeSelector") + _postfix); _combo->tooltip = tooltip(_property);
        for (auto it = _glEnums.begin(); it != _glEnums.end(); ++it)
        { _combo->items.push_back(it->first); _glEnumsRev[it->second] = it->first; }

        _combo->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            {
                std::string value = _combo->items[_combo->index]; _result->value = _glEnums[value];
                if (_entry->setProperty(_object.get(), _property.name, _result->value)) doneEditing();
            };

        _result = new InputValueField(TR("TypeValue") + _postfix);
        _result->type = InputValueField::UIntValue; _result->tooltip = tooltip(_property);
        _result->flags = ImGuiInputTextFlags_CharsHexadecimal;
        _result->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            {
                unsigned int value = (unsigned int)_result->value; _combo->set(_glEnumsRev[value], true);
                _entry->setProperty(_object.get(), _property.name, _result->value);
            };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            unsigned int value = 0; _entry->getProperty(_object.get(), _property.name, value);
            _result->value = (double)value; _combo->set(_glEnumsRev[value], true);
        }

        bool done = _combo->show(mgr, content);
        done |= _result->show(mgr, content); return done;
    }

protected:
    std::map<std::string, GLenum> _glEnums;
    std::map<GLenum, std::string> _glEnumsRev;
    osg::ref_ptr<InputValueField> _result;
    osg::ref_ptr<ComboBox> _combo;
};

REGISTER_SERIALIZER_INTERFACE(ENUM, EnumSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(GLENUM, GLEnumSerializerInterface)
