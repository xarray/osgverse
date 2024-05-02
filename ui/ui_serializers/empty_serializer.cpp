#include "../SerializerInterface.h"
using namespace osgVerse;

class EmptySerializerInterface : public SerializerInterface
{
public:
    EmptySerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
    : SerializerInterface(obj, entry, prop, false) { _hidden = true; _dirty = false; }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    { return false; }

    virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
    { return -1; }  // no spider node to create
};

REGISTER_SERIALIZER_INTERFACE(RW_UNDEFINED, EmptySerializerInterface)
REGISTER_SERIALIZER_INTERFACE(RW_USER, EmptySerializerInterface)
