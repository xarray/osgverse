#include "../SerializerInterface.h"
using namespace osgVerse;

class ImageSerializerInterface : public ObjectSerializerInterface
{
public:
    ImageSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : ObjectSerializerInterface(obj, entry, prop)
    {
        std::string imgName = TR(prop.name) + _postfix + "_IMG";
        _imagePreview = new ImageButton(imgName);
        _imagePreview->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
            {
                // TODO: start a dialog to show, edit or replace this image
            };
    }

    virtual ItemType getType() const { return ObjectType; };

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            osg::Object* newValue = NULL;
            _entry->getProperty(_object.get(), _property.name, newValue);
            _valueImage = dynamic_cast<osg::Image*>(newValue);
            _valueObject = newValue; _lastManager = mgr;

            int w = _valueImage.valid() ? _valueImage->s() : 0;
            int h = _valueImage.valid() ? _valueImage->t() : 0;
            std::string imgName = TR(_property.name) + _postfix + "_IMG";
            mgr->setGuiTexture(imgName, new osg::Texture2D(_valueImage.get()));
            if (w > 0 && h > 0) _imagePreview->size.set((w < 128) ? w : (w * 128 / h), (h < 128) ? h : 128);

            SerializerFactory* factory = SerializerFactory::instance();
            if (_valueObject.valid())
                _valueEntry = factory->createInterfaces(_valueObject.get(), _entry.get(), _serializerUIs);
            for (size_t i = 0; i < _serializerUIs.size(); ++i)
                _serializerUIs[i]->addIndent(2.0f);
        }

        bool done = _imagePreview->show(mgr, content);
        for (size_t i = 0; i < _serializerUIs.size(); ++i)
            done |= _serializerUIs[i]->show(mgr, content);
        return done;
    }

protected:
    virtual ~ImageSerializerInterface()
    {
        if (_lastManager.valid())
            _lastManager->removeGuiTexture(TR(_property.name) + _postfix);
    }

    osg::observer_ptr<osg::Image> _valueImage;
    osg::observer_ptr<ImGuiManager> _lastManager;
    osg::ref_ptr<ImageButton> _imagePreview;
};

REGISTER_SERIALIZER_INTERFACE(IMAGE, ImageSerializerInterface)
