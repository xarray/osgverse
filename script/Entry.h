#ifndef MANA_SCRIPT_ENTRY_HPP
#define MANA_SCRIPT_ENTRY_HPP

#include <osgDB/ClassInterface>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <map>
#include <set>
#include <iostream>

namespace osgVerse
{
    class LibraryEntry
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
                void* ptr = vs->getElement(*object, i);
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
            if (!vs) return false;

            vs->clear(*object);
            for (size_t i = 0; i < value.size(); ++i)
                vs->addElement(*object, (void*)value[i].ptr());
            return true;
        }

        std::string getEnumProperty(const osg::Object* object, const std::string& name);
        bool setEnumProperty(osg::Object* object, const std::string& name, const std::string& value);

        osg::Object* callMethod(osg::Object* object, const std::string& name);
        bool callMethod(osg::Object* object, const std::string& name, osg::Object* arg1);
        bool callMethod(osg::Object* object, const std::string& name,
                        osg::Parameters& args0, osg::Parameters& args1);
        
    protected:
        osgDB::ClassInterface _manager;
        std::set<std::string> _classes;
        std::string _libraryName;
    };
}

#endif
