#include "../SerializerInterface.h"
using namespace osgVerse;

class ImageSerializerInterface : public SerializerInterface
{
public:
    ImageSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
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
            _valueImage = dynamic_cast<osg::Image*>(newValue);

            // TODO
        }

        bool done = false;
        // TODO what to show?
        // TODO: a global 'setObject' button?
        return done;
    }

protected:
    osg::observer_ptr<osg::Image> _valueImage;
};

REGISTER_SERIALIZER_INTERFACE(IMAGE, ImageSerializerInterface)
