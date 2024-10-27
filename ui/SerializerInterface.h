#ifndef MANA_UI_SERIALIZERINTERFACE_HPP
#define MANA_UI_SERIALIZERINTERFACE_HPP

#include <osgDB/Serializer>
#include "ImGui.h"
#include "ImGuiComponents.h"
#include "../script/Entry.h"
#include <map>

namespace osgVerse
{
    class SerializerBaseItem : public ImGuiComponentBase
    {
    public:
        enum ItemType
        {
            GenericType = 0, ObjectType, StringType,
            VectorType, ListType
        };
        SerializerBaseItem(osg::Object* obj, bool composited);

        virtual ItemType getType() const { return GenericType; };
        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content) = 0;
        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter) = 0;

        void addIndent(float incr) { _indent += incr; }
        void dirty() { _dirty = true; }
        void doneEditing() { _edited = true; }

        bool isDirty() const { return _dirty; }
        bool isHidden() const { return _hidden; }
        bool checkEdited() { bool b = _edited; _edited = false; return b; }

    protected:
        virtual bool showInternal(ImGuiManager* mgr, ImGuiContentHandler* content, const std::string& title);
        virtual void showMenuItems(ImGuiManager* mgr, ImGuiContentHandler* content);
        std::string tooltip(const LibraryEntry::Property& prop, const std::string& postfix = "") const;

        osg::observer_ptr<osg::Object> _object;
        std::string _postfix; float _indent;
        bool _composited, _selected, _dirty;
        bool _hidden, _edited;
    };

    class SerializerInterface : public SerializerBaseItem
    {
    public:
        SerializerInterface(osg::Object* obj, LibraryEntry* entry,
                            const LibraryEntry::Property& prop, bool composited);
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        virtual int createSpiderNode(SpiderEditor* spider, bool getter, bool setter);

    protected:
        osg::ref_ptr<LibraryEntry> _entry;
        LibraryEntry::Property _property;
    };

    class SerializerFactory : public osg::Referenced
    {
    public:
        typedef std::function<SerializerBaseItem* (
            osg::Object*, LibraryEntry*, const LibraryEntry::Property&)> InterfaceFunction;
        static SerializerFactory* instance();

        void registerInterface(osgDB::BaseSerializer::Type t, InterfaceFunction func)
        { _creatorMap[t] = func; }

        void registerInterface(const std::string& prop, osg::Object* ref, InterfaceFunction func)
        {
            std::string cName = ref ? std::string(ref->libraryName()) + "::" + ref->className() : "";
            _userCreatorMap[prop][cName] = func;
        }

        void registerBlacklist(const std::string& prop, osg::Object* ref)
        {
            std::string cName = ref ? std::string(ref->libraryName()) + "::" + ref->className() : "";
            _blacklistMap[prop].insert(cName);
        }

        void unregisterInterface(osgDB::BaseSerializer::Type t)
        { if (_creatorMap.find(t) != _creatorMap.end()) _creatorMap.erase(_creatorMap.find(t)); }

        void unregisterInterface(const std::string& prop, osg::Object* ref)
        {
            std::string cName = ref ? std::string(ref->libraryName()) + "::" + ref->className() : "";
            InterfaceFunctionMap& funcMap = _userCreatorMap[prop];
            if (funcMap.find(cName) != funcMap.end())
            {
                funcMap.erase(funcMap.find(cName));
                if (funcMap.empty()) _userCreatorMap.erase(_userCreatorMap.find(prop));
            }
        }

        void unregisterBlacklist(const std::string& prop, osg::Object* ref)
        {
            std::set<std::string>& bl = _blacklistMap[prop];
            std::string cName = ref ? std::string(ref->libraryName()) + "::" + ref->className() : "";
            std::set<std::string>::iterator it = bl.find(cName); if (it != bl.end()) { bl.erase(it); }
            if (bl.empty()) _blacklistMap.erase(_blacklistMap.find(prop));
        }

        /** Create serializer UI items of given object */
        LibraryEntry* createInterfaces(osg::Object* obj, LibraryEntry* lastEntry,
                                       std::vector<osg::ref_ptr<SerializerBaseItem>>& interfaces);

    protected:
        SerializerFactory() {}
        virtual ~SerializerFactory() {}
        
        SerializerBaseItem* createInterface(osg::Object* obj, LibraryEntry* entry,
                                            const LibraryEntry::Property& prop);

        typedef std::map<std::string, InterfaceFunction> InterfaceFunctionMap;
        std::map<osgDB::BaseSerializer::Type, InterfaceFunction> _creatorMap;
        std::map<std::string, InterfaceFunctionMap> _userCreatorMap;
        std::map<std::string, std::set<std::string>> _blacklistMap;
    };

    struct SerializerInterfaceProxy
    {
        SerializerInterfaceProxy(osgDB::BaseSerializer::Type t, SerializerFactory::InterfaceFunction func)
        { SerializerFactory::instance()->registerInterface(t, func); }

        SerializerInterfaceProxy(const std::string& p, osg::Object* r, SerializerFactory::InterfaceFunction f)
        { osg::ref_ptr<osg::Object> ref(r); SerializerFactory::instance()->registerInterface(p, r, f); }
    };
}

#define REGISTER_SERIALIZER_INTERFACE(t, clsName) \
    osgVerse::SerializerInterface* serializerInterfaceFunc_##t##clsName (osg::Object* obj, \
        osgVerse::LibraryEntry* entry, const osgVerse::LibraryEntry::Property& prop) \
        { return new clsName (obj, entry, prop); } \
    extern "C" void serializerInterfaceFuncCaller_##t () {} \
    static osgVerse::SerializerInterfaceProxy proxy_##t##clsName \
    (osgDB::BaseSerializer::RW_##t, serializerInterfaceFunc_##t##clsName );

#define REGISTER_SERIALIZER_INTERFACE2(t, refInstance, clsName) \
    osgVerse::SerializerInterface* serializerInterfaceFunc_##t##clsName (osg::Object* obj, \
        osgVerse::LibraryEntry* entry, const osgVerse::LibraryEntry::Property& prop) \
        { return new clsName (obj, entry, prop); } \
    extern "C" void serializerInterfaceFuncCaller_##t () {} \
    static osgVerse::SerializerInterfaceProxy proxy_##t##clsName \
    (#t, refInstance, serializerInterfaceFunc_##t##clsName );

#define USE_SERIALIZER_INTERFACE(t) \
    extern "C" void serializerInterfaceFuncCaller_##t (); \
    static osgDB::PluginFunctionProxy callerProxy_##t (serializerInterfaceFuncCaller_##t );

#endif
