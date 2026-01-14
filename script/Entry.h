#ifndef MANA_SCRIPT_ENTRY_HPP
#define MANA_SCRIPT_ENTRY_HPP

#include <osg/Version>
#include <osg/Object>
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
#   include <osgDB/ClassInterface>
#   define OSGVERSE_COMPLETED_SCRIPT 1
#else
#   define OSGVERSE_COMPLETED_SCRIPT 0
namespace osg
{ typedef std::vector< osg::ref_ptr<osg::Object> > Parameters; }
#endif

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <map>
#include <set>
#include <iostream>

namespace osgVerse
{
    class LibraryEntry : public osg::Referenced
    {
    public:
        LibraryEntry(const std::string& libName);
        
        void refresh(const std::string& libName);
        const std::set<std::string>& getClasses() const { return _classes; }

        struct Property
        {
            std::string name, typeName, ownerClass;
            osgDB::BaseSerializer::Type type; bool outdated;
            Property() : type(osgDB::BaseSerializer::RW_UNDEFINED), outdated(false) {}
        };
        std::vector<Property> getPropertyNames(const std::string& clsName) const;

        struct Method
        {
            std::string name, ownerClass; bool outdated;
            Method() : outdated(false) {}
        };
        std::vector<Method> getMethodNames(const std::string& clsName) const;

#if OSGVERSE_COMPLETED_SCRIPT
        template<typename T>
        bool getProperty(const osg::Object* object, const std::string& name, T& value)
        { return _manager.getProperty<T>(object, name, value); }
        
        template<typename T>
        bool setProperty(osg::Object* object, const std::string& name, const T& value)
        { return _manager.setProperty<T>(object, name, value); }

        template<typename T>
        bool getProperty(const osg::Object* object, const std::string& name, std::vector<T>& value)
        {
            osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
            osgDB::VectorBaseSerializer* vs = dynamic_cast<osgDB::VectorBaseSerializer*>(
                _manager.getSerializer(object, name, type));
            if (!vs) return false;

            unsigned int size = vs->size(*object);
            for (size_t i = 0; i < size; ++i)
            {
                const void* ptr = vs->getElement(*object, i);
                value.push_back(*(T*)ptr);
            }
            return true;
        }

        template<typename T>
        bool setProperty(osg::Object* object, const std::string& name, const std::vector<T>& value)
        {
            osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
            osgDB::VectorBaseSerializer* vs = dynamic_cast<osgDB::VectorBaseSerializer*>(
                _manager.getSerializer(object, name, type));
            if (!vs) return false; else vs->clear(*object);
            
            for (size_t i = 0; i < value.size(); ++i)
                vs->addElement(*object, (void*)&value[i]);
            return true;
        }

        template<typename T>
        bool setVecProperty(osg::Object* object, const std::string& name, const std::vector<T>& value)
        {
            osgDB::BaseSerializer::Type type = osgDB::BaseSerializer::RW_UNDEFINED;
            osgDB::VectorBaseSerializer* vs = dynamic_cast<osgDB::VectorBaseSerializer*>(
                _manager.getSerializer(object, name, type));
            if (!vs) return false; else vs->clear(*object);

            for (size_t i = 0; i < value.size(); ++i)
                vs->addElement(*object, (void*)value[i].ptr());
            return true;
        }
#else
        template<typename T>
        bool getProperty(const osg::Object* object, const std::string& name, T& value)
        {
            OSG_WARN << "[LibraryEntry] getProperty() not implemented" << std::endl;
            return false;
        }

        template<typename T>
        bool setProperty(osg::Object* object, const std::string& name, const T& value)
        {
            OSG_WARN << "[LibraryEntry] setProperty() not implemented" << std::endl;
            return false;
        }

        template<typename T>
        bool getProperty(const osg::Object* object, const std::string& name, std::vector<T>& value)
        {
            OSG_WARN << "[LibraryEntry] getProperty() not implemented" << std::endl;
            return false;
        }

        template<typename T>
        bool setProperty(osg::Object* object, const std::string& name, const std::vector<T>& value)
        {
            OSG_WARN << "[LibraryEntry] setProperty() not implemented" << std::endl;
            return false;
        }

        template<typename T>
        bool setVecProperty(osg::Object* object, const std::string& name, const std::vector<T>& value)
        {
            OSG_WARN << "[LibraryEntry] setVecProperty() not implemented" << std::endl;
            return false;
        }
#endif

        std::vector<std::string> getEnumPropertyItems(const osg::Object* object, const std::string& name);
        std::string getEnumProperty(const osg::Object* object, const std::string& name);
        bool setEnumProperty(osg::Object* object, const std::string& name, const std::string& value);

        osg::Object* callMethod(osg::Object* object, const std::string& name);
        bool callMethod(osg::Object* object, const std::string& name, osg::Object* arg1);
        bool callMethod(osg::Object* object, const std::string& name,
                        osg::Parameters& args0, osg::Parameters& args1);

        osg::Object* create(const std::string& clsName);
        std::string getLibraryName() const { return _libraryName; }
        static std::string getClassName(osg::Object* obj, bool withLibName);
        
    protected:
#if OSGVERSE_COMPLETED_SCRIPT
        osgDB::ClassInterface _manager;
#endif
        std::set<std::string> _classes;
        std::string _libraryName;
    };

    class MethodInformationManager : public osg::Referenced
    {
    public:
        typedef osgDB::BaseSerializer::Type ArgType;
        static MethodInformationManager* instance();

        struct Argument
        {
            std::string name; ArgType type; bool optional;
            Argument() : type(ArgType::RW_UNDEFINED), optional(false) {}
            Argument(const std::string& n, ArgType t, bool o = false) : name(n), type(t), optional(o) {}
        };

        typedef std::pair<std::vector<Argument>, std::vector<Argument>> MethodInformation;  // <in-args, out-args>
        void addInformation(osgDB::ObjectWrapper* cls, const std::string& methodName, const MethodInformation& info);
        void removeInformation(osgDB::ObjectWrapper* cls, const std::string& methodName);

        void addInformation(osgDB::ObjectWrapper* cls, const std::string& method, const std::vector<Argument>& argsIn)
        { addInformation(cls, method, MethodInformation(argsIn, std::vector<Argument>())); }
        void addInformation(osgDB::ObjectWrapper* cls, const std::string& method,
                            const std::vector<Argument>& argsIn, const std::vector<Argument>& argsOut)
        { addInformation(cls, method, MethodInformation(argsIn, argsOut)); }

        std::vector<Argument>& getInformation(osgDB::ObjectWrapper* cls, const std::string& methodName);
        const std::vector<Argument>& getInformation(osgDB::ObjectWrapper* cls, const std::string& methodName) const;

        typedef std::pair<osg::observer_ptr<osgDB::ObjectWrapper>, std::string> ClassAndMethod;
        std::map<ClassAndMethod, MethodInformation>& getInformationMap() { return _informationMap; }
        const std::map<ClassAndMethod, MethodInformation>& getInformationMap() const { return _informationMap; }

    protected:
        MethodInformationManager();
        std::map<ClassAndMethod, MethodInformation> _informationMap;
        std::vector<MethodInformationManager::Argument> _defaultArguments;
    };
}


/** osgDB::MethodObject macros
    Usage example:
    METHOD_BEGIN(osg, StateSet, getTextureAttribute)
    if (in.size() > 1)  // osg::StateSet::getTextureAttribute(int u, Type type)
    {
        unsigned int unit = 0, type = 0;
        ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[1], type);
        osg::StateAttribute* sa = object->getTextureAttribute(unit, (osg::StateAttribute::Type)type);
        if (sa) out.push_back(sa); return true;
    }
    METHOD_END()
*/
#define METHOD_BEGIN(ns, clsName, method) \
    struct MethodStruct_##ns##_##clsName##_##method : public osgDB::MethodObject { \
        virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const { \
            ns :: clsName* object = reinterpret_cast<ns :: clsName*>(objectPtr); if (!object) return false;
#define ARG_TO_VALUE(obj, var) { if ((obj)->asValueObject()) (obj)->asValueObject()->getScalarValue(var); }
#define VALUE_TO_ARG(objClass, var, obj) osg:: objClass* obj = new osg:: objClass(var);
#define METHOD_END() return false; } };
#define REGISTER_METHOD(wrapper, ns, clsName, method) \
    wrapper->addMethodObject(#method, new MethodStruct_##ns##_##clsName##_##method );

/** osgVerse::MethodInformation macros
    Usage example:
    // osg::StateSet::getTextureAttribute(int u, Type type)
    METHOD_INFO_IN2_OUT1(wrapper, "getTextureAttribute", ARG_INFO("unit", RW_UINT), ARG_INFO("type", RW_UINT),
                                                         ARG_INFO("attribute", RW_OBJECT));
*/
#define METHOD_INFO_IN1(cls, method, arg0) { \
    std::vector<osgVerse::MethodInformationManager::Argument> args; args.push_back(arg0); \
    osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, args); }
#define METHOD_INFO_IN2(cls, method, arg0, arg1) { \
    std::vector<osgVerse::MethodInformationManager::Argument> args; args.push_back(arg0); args.push_back(arg1); \
    osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, args); }
#define METHOD_INFO_IN3(cls, method, arg0, arg1, arg2) { \
    std::vector<osgVerse::MethodInformationManager::Argument> args; args.push_back(arg0); args.push_back(arg1); \
     args.push_back(arg2);osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, args); }
#define METHOD_INFO_OUT1(cls, method, arg0) { \
    std::vector<osgVerse::MethodInformationManager::Argument> argsI, argsO; argsO.push_back(arg0); \
    osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, argsI, argsO); }
#define METHOD_INFO_IN1_OUT1(cls, method, aIn, aOut) { \
    std::vector<osgVerse::MethodInformationManager::Argument> argsI, argsO; argsI.push_back(aIn); argsO.push_back(aOut); \
    osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, argsI, argsO); }
#define METHOD_INFO_IN2_OUT1(cls, method, aIn0, aIn1, aOut) { \
    std::vector<osgVerse::MethodInformationManager::Argument> argsI, argsO; argsI.push_back(aIn0); argsI.push_back(aIn1); \
    argsO.push_back(aOut); osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, argsI, argsO); }
#define METHOD_INFO_IN3_OUT1(cls, method, aIn0, aIn1, aIn2, aOut) { \
    std::vector<osgVerse::MethodInformationManager::Argument> argsI, argsO; argsI.push_back(aIn0); argsI.push_back(aIn1); \
    argsI.push_back(aIn2); argsO.push_back(aOut); osgVerse::MethodInformationManager::instance()->addInformation(cls, #method, argsI, argsO); }
#define ARG_INFO(name, type) \
    osgVerse::MethodInformationManager::Argument(name, osgDB::BaseSerializer:: type, false)
#define OPTIONAL_ARG_INFO(name, type) \
    osgVerse::MethodInformationManager::Argument(name, osgDB::BaseSerializer:: type, true)

#endif
