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
            switch (LibraryEntry::guessVectorDataType(clsName, _property.name))
            {
            case osgDB::BaseSerializer::RW_CHAR: break;  // entry->setProperty(object, key, getVector<char>(value));
            case osgDB::BaseSerializer::RW_UCHAR: break;  // entry->setProperty(object, key, getVector<unsigned char>(value));
            case osgDB::BaseSerializer::RW_SHORT: break;  // entry->setProperty(object, key, getVector<short>(value));
            case osgDB::BaseSerializer::RW_USHORT: break;  // entry->setProperty(object, key, getVector<unsigned short>(value));
            case osgDB::BaseSerializer::RW_INT: break;  // entry->setProperty(object, key, getVector<int>(value));
            case osgDB::BaseSerializer::RW_UINT: break;  // entry->setProperty(object, key, getVector<unsigned int>(value));
            case osgDB::BaseSerializer::RW_FLOAT: break;  // entry->setProperty(object, key, getVector<float>(value));
            case osgDB::BaseSerializer::RW_DOUBLE: break;  // entry->setProperty(object, key, getVector<double>(value));
            case osgDB::BaseSerializer::RW_VEC2F: break;  // entry->setProperty(object, key, getVector<osg::Vec2f>(value, sep));
            case osgDB::BaseSerializer::RW_VEC3F: break;  // entry->setProperty(object, key, getVector<osg::Vec3f>(value, sep));
            case osgDB::BaseSerializer::RW_VEC4F: break;  // entry->setProperty(object, key, getVector<osg::Vec4f>(value, sep));
            case osgDB::BaseSerializer::RW_VEC2D: break;  // entry->setProperty(object, key, getVector<osg::Vec2d>(value, sep));
            case osgDB::BaseSerializer::RW_VEC3D: break;  // entry->setProperty(object, key, getVector<osg::Vec3d>(value, sep));
            case osgDB::BaseSerializer::RW_VEC4D: break;  // entry->setProperty(object, key, getVector<osg::Vec4d>(value, sep));
            case osgDB::BaseSerializer::RW_OBJECT:
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
                } break;
            default: break;
            }
        }

        bool done = false;
        for (std::map<osg::Object*, std::pair<int, SerializerList>>::iterator
             it = _serializerUiMap.begin(); it != _serializerUiMap.end(); ++it)
        {
            std::vector<osg::ref_ptr<SerializerBaseItem>>& serializerUIs = it->second.second;
            std::string lineName = TR("ID " + std::to_string(it->second.first));
            ImGui::Text("%s", lineName.c_str()); ImGui::Separator();
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
