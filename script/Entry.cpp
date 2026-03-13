#include "Entry.h"
using namespace osgVerse;

osgDB::BaseSerializer::Type LibraryEntry::guessVectorDataType(const std::string& clsName, const std::string& propName)
{
    if (clsName == "FloatArray") return osgDB::BaseSerializer::RW_FLOAT;
    else if (clsName == "Vec2Array") return osgDB::BaseSerializer::RW_VEC2F;
    else if (clsName == "Vec3Array") return osgDB::BaseSerializer::RW_VEC3F;
    else if (clsName == "Vec4Array") return osgDB::BaseSerializer::RW_VEC4F;
    else if (clsName == "DoubleArray") return osgDB::BaseSerializer::RW_DOUBLE;
    else if (clsName == "Vec2dArray") return osgDB::BaseSerializer::RW_VEC2D;
    else if (clsName == "Vec3dArray") return osgDB::BaseSerializer::RW_VEC3D;
    else if (clsName == "Vec4dArray") return osgDB::BaseSerializer::RW_VEC4D;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    else if (clsName == "Vec2bArray") return osgDB::BaseSerializer::RW_VEC2B;
    else if (clsName == "Vec3bArray") return osgDB::BaseSerializer::RW_VEC3B;
    else if (clsName == "Vec4bArray") return osgDB::BaseSerializer::RW_VEC4B;
    else if (clsName == "Vec2ubArray") return osgDB::BaseSerializer::RW_VEC2UB;
    else if (clsName == "Vec3ubArray") return osgDB::BaseSerializer::RW_VEC3UB;
    else if (clsName == "Vec4ubArray") return osgDB::BaseSerializer::RW_VEC4UB;
    else if (clsName == "Vec2sArray") return osgDB::BaseSerializer::RW_VEC2S;
    else if (clsName == "Vec3sArray") return osgDB::BaseSerializer::RW_VEC3S;
    else if (clsName == "Vec4sArray") return osgDB::BaseSerializer::RW_VEC4S;
    else if (clsName == "Vec2usArray") return osgDB::BaseSerializer::RW_VEC2US;
    else if (clsName == "Vec3usArray") return osgDB::BaseSerializer::RW_VEC3US;
    else if (clsName == "Vec4usArray") return osgDB::BaseSerializer::RW_VEC4US;
    else if (clsName == "Vec2iArray") return osgDB::BaseSerializer::RW_VEC2I;
    else if (clsName == "Vec3iArray") return osgDB::BaseSerializer::RW_VEC3I;
    else if (clsName == "Vec4iArray") return osgDB::BaseSerializer::RW_VEC4I;
    else if (clsName == "Vec2uiArray") return osgDB::BaseSerializer::RW_VEC2UI;
    else if (clsName == "Vec3uiArray") return osgDB::BaseSerializer::RW_VEC3UI;
    else if (clsName == "Vec4uiArray") return osgDB::BaseSerializer::RW_VEC4UI;
#endif
    else if (clsName == "DrawElementsUByte") return osgDB::BaseSerializer::RW_UCHAR;
    else if (clsName == "DrawElementsIndirectUByte") return osgDB::BaseSerializer::RW_UCHAR;
    else if (clsName == "DrawElementsUShort") return osgDB::BaseSerializer::RW_USHORT;
    else if (clsName == "DrawElementsIndirectUShort") return osgDB::BaseSerializer::RW_USHORT;
    else if (clsName == "DrawElementsUInt") return osgDB::BaseSerializer::RW_UINT;
    else if (clsName == "DrawElementsIndirectUInt") return osgDB::BaseSerializer::RW_UINT;
    else if (clsName == "DrawArrayLengths") return osgDB::BaseSerializer::RW_INT;
    else if (clsName == "MultiDrawArrays") return osgDB::BaseSerializer::RW_INT;
    else return osgDB::BaseSerializer::RW_OBJECT;
}

LibraryEntry::LibraryEntry(const std::string& libName)
{
    osgDB::Registry* registry = osgDB::Registry::instance();
    std::string nodeKitLib = registry->createLibraryNameForNodeKit(libName);
    std::string pluginLib = registry->createLibraryNameForExtension(
        std::string("serializers_") + libName);
    std::string pluginLib2 = registry->createLibraryNameForExtension(libName);

#ifndef VERSE_STATIC_BUILD
    if (registry->loadLibrary(nodeKitLib) != osgDB::Registry::NOT_LOADED ||
        registry->loadLibrary(pluginLib) != osgDB::Registry::NOT_LOADED ||
        registry->loadLibrary(pluginLib2) != osgDB::Registry::NOT_LOADED)
#endif
    { refresh(libName); }
}

void LibraryEntry::refresh(const std::string& libName)
{
    osgDB::ObjectWrapperManager* owm = osgDB::Registry::instance()->getObjectWrapperManager();
    osgDB::ObjectWrapperManager::WrapperMap& wrappers = owm->getWrapperMap();

    _classes.clear(); _libraryName = libName;
    for (osgDB::ObjectWrapperManager::WrapperMap::iterator itr = wrappers.begin();
         itr != wrappers.end(); ++itr)
    {
        std::size_t sep = itr->first.find("::");
        if (sep != std::string::npos && libName == itr->first.substr(0, sep))
            _classes.insert(itr->first.substr(sep + 2));
    }
}

std::vector<LibraryEntry::Property> LibraryEntry::getPropertyNames(const std::string& clsName) const
{
    std::size_t sep = clsName.find("::"); std::string name = clsName;
    if (sep == std::string::npos) name = _libraryName + "::" + clsName;

    osgDB::Registry* registry = osgDB::Registry::instance();
    osgDB::ObjectWrapperManager* owm = registry->getObjectWrapperManager();
    osgDB::ObjectWrapper* ow = owm->findWrapper(name);

    std::vector<Property> properties;
    if (ow != NULL)
    {
#if OSGVERSE_COMPLETED_SCRIPT
#   if OSG_VERSION_GREATER_THAN(3, 4, 1)
        const osgDB::ObjectWrapper::RevisionAssociateList& associates = ow->getAssociates();
        for (osgDB::ObjectWrapper::RevisionAssociateList::const_iterator aitr = associates.begin();
             aitr != associates.end(); ++aitr)
        {
            osgDB::ObjectWrapper* ow1 = registry->getObjectWrapperManager()->findWrapper(aitr->_name);
            if (ow1 == NULL) continue;
            
            unsigned int i = 0;
            const osgDB::ObjectWrapper::SerializerList& sList = ow1->getSerializerList();
            for (osgDB::ObjectWrapper::SerializerList::const_iterator sitr = sList.begin();
                 sitr != sList.end(); ++sitr, ++i)
            {
                Property prop;
                prop.ownerClass = aitr->_name; prop.name = (*sitr)->getName();
                prop.type = ow1->getTypeList()[i];
                prop.typeName = _manager.getTypeName(prop.type);
                prop.outdated = (OPENSCENEGRAPH_SOVERSION < aitr->_firstVersion) ||
                                (OPENSCENEGRAPH_SOVERSION > aitr->_lastVersion);
                properties.push_back(prop);
            }
        }
#   else
        const osgDB::StringList& associates = ow->getAssociates();
        for (size_t n = 0; n < associates.size(); ++n)
        {
            osgDB::ObjectWrapper* ow1 = registry->getObjectWrapperManager()->findWrapper(associates[n]);
            if (ow1 == NULL) continue;

            unsigned int i = 0;
            const osgDB::ObjectWrapper::SerializerList& sList = ow1->getSerializerList();
            for (osgDB::ObjectWrapper::SerializerList::const_iterator sitr = sList.begin();
                sitr != sList.end(); ++sitr, ++i)
            {
                Property prop;
                prop.ownerClass = associates[n]; prop.name = (*sitr)->getName();
                prop.type = ow1->getTypeList()[i];
                prop.typeName = _manager.getTypeName(prop.type);
                properties.push_back(prop);
            }
        }
#   endif
#endif
    }
    return properties;
}

std::vector<LibraryEntry::Method> LibraryEntry::getMethodNames(const std::string& clsName) const
{
    std::size_t sep = clsName.find("::"); std::string name = clsName;
    if (sep == std::string::npos) name = _libraryName + "::" + clsName;

    osgDB::Registry* registry = osgDB::Registry::instance();
    osgDB::ObjectWrapperManager* owm = registry->getObjectWrapperManager();
    osgDB::ObjectWrapper* ow = owm->findWrapper(name);

    std::vector<Method> methods;
    if (ow != NULL)
    {
#if OSGVERSE_COMPLETED_SCRIPT
#   if OSG_VERSION_GREATER_THAN(3, 4, 1)
        const osgDB::ObjectWrapper::RevisionAssociateList& associates = ow->getAssociates();
        for (osgDB::ObjectWrapper::RevisionAssociateList::const_iterator aitr = associates.begin();
             aitr != associates.end(); ++aitr)
        {
            osgDB::ObjectWrapper* ow1 = owm->findWrapper(aitr->_name);
            if (ow1 == NULL) continue;

            const osgDB::ObjectWrapper::MethodObjectMap& mMap = ow1->getMethodObjectMap();
            for (osgDB::ObjectWrapper::MethodObjectMap::const_iterator mitr = mMap.begin();
                 mitr != mMap.end(); ++mitr)
            {
                Method method;
                method.ownerClass = aitr->_name; method.name = mitr->first;
                method.outdated = (OPENSCENEGRAPH_SOVERSION < aitr->_firstVersion) ||
                                  (OPENSCENEGRAPH_SOVERSION > aitr->_lastVersion);
                methods.push_back(method);
            }
        }
#   else
        const osgDB::StringList& associates = ow->getAssociates();
        for (size_t n = 0; n < associates.size(); ++n)
        {
            osgDB::ObjectWrapper* ow1 = registry->getObjectWrapperManager()->findWrapper(associates[n]);
            if (ow1 == NULL) continue;

            const osgDB::ObjectWrapper::MethodObjectMap& mMap = ow1->getMethodObjectMap();
            for (osgDB::ObjectWrapper::MethodObjectMap::const_iterator mitr = mMap.begin();
                 mitr != mMap.end(); ++mitr)
            {
                Method method;
                method.ownerClass = associates[n]; method.name = mitr->first;
                methods.push_back(method);
            }
        }
#   endif
#endif
    }
    return methods;
}

std::string LibraryEntry::getClassName(osg::Object* obj, bool withLibName)
{
    if (!obj) return ""; else if (!withLibName) return obj->className();
    return obj->libraryName() + std::string("::") + obj->className();
}

#if OSGVERSE_COMPLETED_SCRIPT
std::vector<std::string> LibraryEntry::getEnumPropertyItems(const osg::Object* object, const std::string& name)
{
    osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
    osgDB::BaseSerializer* bs = _manager.getSerializer(object, name, type);
    std::vector<std::string> items;
    if (bs && bs->getIntLookup())
    {
        const osgDB::IntLookup::StringToValue& values = bs->getIntLookup()->getStringToValue();
        for (osgDB::IntLookup::StringToValue::const_iterator itr = values.begin();
            itr != values.end(); ++itr) items.push_back(itr->first);
    }
    return items;
}

std::string LibraryEntry::getEnumProperty(const osg::Object* object, const std::string& name)
{
    int value = 0;
    if (_manager.getProperty<int>(object, name, value))
    {
        osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
        osgDB::BaseSerializer* bs = _manager.getSerializer(object, name, type);
        if (bs && bs->getIntLookup())
            return bs->getIntLookup()->getString(value);
    }
    return std::to_string(value);
}

bool LibraryEntry::setEnumProperty(osg::Object* object, const std::string& name,
                                   const std::string& value)
{
    osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
    osgDB::BaseSerializer* bs = _manager.getSerializer(object, name, type);
    if (bs && bs->getIntLookup())
        return _manager.setProperty(object, name, bs->getIntLookup()->getValue(value.c_str()));
    return false;
}

bool LibraryEntry::callMethod(osg::Object* object, const std::string& name, osg::Object* arg1)
{
    osg::Parameters args0, args1; args0.push_back(arg1);
    return _manager.run(object, name, args0, args1);
}

osg::Object* LibraryEntry::callMethod(osg::Object* object, const std::string& name)
{
    osg::Parameters args0, args1;
    if (_manager.run(object, name, args0, args1))
    { if (!args1.empty()) return args1.front().release(); }
    return NULL;
}

bool LibraryEntry::callMethod(osg::Object* object, const std::string& name,
                              osg::Parameters& args0, osg::Parameters& args1)
{ return _manager.run(object, name, args0, args1); }

osg::Object* LibraryEntry::create(const std::string& clsName)
{
    std::size_t sep = clsName.find("::"); std::string name = clsName;
    if (sep == std::string::npos) name = _libraryName + "::" + clsName;
    return _manager.createObject(name);
}
#else
std::vector<std::string> LibraryEntry::getEnumPropertyItems(const osg::Object* object, const std::string& name)
{
    OSG_WARN << "[LibraryEntry] getEnumPropertyItems() not implemented" << std::endl;
    return std::vector<std::string>();
}

std::string LibraryEntry::getEnumProperty(const osg::Object* object, const std::string& name)
{ OSG_WARN << "[LibraryEntry] getEnumProperty() not implemented" << std::endl; return ""; }

bool LibraryEntry::setEnumProperty(osg::Object* object, const std::string& name,
                                   const std::string& value)
{ OSG_WARN << "[LibraryEntry] setEnumProperty() not implemented" << std::endl; return false; }

bool LibraryEntry::callMethod(osg::Object* object, const std::string& name, osg::Object* arg1)
{ OSG_WARN << "[LibraryEntry] callMethod() not implemented" << std::endl; return false; }

osg::Object* LibraryEntry::callMethod(osg::Object* object, const std::string& name)
{ OSG_WARN << "[LibraryEntry] callMethod() not implemented" << std::endl; return NULL; }

bool LibraryEntry::callMethod(osg::Object* object, const std::string& name,
                              osg::Parameters& args0, osg::Parameters& args1)
{ OSG_WARN << "[LibraryEntry] callMethod() not implemented" << std::endl; return false; }

osg::Object* LibraryEntry::create(const std::string& name)
{ OSG_WARN << "[LibraryEntry] create() not implemented" << std::endl; return NULL; }
#endif

MethodInformationManager::MethodInformationManager()
{
    // TODO: add default method information
}

MethodInformationManager* MethodInformationManager::instance()
{
    static osg::ref_ptr<MethodInformationManager> s_instance = new MethodInformationManager;
    return s_instance.get();
}

void MethodInformationManager::addInformation(osgDB::ObjectWrapper* cls, const std::string& m, const MethodInformation& info)
{ ClassAndMethod pair(cls, m); _informationMap[pair] = info; }

void MethodInformationManager::removeInformation(osgDB::ObjectWrapper* cls, const std::string& methodName)
{
    ClassAndMethod pair(cls, methodName);
    std::map<ClassAndMethod, MethodInformation>::iterator it = _informationMap.find(pair);
    if (it != _informationMap.end()) _informationMap.erase(it);
}

std::vector<MethodInformationManager::Argument>& MethodInformationManager::getInformation(
        osgDB::ObjectWrapper* cls, const std::string& methodName)
{
    ClassAndMethod pair(cls, methodName);
    std::map<ClassAndMethod, MethodInformation>::iterator it = _informationMap.find(pair);
    if (it != _informationMap.end()) return it->second.second; else return _defaultArguments;
}

const std::vector<MethodInformationManager::Argument>& MethodInformationManager::getInformation(
        osgDB::ObjectWrapper* cls, const std::string& methodName) const
{
    ClassAndMethod pair(cls, methodName);
    std::map<ClassAndMethod, MethodInformation>::const_iterator it = _informationMap.find(pair);
    if (it != _informationMap.end()) return it->second.second; else return _defaultArguments;
}
