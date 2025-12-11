#include "../SerializerInterface.h"
using namespace osgVerse;

class ListSerializerInterface : public SerializerInterface
{
public:
    ListSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        //_check->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        //{ _entry->setProperty(_object.get(), _property.name, _check->value); };
    }

    virtual ItemType getType() const { return ListType; };

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            std::string clsName = _object->className();
            if (clsName == "FloatArray")
                {}  // return entry->setProperty(object, key, getVector<float>(value));
            else if (clsName == "Vec2Array")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec2f>(value, sep));
            else if (clsName == "Vec3Array")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec3f>(value, sep));
            else if (clsName == "Vec4Array")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec4f>(value, sep));
            else if (clsName == "DoubleArray")
                {}  // return entry->setProperty(object, key, getVector<double>(value));
            else if (clsName == "Vec2dArray")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec2d>(value, sep));
            else if (clsName == "Vec3dArray")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec3d>(value, sep));
            else if (clsName == "Vec4dArray")
                {}  // return entry->setVecProperty(object, key, getVecVector<osg::Vec4d>(value, sep));
            else if (clsName == "DrawElementsUByte")
                {}  // return entry->setProperty(object, key, getVector<unsigned char>(value));
            else if (clsName == "DrawElementsUShort")
                {}  // return entry->setProperty(object, key, getVector<unsigned short>(value));
            else if (clsName == "DrawElementsUInt")
                {}  // return entry->setProperty(object, key, getVector<unsigned int>(value));
            else  // treat as ObjectVector
            {
                std::vector<osg::Object*> objList; _serializerUiMap.clear();
                _entry->getProperty(_object.get(), _property.name, objList);
                
                SerializerFactory* factory = SerializerFactory::instance();
                for (size_t i = 0; i < objList.size(); ++i)
                {
                    osg::Object* obj = objList[i]; if (!obj)  continue;
                    std::vector<osg::ref_ptr<SerializerBaseItem>> serializerUIs;
                    factory->createInterfaces(obj, _entry.get(), serializerUIs);
                    for (size_t j = 0; j < serializerUIs.size(); ++j) serializerUIs[j]->addIndent(2.0f);
                    _serializerUiMap[obj] = std::pair<int, SerializerList>((int)i, serializerUIs);
                }
            }
        }

        bool done = false;
        for (std::map<osg::Object*, std::pair<int, SerializerList>>::iterator
             it = _serializerUiMap.begin(); it != _serializerUiMap.end(); ++it)
        {
            std::vector<osg::ref_ptr<SerializerBaseItem>>& serializerUIs = it->second.second;
            std::string lineName = "ID " + std::to_string(it->second.first);
            ImGui::Text(TR(lineName).c_str()); ImGui::Separator();
            for (size_t i = 0; i < serializerUIs.size(); ++i)
                done |= serializerUIs[i]->show(mgr, content);
            ImGui::Separator();
        }
        return done;
    }

protected:
    typedef std::vector<osg::ref_ptr<SerializerBaseItem>> SerializerList;
    std::map<osg::Object*, std::pair<int, SerializerList>> _serializerUiMap;
};

REGISTER_SERIALIZER_INTERFACE(LIST, ListSerializerInterface)
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
REGISTER_SERIALIZER_INTERFACE(VECTOR, ListSerializerInterface)
#endif
