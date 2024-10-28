#include "../SerializerInterface.h"
using namespace osgVerse;

ObjectSerializerInterface::ObjectSerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                                     const LibraryEntry::Property& prop)
:   SerializerInterface(obj, entry, prop, true) {}

bool ObjectSerializerInterface::showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
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
    return done;
}

REGISTER_SERIALIZER_INTERFACE(OBJECT, ObjectSerializerInterface)
