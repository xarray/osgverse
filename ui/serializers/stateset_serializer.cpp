#include <osg/StateSet>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include "../SerializerInterface.h"
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
    :   ObjectSerializerInterface(obj, entry, prop)
    { for (int i = 0; i < 3; ++i) _separator[i] = 0; }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            osg::Object* newValue = NULL;
            _entry->getProperty(_object.get(), _property.name, newValue);
            _valueObject = newValue; _serializerUIs.clear();

            SerializerFactory* factory = SerializerFactory::instance();
            if (_valueObject.valid())
            {
                LibraryEntry* entry = _entry.valid() ? _entry.get() : new LibraryEntry("osg");
                osg::StateSet* ss = static_cast<osg::StateSet*>(_valueObject.get());
                _valueEntry = factory->createInterfaces(_valueObject.get(), _entry.get(), _serializerUIs);
                _separator[0] = _serializerUIs.size();

                // Attributes
                osg::StateSet::AttributeList& attrList = ss->getAttributeList();
                for (osg::StateSet::AttributeList::iterator itr = attrList.begin();
                     itr != attrList.end(); ++itr)
                {
                    _serializerUIs.push_back(new AttributeSerializerInterface(
                        itr->second.first.get(), entry, ss, itr->second.second));
                }
                _separator[1] = _serializerUIs.size() - _separator[0];

                // Texture attributes
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
                _separator[2] = _serializerUIs.size() - _separator[0] - _separator[1];

                // Uniforms
                osg::StateSet::UniformList& uniforms = ss->getUniformList();
                for (osg::StateSet::UniformList::iterator itr = uniforms.begin();
                     itr != uniforms.end(); ++itr)
                {
                    // TODO
                }
            }
            for (size_t i = 0; i < _serializerUIs.size(); ++i)
                _serializerUIs[i]->addIndent(2.0f);
        }

        bool done = false;
        static std::string separatedNames[3] = {"Attributes", "Textures", "Uniforms"};
        for (size_t i = 0; i < _serializerUIs.size(); ++i)
        {
            for (int s = 0; s < 3; ++s)
            {
                std::string num = " " + std::to_string(_separator[s]);
                if (_separator[s] > 0 && _separator[s] == i)
                { ImGui::Separator(); ImGui::Text(TR(separatedNames[s] + num).c_str()); }
            }
            done |= _serializerUIs[i]->show(mgr, content);
        }
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

    size_t _separator[3];
};

REGISTER_SERIALIZER_INTERFACE2(StateSet, NULL, StateSetSerializerInterface)
