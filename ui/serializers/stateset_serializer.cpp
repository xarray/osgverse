#include <osg/StateSet>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include "../SerializerInterface.h"
using namespace osgVerse;

class StateSetSerializerInterface : public SerializerInterface
{
public:
    StateSetSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
    }

    virtual ItemType getType() const { return ObjectType; };

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            osg::Object* newValue = NULL;
            _entry->getProperty(_object.get(), _property.name, newValue);
            _valueObject = newValue; _serializerUIs.clear();

            SerializerFactory* factory = SerializerFactory::instance();
            if (_valueObject.valid())
                _valueEntry = factory->createInterfaces(_valueObject.get(), _entry.get(), _serializerUIs);
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
        if (ImGui::MenuItem(TR(_valueObject.valid() ? "Delete" : "Create").c_str()))
        {
            if (!_valueObject)
                _valueObject = _entry->callMethod(_object.get(), "getOrCreateStateSet");
            else
                _entry->setProperty(_object.get(), _property.name, (osg::Object*)NULL);
            dirty();
        }
        SerializerInterface::showMenuItems(mgr, content);
    }

    osg::observer_ptr<osg::Object> _valueObject;
    osg::ref_ptr<LibraryEntry> _valueEntry;
    std::vector<osg::ref_ptr<SerializerBaseItem>> _serializerUIs;
};

REGISTER_SERIALIZER_INTERFACE2(StateSet, NULL, StateSetSerializerInterface)
