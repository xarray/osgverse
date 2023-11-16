#include <nanoid/nanoid.h>
#include <osg/ProxyNode>
#include <osgDB/Serializer>
#include "ScriptBase.h"
using namespace osgVerse;

LibraryEntry* ScriptBase::getOrCreateEntry(const std::string& libName)
{
    if (_entries.find(libName) == _entries.end())
        _entries[libName] = new LibraryEntry(libName);
    return _entries[libName].get();
}

ScriptBase::Result ScriptBase::createFromObject(osg::Object* obj)
{
    if (obj != NULL)
    {
        std::string id = "vobj_" + nanoid::generate(8);
        obj->setName(id); _objects[id] = obj;
        return Result(id, obj);
    }
    else
        return Result(-6, "Invalid creation");
}

ScriptBase::Result ScriptBase::create(const std::string& compName,
                                      const PropertyMap& properties)
{
    std::string libName, clsName; std::size_t sep = compName.find("::");
    if (sep == std::string::npos) { libName = "osg"; clsName = compName; }
    else { libName = compName.substr(0, sep); clsName = compName.substr(sep + 2); }

    if (_entries.find(libName) == _entries.end())
        _entries[libName] = new LibraryEntry(libName);
    osg::Object* obj = _entries[libName]->create(clsName);

    if (obj != NULL)
    {
        std::string id = "vobj_" + nanoid::generate(8);
        obj->setName(id); _objects[id] = obj;
        Result result(id, obj), result2 = set(id, properties);
        result.msg = result2.msg; return result;
    }
    else
        return Result(-6, "Invalid creation type: " + compName);
}

ScriptBase::Result ScriptBase::create(const std::string& type, const std::string& uri,
                                      const PropertyMap& properties)
{
    osg::ref_ptr<osgDB::Options> opt;
    for (PropertyMap::const_iterator itr = properties.begin();
         itr != properties.end(); ++itr)
    {
        if (itr->first == "Options")
        {
            if (!opt) opt = new osgDB::Options(itr->second);
            else opt->setOptionString(itr->second);
        }
    }

    osg::Object* obj = NULL; std::string t;
    std::transform(type.begin(), type.end(), t.begin(), tolower);
    if (t == "image")
    {
        OSG_WARN << "[ScriptBase] Loading image from proxy is not implemented" << std::endl;
        //obj = osgDB::readImageFile(uri);
    }
    else
    {
        osg::ProxyNode* proxy = new osg::ProxyNode; obj = proxy;
        proxy->setDatabaseOptions(opt.get());
        proxy->setFileName(0, uri);  //obj = osgDB::readNodeFile(uri);
    }

    if (obj != NULL)
    {
        std::string id = "vobj_" + nanoid::generate(8);
        obj->setName(id); _objects[id] = obj;
        Result result(id, obj), result2 = set(id, properties);
        result.msg = result2.msg; return result;
    }
    else
        return Result(-7, "Invalid data URI: " + uri + " (" + type + ")");
}

ScriptBase::Result ScriptBase::set(const std::string& nodePath, const PropertyMap& properties)
{
    osg::Object* obj = getFromPath(nodePath);
    if (obj != NULL)
    {
        std::string libName = obj->libraryName(), clsName = obj->className();
        if (_entries.find(libName) == _entries.end())
            _entries[libName] = new LibraryEntry(libName);

        Result result; result.obj = obj;
        LibraryEntry* entry = _entries[libName].get();
        std::vector<LibraryEntry::Property> names = entry->getPropertyNames(clsName);

        for (PropertyMap::const_iterator itr = properties.begin();
            itr != properties.end(); ++itr)
        {
            if (!setProperty(itr->first, itr->second, entry, obj, names))
            {
                if (!result.msg.empty()) result.msg += "\n"; else result.code = -2;
                result.msg += "Can't set property: " + itr->first;
            }
        }
        return result;
    }
    else
        return Result(-1, "Invalid scene object: " + nodePath);
}

ScriptBase::Result ScriptBase::call(const std::string& nodePath, const std::string& key,
                                    const ParameterList& params)
{
    osg::Object* obj = getFromPath(nodePath);
    if (obj != NULL)
    {
        std::string libName = obj->libraryName(), clsName = obj->className();
        if (_entries.find(libName) == _entries.end())
            _entries[libName] = new LibraryEntry(libName);

        Result result; LibraryEntry* entry = _entries[libName].get();
        std::vector<LibraryEntry::Method> methods = entry->getMethodNames(clsName);

        osg::Parameters inArgs, outArgs;
        for (size_t i = 0; i < params.size(); ++i)
            inArgs.push_back(getFromPath(params[i]));
        if (!entry->callMethod(obj, key, inArgs, outArgs))
        { result.code = -8; result.msg += "Can't call method: " + key; }

        if (!outArgs.empty()) result.obj = outArgs[0].get();
        return result;
    }
    else
        return Result(-1, "Invalid scene object: " + nodePath);
}

ScriptBase::Result ScriptBase::get(const std::string& nodePath, const std::string& key)
{
    osg::Object* obj = getFromPath(nodePath);
    if (obj != NULL)
    {
        std::string libName = obj->libraryName(), clsName = obj->className();
        if (_entries.find(libName) == _entries.end())
            _entries[libName] = new LibraryEntry(libName);

        Result result; result.obj = obj;
        LibraryEntry* entry = _entries[libName].get();
        std::vector<LibraryEntry::Property> names = entry->getPropertyNames(clsName);

        if (!getProperty(key, result.value, entry, obj, names))
            return Result(-3, "Can't get property: " + key);
        return result;
    }
    else
        return Result(-1, "Invalid scene object: " + nodePath);
}

ScriptBase::Result ScriptBase::remove(const std::string& nodePath)
{
    osg::Object* obj = getFromPath(nodePath);
    if (obj != NULL)
    {
        std::string id;
        for (std::map<std::string, osg::ref_ptr<osg::Object>>::iterator itr = _objects.begin();
             itr != _objects.end(); ++itr)
        { if (obj == itr->second) { id = itr->first; break; } }

        Result result; result.obj = obj;
        if (id.empty())
        { result.code = -4; result.msg = "Can't delete object not created here: " + nodePath; }
        else if (obj->referenceCount() > 1)
        { result.code = -5; result.msg = "Can't delete object still referenced: " + nodePath; }
        else _objects.erase(_objects.find(id));
        return result;
    }
    else
        return Result(-1, "Invalid scene object: " + nodePath);
}

osg::Object* ScriptBase::getFromPath(const std::string& nodePath)
{
    osg::Object* obj = _rootNode.get();
    if (nodePath.empty() || nodePath == "root")
        return obj;
    else if (_objects.find(nodePath) != _objects.end())
        return _objects[nodePath].get();

    osgDB::StringList path;
    osgDB::split(nodePath, path, '/');
    if (_objects.find(path[0]) != _objects.end())
        obj = _objects[path[0]].get();

#if OSG_VERSION_GREATER_THAN(3, 3, 0)
    osg::Node* node = obj ? obj->asNode() : NULL;
#else
    osg::Node* node = dynamic_cast<osg::Node*>(obj);
#endif
    if (node != NULL)
    {
        for (size_t i = 1; i < path.size(); ++i)
        {
            const std::string& name = path[i];
            osg::Group* parent = node ? node->asGroup() : NULL;
            if (parent == NULL) return NULL; node = NULL;

            if (name == "0")
            {
                if (parent->getNumChildren() > 0)
                    node = parent->getChild(0);
            }
            else
            {
                int index = atoi(name.c_str());
                if (index > 0 && index < (int)parent->getNumChildren())
                    node = parent->getChild(index);
                else
                {
                    for (size_t j = 0; j < parent->getNumChildren(); ++j)
                    {
                        if (parent->getChild(j)->getName() == name)
                        { node = parent->getChild(j); break; }
                    }
                }
            }
        }
        obj = node;
    }
    return obj;
}

template<typename T> static T getVecValue(const std::string& v)
{
    osgDB::StringList values; osgDB::split(v, values, ' ');
    T result; int num = osg::minimum((int)values.size(), (int)T::num_components);
    for (int i = 0; i < num; ++i)
        result[i] = (typename T::value_type)atof(values[i].c_str());
    return result;
}

template<typename T> static T getQuatValue(const std::string& v)
{
    osgDB::StringList values; osgDB::split(v, values, ' ');
    T result; int num = osg::minimum((int)values.size(), 4);
    for (int i = 0; i < num; ++i)
        result[i] = (typename T::value_type)atof(values[i].c_str());
    return result;
}

template<typename T> static T getMatrixValue(const std::string& v, int num = 16)
{
    osgDB::StringList values; osgDB::split(v, values, ' ');
    T result; typename T::value_type* ptr = (typename T::value_type*)result.ptr();
    int minNum = osg::minimum((int)values.size(), num);
    for (int i = 0; i < minNum; ++i)
        *(ptr + i) = (typename T::value_type)atof(values[i].c_str());
    return result;
}

template<typename T> static std::vector<T> getVector(const std::string& v)
{
    osgDB::StringList values; osgDB::split(v, values, ' ');
    std::vector<T> result;
    for (size_t i = 0; i < values.size(); ++i)
        result.push_back((T)atof(values[i].c_str()));
    return result;
}

template<typename T> static std::vector<T> getVecVector(const std::string& v, char s)
{
    osgDB::StringList values; osgDB::split(v, values, s);
    std::vector<T> result;
    for (size_t i = 0; i < values.size(); ++i)
        result.push_back(getVecValue<T>(values[i]));
    return result;
}

bool ScriptBase::setProperty(const std::string& key, const std::string& value,
                             LibraryEntry* entry, osg::Object* object,
                             const std::vector<LibraryEntry::Property>& names)
{
    std::string clsName = object->className();
    std::string value2; char sep = _vecSeparator;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const LibraryEntry::Property& prop = names[i];
        if (prop.name != key || prop.outdated) continue;

#if OSGVERSE_COMPLETED_SCRIPT
        switch (prop.type)
        {
        case osgDB::BaseSerializer::RW_OBJECT:
        case osgDB::BaseSerializer::RW_IMAGE:
            return entry->setProperty(object, key, getFromPath(value));
        case osgDB::BaseSerializer::RW_BOOL:
            std::transform(value.begin(), value.end(), value2.begin(), tolower);
            if (value2 == "true") return entry->setProperty(object, key, true);
            else return entry->setProperty(object, key, atoi(value2.c_str()) > 0);
        case osgDB::BaseSerializer::RW_CHAR:
            return entry->setProperty(object, key, (char)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_UCHAR:
            return entry->setProperty(object, key, (unsigned char)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_SHORT:
            return entry->setProperty(object, key, (short)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_USHORT:
            return entry->setProperty(object, key, (unsigned short)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_INT:
        case osgDB::BaseSerializer::RW_GLENUM:
            return entry->setProperty(object, key, (int)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_UINT:
            return entry->setProperty(object, key, (unsigned int)atoi(value.c_str()));
        case osgDB::BaseSerializer::RW_FLOAT:
            return entry->setProperty(object, key, (float)atof(value.c_str()));
        case osgDB::BaseSerializer::RW_DOUBLE:
            return entry->setProperty(object, key, (double)atof(value.c_str()));
        case osgDB::BaseSerializer::RW_QUAT:
            return entry->setProperty(object, key, getQuatValue<osg::Quat>(value));
        case osgDB::BaseSerializer::RW_VEC2F:
            return entry->setProperty(object, key, getVecValue<osg::Vec2f>(value));
        case osgDB::BaseSerializer::RW_VEC3F:
            return entry->setProperty(object, key, getVecValue<osg::Vec3f>(value));
        case osgDB::BaseSerializer::RW_VEC4F:
            return entry->setProperty(object, key, getVecValue<osg::Vec4f>(value));
        case osgDB::BaseSerializer::RW_VEC2D:
            return entry->setProperty(object, key, getVecValue<osg::Vec2d>(value));
        case osgDB::BaseSerializer::RW_VEC3D:
            return entry->setProperty(object, key, getVecValue<osg::Vec3d>(value));
        case osgDB::BaseSerializer::RW_VEC4D:
            return entry->setProperty(object, key, getVecValue<osg::Vec4d>(value));
#if OSG_VERSION_GREATER_THAN(3, 4, 0)
        case osgDB::BaseSerializer::RW_VEC2B:
            return entry->setProperty(object, key, getVecValue<osg::Vec2b>(value));
        case osgDB::BaseSerializer::RW_VEC3B:
            return entry->setProperty(object, key, getVecValue<osg::Vec3b>(value));
        case osgDB::BaseSerializer::RW_VEC4B:
            return entry->setProperty(object, key, getVecValue<osg::Vec4b>(value));
        case osgDB::BaseSerializer::RW_VEC2UB:
            return entry->setProperty(object, key, getVecValue<osg::Vec2ub>(value));
        case osgDB::BaseSerializer::RW_VEC3UB:
            return entry->setProperty(object, key, getVecValue<osg::Vec3ub>(value));
        case osgDB::BaseSerializer::RW_VEC4UB:
            return entry->setProperty(object, key, getVecValue<osg::Vec4ub>(value));
        case osgDB::BaseSerializer::RW_VEC2S:
            return entry->setProperty(object, key, getVecValue<osg::Vec2s>(value));
        case osgDB::BaseSerializer::RW_VEC3S:
            return entry->setProperty(object, key, getVecValue<osg::Vec3s>(value));
        case osgDB::BaseSerializer::RW_VEC4S:
            return entry->setProperty(object, key, getVecValue<osg::Vec4s>(value));
        case osgDB::BaseSerializer::RW_VEC2US:
            return entry->setProperty(object, key, getVecValue<osg::Vec2us>(value));
        case osgDB::BaseSerializer::RW_VEC3US:
            return entry->setProperty(object, key, getVecValue<osg::Vec3us>(value));
        case osgDB::BaseSerializer::RW_VEC4US:
            return entry->setProperty(object, key, getVecValue<osg::Vec4us>(value));
        case osgDB::BaseSerializer::RW_VEC2I:
            return entry->setProperty(object, key, getVecValue<osg::Vec2i>(value));
        case osgDB::BaseSerializer::RW_VEC3I:
            return entry->setProperty(object, key, getVecValue<osg::Vec3i>(value));
        case osgDB::BaseSerializer::RW_VEC4I:
            return entry->setProperty(object, key, getVecValue<osg::Vec4i>(value));
        case osgDB::BaseSerializer::RW_VEC2UI:
            return entry->setProperty(object, key, getVecValue<osg::Vec2ui>(value));
        case osgDB::BaseSerializer::RW_VEC3UI:
            return entry->setProperty(object, key, getVecValue<osg::Vec3ui>(value));
        case osgDB::BaseSerializer::RW_VEC4UI:
            return entry->setProperty(object, key, getVecValue<osg::Vec4ui>(value));
#endif
        case osgDB::BaseSerializer::RW_MATRIXF:
            return entry->setProperty(object, key, getMatrixValue<osg::Matrixf>(value));
        case osgDB::BaseSerializer::RW_MATRIXD:
            return entry->setProperty(object, key, getMatrixValue<osg::Matrixd>(value));
        case osgDB::BaseSerializer::RW_MATRIX:
            return entry->setProperty(object, key, getMatrixValue<osg::Matrix>(value));
        case osgDB::BaseSerializer::RW_STRING:
            return entry->setProperty(object, key, value);
        case osgDB::BaseSerializer::RW_ENUM:
            return entry->setEnumProperty(object, key, value);
        case osgDB::BaseSerializer::RW_VECTOR:
            if (clsName == "FloatArray")
                return entry->setProperty(object, key, getVector<float>(value));
            else if (clsName == "Vec2Array")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec2f>(value, sep));
            else if (clsName == "Vec3Array")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec3f>(value, sep));
            else if (clsName == "Vec4Array")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec4f>(value, sep));
            else if (clsName == "DoubleArray")
                return entry->setProperty(object, key, getVector<double>(value));
            else if (clsName == "Vec2dArray")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec2d>(value, sep));
            else if (clsName == "Vec3dArray")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec3d>(value, sep));
            else if (clsName == "Vec4dArray")
                return entry->setVecProperty(object, key, getVecVector<osg::Vec4d>(value, sep));
            break;
        //RW_PLANE, RW_BOUNDINGBOXF, RW_BOUNDINGBOXD, RW_BOUNDINGSPHEREF, RW_BOUNDINGSPHERED
        }
#else
        OSG_WARN << "[ScriptBase] setProperty() not implemented" << std::endl;
#endif
    }
    return false;
}

template<typename T> static std::string setVecValue(const T& v)
{
    std::stringstream ss; int num = T::num_components;
    for (int i = 0; i < num; ++i) { if (i > 0) ss << " "; ss << v[i]; }
    return ss.str();
}

template<typename T> static std::string setQuatValue(const T& v)
{
    std::stringstream ss;
    for (int i = 0; i < 4; ++i) { if (i > 0) ss << " "; ss << v[i]; }
    return ss.str();
}

template<typename T> static std::string setMatrixValue(const T& v, int num = 16)
{
    std::stringstream ss; typename T::value_type* ptr = (typename T::value_type*)v.ptr();
    for (int i = 0; i < num; ++i) { if (i > 0) ss << " "; ss << *(ptr + i); }
    return ss.str();
}

template<typename T> static std::string setVector(const std::vector<T>& v)
{
    std::stringstream ss;
    for (size_t i = 0; i < v.size(); ++i)
    { if (i > 0) ss << " "; ss << v[i]; }
    return ss.str();
}

template<typename T> static std::string setVecVector(const std::vector<T>& v, char s)
{
    std::stringstream ss; int num = T::num_components;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const T& vec = v[i]; if (i > 0) ss << " ";
        for (int j = 0; j < num; ++j) { if (j > 0) ss << s; ss << vec[j]; }
    }
    return ss.str();
}

bool ScriptBase::getProperty(const std::string& key, std::string& value,
                             LibraryEntry* entry, osg::Object* object,
                             const std::vector<LibraryEntry::Property>& names)
{
    std::string clsName = object->className();
    std::string value2; char sep = _vecSeparator;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const LibraryEntry::Property& prop = names[i];
        if (prop.name != key || prop.outdated) continue;

#define GET_PROP_VALUE(type, func) { \
    type v; if (!entry->getProperty(object, key, v)) return false; \
    value = func (v); return true; }
#define GET_PROP_VALUE2(type, func, arg) { \
    type v; if (!entry->getProperty(object, key, v)) return false; \
    value = func (v, arg); return true; }

#if OSGVERSE_COMPLETED_SCRIPT
        switch (prop.type)
        {
        //case osgDB::BaseSerializer::RW_OBJECT:
        //case osgDB::BaseSerializer::RW_IMAGE:
        //case osgDB::BaseSerializer::RW_BOOL:
        case osgDB::BaseSerializer::RW_CHAR: GET_PROP_VALUE(char, std::to_string);
        case osgDB::BaseSerializer::RW_UCHAR: GET_PROP_VALUE(unsigned char, std::to_string);
        case osgDB::BaseSerializer::RW_SHORT: GET_PROP_VALUE(short, std::to_string);
        case osgDB::BaseSerializer::RW_USHORT: GET_PROP_VALUE(unsigned short, std::to_string);
        case osgDB::BaseSerializer::RW_INT: GET_PROP_VALUE(int, std::to_string);
        case osgDB::BaseSerializer::RW_GLENUM: GET_PROP_VALUE(GLenum, std::to_string);
        case osgDB::BaseSerializer::RW_UINT: GET_PROP_VALUE(unsigned int, std::to_string);
        case osgDB::BaseSerializer::RW_FLOAT: GET_PROP_VALUE(float, std::to_string);
        case osgDB::BaseSerializer::RW_DOUBLE: GET_PROP_VALUE(double, std::to_string);
        case osgDB::BaseSerializer::RW_QUAT: GET_PROP_VALUE(osg::Quat, setQuatValue);
        case osgDB::BaseSerializer::RW_VEC2F: GET_PROP_VALUE(osg::Vec2f, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3F: GET_PROP_VALUE(osg::Vec3f, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4F: GET_PROP_VALUE(osg::Vec4f, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2D: GET_PROP_VALUE(osg::Vec2d, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3D: GET_PROP_VALUE(osg::Vec3d, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4D: GET_PROP_VALUE(osg::Vec4d, setVecValue);
#if OSG_VERSION_GREATER_THAN(3, 4, 0)
        case osgDB::BaseSerializer::RW_VEC2B: GET_PROP_VALUE(osg::Vec2b, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3B: GET_PROP_VALUE(osg::Vec3b, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4B: GET_PROP_VALUE(osg::Vec4b, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2UB: GET_PROP_VALUE(osg::Vec2ub, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3UB: GET_PROP_VALUE(osg::Vec3ub, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4UB: GET_PROP_VALUE(osg::Vec4ub, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2S: GET_PROP_VALUE(osg::Vec2s, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3S: GET_PROP_VALUE(osg::Vec3s, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4S: GET_PROP_VALUE(osg::Vec4s, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2US: GET_PROP_VALUE(osg::Vec2us, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3US: GET_PROP_VALUE(osg::Vec3us, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4US: GET_PROP_VALUE(osg::Vec4us, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2I: GET_PROP_VALUE(osg::Vec2i, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3I: GET_PROP_VALUE(osg::Vec3i, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4I: GET_PROP_VALUE(osg::Vec4i, setVecValue);
        case osgDB::BaseSerializer::RW_VEC2UI: GET_PROP_VALUE(osg::Vec2ui, setVecValue);
        case osgDB::BaseSerializer::RW_VEC3UI: GET_PROP_VALUE(osg::Vec3ui, setVecValue);
        case osgDB::BaseSerializer::RW_VEC4UI: GET_PROP_VALUE(osg::Vec4ui, setVecValue);
#endif
        case osgDB::BaseSerializer::RW_MATRIXF: GET_PROP_VALUE(osg::Matrixf, setMatrixValue);
        case osgDB::BaseSerializer::RW_MATRIXD: GET_PROP_VALUE(osg::Matrixd, setMatrixValue);
        case osgDB::BaseSerializer::RW_MATRIX: GET_PROP_VALUE(osg::Matrix, setMatrixValue);
        case osgDB::BaseSerializer::RW_STRING: GET_PROP_VALUE(std::string, std::string);
        case osgDB::BaseSerializer::RW_ENUM:
            value = entry->getEnumProperty(object, key);
            return !value.empty();
        case osgDB::BaseSerializer::RW_VECTOR:
            if (clsName == "FloatArray")
                GET_PROP_VALUE(std::vector<float>, setVector)
            else if (clsName == "Vec2Array")
                GET_PROP_VALUE2(std::vector<osg::Vec2f>, setVecVector, sep)
            else if (clsName == "Vec3Array")
                GET_PROP_VALUE2(std::vector<osg::Vec3f>, setVecVector, sep)
            else if (clsName == "Vec4Array")
                GET_PROP_VALUE2(std::vector<osg::Vec4f>, setVecVector, sep)
            else if (clsName == "DoubleArray")
                GET_PROP_VALUE(std::vector<double>, setVector)
            else if (clsName == "Vec2dArray")
                GET_PROP_VALUE2(std::vector<osg::Vec2d>, setVecVector, sep)
            else if (clsName == "Vec3dArray")
                GET_PROP_VALUE2(std::vector<osg::Vec3d>, setVecVector, sep)
            else if (clsName == "Vec4dArray")
                GET_PROP_VALUE2(std::vector<osg::Vec4d>, setVecVector, sep)
            break;
            //RW_PLANE, RW_BOUNDINGBOXF, RW_BOUNDINGBOXD, RW_BOUNDINGSPHEREF, RW_BOUNDINGSPHERED
        }
#else
        OSG_WARN << "[ScriptBase] getProperty() not implemented" << std::endl;
#endif
    }
    return false;
}
