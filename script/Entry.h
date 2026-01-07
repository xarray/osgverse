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
        static MethodInformationManager* instance();

        struct Argument
        {
            std::string name; osgDB::BaseSerializer::Type type; bool optional;
            Argument() : type(osgDB::BaseSerializer::RW_UNDEFINED), optional(false) {}
        };
        typedef std::pair<std::string, std::vector<Argument>> MethodInformation;

        void addInformation(const std::string& clsName, const std::string& methodName, const MethodInformation& info);
        void removeInformation(const std::string& clsName, const std::string& methodName);
        std::vector<Argument>& getInformation(const std::string& clsName, const std::string& methodName);
        const std::vector<Argument>& getInformation(const std::string& clsName, const std::string& methodName) const;

        typedef std::pair<std::string, std::string> ClassAndMethod;
        std::map<ClassAndMethod, MethodInformation>& getInformationMap() { return _informationMap; }
        const std::map<ClassAndMethod, MethodInformation>& getInformationMap() const { return _informationMap; }

    protected:
        MethodInformationManager();
        std::map<ClassAndMethod, MethodInformation> _informationMap;
    };
}

#endif
