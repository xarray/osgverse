#ifndef MANA_UI_SERIALIZERINTERFACE_HPP
#define MANA_UI_SERIALIZERINTERFACE_HPP

#include <osgDB/Serializer>
#include "ImGui.h"
#include "ImGuiComponents.h"
#include "../script/Entry.h"
#include <map>

namespace osgVerse
{
    class SerializerInterface : public ImGuiComponentBase
    {
    public:
        SerializerInterface(osg::Object* obj, LibraryEntry* entry,
                            const LibraryEntry::Property& prop, bool composited);
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content) = 0;

        void addIndent(float incr) { _indent += incr; }

        void dirty() { _dirty = true; }
        bool isDirty() const { return _dirty; }

    protected:
        osg::observer_ptr<osg::Object> _object;
        osg::ref_ptr<LibraryEntry> _entry;
        LibraryEntry::Property _property;
        std::string _postfix;
        float _indent;
        bool _composited, _selected, _dirty;
    };

    class SerializerFactory : public osg::Referenced
    {
    public:
        typedef std::function<SerializerInterface* (
            osg::Object*, LibraryEntry*, const LibraryEntry::Property&)> InterfaceFunction;
        static SerializerFactory* instance();

        void registerInterface(osgDB::BaseSerializer::Type t, InterfaceFunction func)
        { _creatorMap[t] = func; }

        void unregisterInterface(osgDB::BaseSerializer::Type t)
        { if (_creatorMap.find(t) != _creatorMap.end()) _creatorMap.erase(_creatorMap.find(t)); }

        LibraryEntry* createInterfaces(osg::Object* obj, LibraryEntry* lastEntry,
                                       std::vector<osg::ref_ptr<SerializerInterface>>& interfaces);
        SerializerInterface* createInterface(osg::Object* obj, LibraryEntry* entry,
                                             const LibraryEntry::Property& prop);

    protected:
        SerializerFactory() {}
        virtual ~SerializerFactory() {}
        std::map<osgDB::BaseSerializer::Type, InterfaceFunction> _creatorMap;
    };

    struct SerializerInterfaceProxy
    {
        SerializerInterfaceProxy(osgDB::BaseSerializer::Type t, SerializerFactory::InterfaceFunction func)
        { SerializerFactory::instance()->registerInterface(t, func); }
    };
}

#define REGISTER_SERIALIZER_INTERFACE(t, clsName) \
    osgVerse::SerializerInterface* serializerInterfaceFunc_##clsName (osg::Object* obj, \
        osgVerse::LibraryEntry* entry, const osgVerse::LibraryEntry::Property& prop) \
        { return new clsName (obj, entry, prop); } \
    extern "C" void serializerInterfaceFuncCaller_##t () {} \
    static osgVerse::SerializerInterfaceProxy proxy_##clsName \
    (osgDB::BaseSerializer::##t, serializerInterfaceFunc_##clsName );

#define USE_SERIALIZER_INTERFACE(t) \
    extern "C" void serializerInterfaceFuncCaller_##t (); \
    static osgDB::PluginFunctionProxy callerProxy_##t (serializerInterfaceFuncCaller_##t );

#endif
