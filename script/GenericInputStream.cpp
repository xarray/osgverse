#include "GenericInputStream.h"
#include <nanoid/nanoid.h>
#include <stdarg.h>
using namespace osgVerse;

bool InputUserData::get(const char* name, ...)
{
    va_list params; va_start(params, name);
    // TODO
    va_end(params); return false;
}

bool InputUserData::get(const ObjectTypeAndID& parent, const char* name, ...)
{
    va_list params; va_start(params, name);
    // TODO
    va_end(params); return false;
}

void InputUserData::add(const char* name, ...)
{
    va_list params; va_start(params, name);
    // TODO
    va_end(params);
}

void InputUserData::add(const ObjectTypeAndID& parent, const char* name, ...)
{
    va_list params; va_start(params, name);
    // TODO
    va_end(params);
}

ObjectTypeAndID InputUserData::readObjectFromStream(InputStream& is, const std::string& type)
{
    // TODO
    return ObjectTypeAndID(type, "");
}
