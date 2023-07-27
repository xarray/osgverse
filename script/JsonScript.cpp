#include <osg/Version>
#include "JsonScript.h"
using namespace osgVerse;

picojson::value JsonScript::execute(ExecutionType t, picojson::value in)
{
    PropertyMap properties; ParameterList params;
    if (in.contains("properties"))
    {
        const picojson::value& propsVal = in.get("properties");
        if (propsVal.is<picojson::array>())
        {
            size_t index = 0;
            while (propsVal.contains(index))
            {
                const picojson::value& propVal = propsVal.get(index++);
                if (propVal.is<picojson::object>())
                {
                    const picojson::object& obj = propVal.get<picojson::object>();
                    for (picojson::object::const_iterator itr = obj.begin();
                         itr != obj.end(); ++itr) properties[itr->first] = itr->second.to_str();
                }
                else if (!propVal.is<picojson::array>())
                    params.push_back(propVal.to_str());
            }
        }
        else if (propsVal.is<picojson::object>())
        {
            const picojson::object& obj = propsVal.get<picojson::object>();
            for (picojson::object::const_iterator itr = obj.begin();
                 itr != obj.end(); ++itr) properties[itr->first] = itr->second.to_str();
        }
        else
            OSG_WARN << "[JsonScript] Unknown properties format: "
                     << propsVal.to_str() << std::endl;
    }

    Result result; bool valueIsJson = false;
    switch (t)
    {
    case EXE_Creation:
        if (in.contains("class"))
        {
            const picojson::value& classVal = in.get("class");
            result = create(classVal.to_str(), properties);
        }
        else if (in.contains("uri"))
        {
            const picojson::value& uriVal = in.get("uri");
            const picojson::value& typeVal = in.get("type");
            result = create(typeVal.to_str(), uriVal.to_str(), properties);
        }
        else
        {
            OSG_WARN << "[JsonScript] Creation command without key data: "
                     << in.to_str() << std::endl;
            result.code = -10; result.msg = "Incomplete JSON command";
        }
        break;
    case EXE_Set:
        if (in.contains("object"))
        {
            const picojson::value& objVal = in.get("object");
            if (in.contains("method"))
            {
                const picojson::value& methodVal = in.get("method");
                result = call(objVal.to_str(), methodVal.to_str(), params);
            }
            else
                result = set(objVal.to_str(), properties);
        }
        else
        {
            OSG_WARN << "[JsonScript] Set command without key data: "
                     << in.to_str() << std::endl;
            result.code = -10; result.msg = "Incomplete JSON command";
        }
        break;
    case EXE_Get:
        if (in.contains("object") && in.contains("property"))
        {
            const picojson::value& objVal = in.get("object");
            const picojson::value& propVal = in.get("property");
            result = get(objVal.to_str(), propVal.to_str());
        }
        else
        {
            OSG_WARN << "[JsonScript] Get command without key data: "
                     << in.to_str() << std::endl;
            result.code = -10; result.msg = "Incomplete JSON command";
        }
        break;
    case EXE_Remove:
        if (in.contains("object"))
            result = remove(in.get("object").to_str());
        else
        {
            OSG_WARN << "[JsonScript] Remove command without key data: "
                     << in.to_str() << std::endl;
            result.code = -10; result.msg = "Incomplete JSON command";
        }
        break;
    case EXE_List:
        valueIsJson = true;
        if (in.contains("object"))
        {
            std::string objName = in.get("object").to_str();
            osg::Object* obj = getFromPath(objName);
            if (obj != NULL)
            {
                result.value = "{\"class\": \"" + std::string(obj->className())
                    + "\", \"library\": \"" + std::string(obj->libraryName())
                    + "\", \"referenced\": " + std::to_string(obj->referenceCount());
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
                if (obj->asNode())
                {
                    osg::Node* node = obj->asNode();
                    if (node->asGroup())
                    {
                        unsigned int num = node->asGroup()->getNumChildren();
                        result.value += ", \"children\": \"" + std::to_string(num) + "\"";
                    }
                }
#endif
                result.value += "}";
                result.obj = obj;
            }
            else result.value = "{}";
        }
        else if (in.contains("class"))
        {
            std::string clsName = in.get("class").to_str(), value1, value2;
            std::string libName = in.contains("library") ? in.get("library").to_str() : "osg";
            LibraryEntry* entry = getOrCreateEntry(libName);

            std::vector<osgVerse::LibraryEntry::Property> props = entry->getPropertyNames(clsName);
            for (size_t i = 0; i < props.size(); ++i)
            {
                if (props[i].outdated) continue;
                if (value1.empty()) value1 = "\"properties\": ["; else value1 += ", ";
                value1 += "{\"name\": \"" + props[i].name + "\", "
                           "\"type\": \"" + props[i].typeName + "\"}";
            }
            if (!value1.empty()) value1 += "]"; else value1 = "\"properties\": []";

            std::vector<osgVerse::LibraryEntry::Method> methods = entry->getMethodNames(clsName);
            for (size_t i = 0; i < methods.size(); ++i)
            {
                if (methods[i].outdated) continue;
                if (value2.empty()) value2 = "\"methods\": ["; else value2 += ", ";
                value2 += "{\"name\": \"" + methods[i].name + "\"}";
            }
            if (!value2.empty()) value2 += "]"; else value2 = "\"methods\": []";

            result.value = "{" + value1 + ", " + value2 + ", \"class\": \"" + clsName
                         + "\", \"library\": \"" + libName + "\"}";
        }
        else if (in.contains("library"))
        {
            std::string libName = in.get("library").to_str();
            LibraryEntry* entry = getOrCreateEntry(libName);
            const std::set<std::string>& classes = entry->getClasses();
            if (classes.empty()) break;

            std::set<std::string>::const_iterator itr = classes.begin();
            result.value = "{\"classes\": [\"" + *itr + "\""; itr++;
            for (; itr != classes.end(); ++itr) result.value += ", \"" + (*itr) + "\"";
            result.value += "], \"library\": \"" + libName + "\"}";
        }
        else
        {
            OSG_WARN << "[JsonScript] List command without key data: "
                     << in.to_str() << std::endl;
            result.code = -10; result.msg = "Incomplete JSON command";
        }
        break;
    }

    picojson::object retValues;
    retValues["code"] = picojson::value((double)result.code);
    retValues["message"] = picojson::value(result.msg.empty() ? "ok" : result.msg);
    if (valueIsJson) picojson::parse(retValues["value"], result.value);
    else retValues["value"] = picojson::value(result.value);

    if (result.obj.valid())
    {
        std::string name = result.obj->getName();
        if (name.find("vobj") == name.npos)
        {
            createFromObject(result.obj.get());
            name = result.obj->getName();  // create ID for unregistered node
        }
        retValues["object"] = picojson::value(name);
    }
    return picojson::value(retValues);
}
