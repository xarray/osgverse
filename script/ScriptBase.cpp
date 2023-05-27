#include <nanoid/nanoid.h>
#include <osgDB/Serializer>
#include "ScriptBase.h"
using namespace osgVerse;

ScriptBase::Result ScriptBase::create(const std::string& typeName,
                                      const PropertyMap& properties)
{
    std::string libName, clsName; std::size_t sep = typeName.find("::");
    if (sep == std::string::npos) { libName = "osg"; clsName = typeName; }
    else { libName = typeName.substr(0, sep); clsName = typeName.substr(sep + 2); }

    if (_entries.find(libName) == _entries.end())
        _entries[libName] = new LibraryEntry(libName);
    osg::Object* obj = _entries[libName]->create(clsName);

    if (obj != NULL)
    {
        LibraryEntry* entry = _entries[libName].get();
        std::string id = nanoid::generate(8);
        Result result(id); _objects[id] = obj;

        std::vector<LibraryEntry::Property> names = entry->getPropertyNames(clsName);
        for (PropertyMap::const_iterator itr = properties.begin();
             itr != properties.end(); ++itr)
        {
            if (!setProperty(itr->first, itr->second, entry, obj, names))
            {
                if (!result.msg.empty()) result.msg += "\n";
                result.msg += "Can't set property: " + itr->first;
            }
        }
        return result;
    }
    else
        return Result(-2, "Invalid creation type: " + typeName);
}

ScriptBase::Result ScriptBase::set(const std::string& nodePath, const PropertyMap& properties)
{
    osg::Object* obj = getFromPath(nodePath);
    if (obj != NULL)
    {
        std::string libName = obj->libraryName(), clsName = obj->className();
        if (_entries.find(libName) == _entries.end())
            _entries[libName] = new LibraryEntry(libName);

        Result result;
        LibraryEntry* entry = _entries[libName].get();
        std::vector<LibraryEntry::Property> names = entry->getPropertyNames(clsName);

        for (PropertyMap::const_iterator itr = properties.begin();
            itr != properties.end(); ++itr)
        {
            if (!setProperty(itr->first, itr->second, entry, obj, names))
            {
                if (!result.msg.empty()) result.msg += "\n";
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

        Result result;
        LibraryEntry* entry = _entries[libName].get();

        // TODO
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

        Result result;
        LibraryEntry* entry = _entries[libName].get();

        // TODO
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

        if (id.empty())
            return Result(-4, "Can't delete object which is not created here: " + nodePath);
        else if (obj->referenceCount() > 1)
            return Result(-5, "Can't delete object which is still referenced: " + nodePath);
        _objects.erase(_objects.find(id)); return Result();
    }
    else
        return Result(-1, "Invalid scene object: " + nodePath);
}

bool ScriptBase::setProperty(const std::string& key, const std::string& value,
                             LibraryEntry* entry, osg::Object* object,
                             const std::vector<LibraryEntry::Property>& names)
{
    for (size_t i = 0; i < names.size(); ++i)
    {
        const LibraryEntry::Property& prop = names[i];
        if (prop.name == key && !prop.outdated)
        {
            switch (prop.type)
            {
            case osgDB::BaseSerializer::RW_OBJECT:
            case osgDB::BaseSerializer::RW_IMAGE:
                return entry->setProperty(object, key, getFromPath(value));
            //RW_BOOL, RW_CHAR, RW_UCHAR, RW_SHORT, RW_USHORT, RW_INT, RW_UINT, RW_FLOAT, RW_DOUBLE,
            //RW_VEC2F, RW_VEC2D, RW_VEC3F, RW_VEC3D, RW_VEC4F, RW_VEC4D, RW_QUAT, RW_PLANE,
            //RW_MATRIXF, RW_MATRIXD, RW_MATRIX, RW_GLENUM, RW_STRING, RW_ENUM,
            //RW_VEC2B, RW_VEC2UB, RW_VEC2S, RW_VEC2US, RW_VEC2I, RW_VEC2UI,
            //RW_VEC3B, RW_VEC3UB, RW_VEC3S, RW_VEC3US, RW_VEC3I, RW_VEC3UI,
            //RW_VEC4B, RW_VEC4UB, RW_VEC4S, RW_VEC4US, RW_VEC4I, RW_VEC4UI,
            //RW_BOUNDINGBOXF, RW_BOUNDINGBOXD,
            //RW_BOUNDINGSPHEREF, RW_BOUNDINGSPHERED,
            //RW_VECTOR
            }
        }
    }
    return false;
}
