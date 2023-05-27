#ifndef MANA_SCRIPT_SCRIPTBASE_HPP
#define MANA_SCRIPT_SCRIPTBASE_HPP

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include "Entry.h"

namespace osgVerse
{
    class ScriptBase : public osg::Referenced
    {
    public:
        typedef std::map<std::string, std::string> PropertyMap;
        typedef std::vector<std::string> ParameterList;

        struct Result
        {
            Result(int c = 0, const std::string& m = "") : code(c), msg(m) {}
            Result(const std::string& v) : code(0), value(v) {}
            int code; std::string msg, value;
        };

        /** POST: create an object with properties
        *   Example: /scene/create { 'class': '...', 'properties': ['...': '...'] }
        *            Result = { 'code': ..., 'msg': '...', 'id': ... } */
        Result create(const std::string& clsName, const PropertyMap& properties);

        /** PUT: find an object and set properties/call methods
        *   Example: /scene/idXXX { 'set'/'call': '...', 'value': '...' }
        *            Result = { 'code': ..., 'msg': '...' } */
        Result set(const std::string& nodePath, const PropertyMap& properties);
        Result call(const std::string& nodePath, const std::string& key,
                    const ParameterList& params);

        /** GET: find an object and get its property
        *   Example: /scene/idXXX { 'get': '...' }
        *            Result = { 'code': ..., 'msg': '...', 'value': '...' } */
        Result get(const std::string& nodePath, const std::string& key);

        /** DELETE: delete an existing object
        *   Example: /scene/idXXX
        *            Result = { 'code': ..., 'msg': '...' } */
        Result remove(const std::string& nodePath);

        /** Get node path:
        *   /scene/idXXX, /scene/nameA/nameB, /scene/nameA/0 (first child) */
        osg::Object* getFromPath(const std::string& nodePath);

    protected:
        bool setProperty(const std::string& key, const std::string& value,
                         LibraryEntry* entry, osg::Object* object,
                         const std::vector<LibraryEntry::Property>& names);

        std::map<std::string, osg::ref_ptr<osg::Object>> _objects;
        std::map<std::string, osg::ref_ptr<LibraryEntry>> _entries;
    };
}

#endif
