#include <osg/StateSet>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include "serializer_utils.h"
using namespace osgVerse;

class AttributeSerializerInterface : public SerializerBaseItem
{
public:
    AttributeSerializerInterface(osg::StateAttribute* attr, LibraryEntry* entry,
                                 osg::StateSet* ss, int v, int u = -1)
    :   SerializerBaseItem(attr, true), _entry(entry), _parent(ss), _value(v), _unit(u) {}

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        std::string title = TR(_object->className()) + _postfix;
        if (_unit >= 0) title = TR("Unit" + std::to_string(_unit) + ": ") + title;
        return showInternal(mgr, content, title);
    }

    virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
    {
        // TODO
        return -1;
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            SerializerFactory* factory = SerializerFactory::instance();
            if (_object.valid())
                factory->createInterfaces(_object.get(), _entry.get(), _serializerUIs);

            // Most state attributes never need a user-editable name, so hide the "Name"
            // field to keep the property panel clean (Program is kept as an exception).
            if (_object.valid() && _object->className() != std::string("Program"))
            {
                for (size_t i = 0; i < _serializerUIs.size(); )
                {
                    SerializerInterface* si = dynamic_cast<SerializerInterface*>(_serializerUIs[i].get());
                    if (si != NULL && si->getPropertyName() == "Name")
                        _serializerUIs.erase(_serializerUIs.begin() + i);
                    else ++i;
                }
            }
            for (size_t i = 0; i < _serializerUIs.size(); ++i)
                _serializerUIs[i]->addIndent(2.0f);
        }

        bool done = false;
        for (size_t i = 0; i < _serializerUIs.size(); ++i)
            done |= _serializerUIs[i]->show(mgr, content);
        return done;
    }

protected:
    virtual void showMenuItems(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (ImGui::MenuItem(TR("Delete").c_str()))
        {
            // TODO
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::ON) ?
                               "Turn Off" : "Turn On").c_str()))
        {
            // TODO
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::OVERRIDE) ?
                               "Override Off" : "Override On").c_str()))
        {
            // TODO
        }

        if (ImGui::MenuItem(TR((_value & osg::StateAttribute::PROTECTED) ?
                               "Unprotect" : "Protect").c_str()))
        {
            // TODO
        }
        SerializerBaseItem::showMenuItems(mgr, content);
    }

    osg::ref_ptr<LibraryEntry> _entry;
    osg::observer_ptr<osg::StateSet> _parent;
    std::vector<osg::ref_ptr<SerializerBaseItem>> _serializerUIs;
    int _value, _unit;
};

class StateSetSerializerInterface : public ObjectSerializerInterface
{
public:
    StateSetSerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                const LibraryEntry::Property& prop)
    :   ObjectSerializerInterface(obj, entry, prop), _needsRebuild(false)
    { for (int i = 0; i < 3; ++i) { _sectionStart[i] = 0; _sectionCount[i] = 0; } }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty() || _needsRebuild)
        {
            _needsRebuild = false;
            osg::Object* newValue = NULL;
            _entry->getProperty(_object.get(), _property.name, newValue);
            _valueObject = newValue; _serializerUIs.clear();

            SerializerFactory* factory = SerializerFactory::instance();
            for (int i = 0; i < 3; ++i) { _sectionStart[i] = 0; _sectionCount[i] = 0; }
            if (_valueObject.valid())
            {
                LibraryEntry* entry = _entry.valid() ? _entry.get() : new LibraryEntry("osg");
                osg::StateSet* ss = static_cast<osg::StateSet*>(_valueObject.get());

                // Properties of the StateSet itself, shown before all attributes
                _valueEntry = factory->createInterfaces(_valueObject.get(), _entry.get(), _serializerUIs);
                size_t numProperties = _serializerUIs.size();

                // Attributes
                osg::StateSet::AttributeList& attrList = ss->getAttributeList();
                for (osg::StateSet::AttributeList::iterator itr = attrList.begin();
                     itr != attrList.end(); ++itr)
                {
                    _serializerUIs.push_back(new AttributeSerializerInterface(
                        itr->second.first.get(), entry, ss, itr->second.second));
                }
                _sectionStart[0] = numProperties;
                _sectionCount[0] = _serializerUIs.size() - numProperties;

                // Texture attributes
                size_t numAfterAttributes = _serializerUIs.size();
                osg::StateSet::TextureAttributeList& texAttrList = ss->getTextureAttributeList();
                for (size_t i = 0; i < texAttrList.size(); ++i)
                {
                    osg::StateSet::AttributeList& attr = texAttrList[i];
                    for (osg::StateSet::AttributeList::iterator itr = attr.begin();
                         itr != attr.end(); ++itr)
                    {
                        _serializerUIs.push_back(new AttributeSerializerInterface(
                            itr->second.first.get(), entry, ss, itr->second.second, i));
                    }
                }
                _sectionStart[1] = numAfterAttributes;
                _sectionCount[1] = _serializerUIs.size() - numAfterAttributes;

                // Uniforms
                size_t numAfterTextureAttributes = _serializerUIs.size();
                osg::StateSet::UniformList& uniforms = ss->getUniformList();
                for (osg::StateSet::UniformList::iterator itr = uniforms.begin();
                     itr != uniforms.end(); ++itr)
                {
                    osg::Uniform* uniform = itr->second.first.get();
                    if (uniform == NULL) continue;
                    _serializerUIs.push_back(createUniformSerializerItem(uniform, ss, itr->second.second));
                }
                _sectionStart[2] = numAfterTextureAttributes;
                _sectionCount[2] = _serializerUIs.size() - numAfterTextureAttributes;
            }
            for (size_t i = 0; i < _serializerUIs.size(); ++i)
                _serializerUIs[i]->addIndent(2.0f);
        }

        bool done = false, rebuild = false;
        static const char* separatedNames[3] = {"Attributes", "Texture Attributes", "Uniforms"};
        for (size_t i = 0; i <= _serializerUIs.size(); ++i)
        {
            // Print a section header right before the first item of each sub-list
            for (int s = 0; s < 3; ++s)
            {
                if (_valueObject.valid() && _sectionStart[s] == i)
                {
                    std::string name = TR(separatedNames[s]) + " " + std::to_string(_sectionCount[s]);
                    ImGui::Separator(); ImGui::Text("%s", name.c_str());
                }
            }
            if (i >= _serializerUIs.size()) break;

            done |= _serializerUIs[i]->show(mgr, content);
            if (_serializerUIs[i]->checkStructureChanged()) rebuild = true;
        }
        if (rebuild) _needsRebuild = true;  // done in the next frame
        return done;
    }

protected:
    virtual void showMenuItems(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (ImGui::MenuItem(TR(_valueObject.valid() ? "Delete" : "Create").c_str()))
        {
            if (!_valueObject)
                _valueObject = _entry->callMethod(_object.get(), "getOrCreateStateSet");
            else
                _entry->setProperty(_object.get(), _property.name, (osg::Object*)NULL);
            dirty();
        }

        if (ImGui::MenuItem(TR("New Attribute").c_str()))
        {
            // TODO
        }

        if (ImGui::MenuItem(TR("New Texture").c_str()))
        {
            // TODO
        }

        if (ImGui::MenuItem(TR("New Uniform").c_str()))
        {
            // TODO
        }
        SerializerInterface::showMenuItems(mgr, content);
    }

    size_t _sectionStart[3], _sectionCount[3];
    bool _needsRebuild;
};

class RenderingHintSerializerInterface : public SerializerInterface
{
public:
    RenderingHintSerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                     const LibraryEntry::Property& prop)
    :   SerializerInterface(obj, entry, prop, false)
    {
        // osg::StateSet::RenderingHint has only three valid choices
        static const int hintValues[3] = { osg::StateSet::DEFAULT_BIN,
                                           osg::StateSet::TRANSPARENT_BIN,
                                           osg::StateSet::OPAQUE_BIN };
        _combo = new ComboBox(TR(_property.name) + _postfix);
        _combo->tooltip = tooltip(_property);
        _combo->items.push_back("DEFAULT_BIN");
        _combo->items.push_back("TRANSPARENT_BIN");
        _combo->items.push_back("OPAQUE_BIN");
        _combo->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            if (_entry->setProperty(_object.get(), _property.name, hintValues[_combo->index]))
                doneEditing();
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            int value = (int)osg::StateSet::DEFAULT_BIN;
            _entry->getProperty(_object.get(), _property.name, value);
            _combo->index = 0;
            if (value == (int)osg::StateSet::TRANSPARENT_BIN) _combo->index = 1;
            else if (value == (int)osg::StateSet::OPAQUE_BIN) _combo->index = 2;
        }
        return _combo->show(mgr, content);
    }

    virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
    { return -1; }  // TODO

protected:
    osg::ref_ptr<ComboBox> _combo;
};

REGISTER_SERIALIZER_INTERFACE2(StateSet, NULL, StateSetSerializerInterface)
REGISTER_SERIALIZER_INTERFACE2(RenderingHint, NULL, RenderingHintSerializerInterface)
