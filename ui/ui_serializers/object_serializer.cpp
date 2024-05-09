#include "../SerializerInterface.h"
using namespace osgVerse;

class ObjectSerializerInterface : public SerializerInterface
{
public:
    ObjectSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            osg::Object* newValue = NULL;
            _entry->getProperty(_object.get(), _property.name, newValue);
            _valueObject = newValue;

            SerializerFactory* factory = SerializerFactory::instance();
            if (_valueObject.valid())
                _valueEntry = factory->createInterfaces(_valueObject.get(), _entry.get(), _serializerUIs);
            for (size_t i = 0; i < _serializerUIs.size(); ++i)
                _serializerUIs[i]->addIndent(2.0f);
        }

        bool done = false;
        for (size_t i = 0; i < _serializerUIs.size(); ++i)
            done |= _serializerUIs[i]->show(mgr, content);
        // TODO: a global 'setObject' button?
        return done;
    }

protected:
    osg::observer_ptr<osg::Object> _valueObject;
    osg::ref_ptr<LibraryEntry> _valueEntry;
    std::vector<osg::ref_ptr<SerializerInterface>> _serializerUIs;
};

REGISTER_SERIALIZER_INTERFACE(OBJECT, ObjectSerializerInterface)
