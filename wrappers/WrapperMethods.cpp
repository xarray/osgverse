#include <osg/Geometry>
#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/Serializer>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>

#include <readerwriter/Utilities.h>
#include <script/Entry.h>
#include "Export.h"
using namespace osgVerse;

/////////////////////////////// STATESET
METHOD_BEGIN(osg, StateSet, getAttribute)
if (in.size() > 0)  // osg::StateSet::getAttribute(Type type)
{
    unsigned int type = 0; ARG_TO_VALUE(in[0], type);
    osg::StateAttribute* sa = object->getAttribute((osg::StateAttribute::Type)type);
    if (sa) out.push_back(sa); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, getTextureAttribute)
if (in.size() > 1)  // osg::StateSet::getTextureAttribute(unsigned int u, Type type)
{
    unsigned int unit = 0, type = 0; ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[1], type);
    osg::StateAttribute* sa = object->getTextureAttribute(unit, (osg::StateAttribute::Type)type);
    if (sa) out.push_back(sa); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, getMode)
if (in.size() > 0)  // osg::StateSet::getMode(GLenum mode)
{
    unsigned int mode = 0; ARG_TO_VALUE(in[0], mode);
    VALUE_TO_ARG(UIntValueObject, (unsigned int)object->getMode(mode), rtn);
    out.push_back(rtn); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, getTextureMode)
if (in.size() > 1)  // osg::StateSet::getTextureMode(unsigned int u, GLenum mode)
{
    unsigned int unit = 0, mode = 0; ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[1], mode);
    VALUE_TO_ARG(UIntValueObject, (unsigned int)object->getTextureMode(unit, mode), rtn);
    out.push_back(rtn); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, setAttribute)
if (in.size() > 1)  // osg::StateSet::setAttribute(osg::StateAttribute* sa, unsigned int flags)
{
    osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[0].get());
    unsigned int flags = 0; ARG_TO_VALUE(in[1], flags);
    if (sa != NULL) { object->setAttribute(sa, flags); return true; }
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, setTextureAttribute)
if (in.size() > 2)  // osg::StateSet::setTextureAttribute(unsigned int unit, osg::StateAttribute* sa, unsigned int flags)
{
    osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[1].get());
    unsigned int unit = 0, flags = 0; ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[2], flags);
    if (sa != NULL) { object->setTextureAttribute(unit, sa, flags); return true; }
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, setMode)
if (in.size() > 1)  // osg::StateSet::setMode(GLenum mode, unsigned int flags)
{
    unsigned int mode = 0, flags = 0; ARG_TO_VALUE(in[0], mode); ARG_TO_VALUE(in[1], flags);
    object->setMode(mode, flags); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, setTextureMode)
if (in.size() > 2)  // osg::StateSet::setTextureMode(unsigned int unit, GLenum mode, unsigned int flags)
{
    unsigned int unit = 0, mode = 0, flags = 0;
    ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[1], mode); ARG_TO_VALUE(in[2], flags);
    object->setTextureMode(unit, mode, flags); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, removeAttribute)
if (in.size() > 0)  // osg::StateSet::removeAttribute(osg::StateAttribute* sa)
{
    osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[0].get());
    if (sa != NULL) { object->removeAttribute(sa); return true; }
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, removeTextureAttribute)
if (in.size() > 1)  // osg::StateSet::removeTextureAttribute(unsigned int unit, osg::StateAttribute* sa)
{
    unsigned int unit = 0; ARG_TO_VALUE(in[0], unit);
    osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[1].get());
    if (sa != NULL) { object->removeTextureAttribute(unit, sa); return true; }
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, removeMode)
if (in.size() > 0)  // osg::StateSet::removeMode(GLenum mode)
{
    unsigned int mode = 0; ARG_TO_VALUE(in[0], mode);
    object->removeMode(mode); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, removeTextureMode)
if (in.size() > 1)  // osg::StateSet::removeTextureMode(unsigned int unit, GLenum mode)
{
    unsigned int unit = 0, mode = 0; ARG_TO_VALUE(in[0], unit); ARG_TO_VALUE(in[1], mode);
    object->removeTextureMode(unit, mode); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, getUniform)
if (in.size() > 0)  // osg::StateSet::getUniform(const std::string& name)
{
    std::string name; ARG_TO_VALUE(in[0], name);
    osg::Uniform* uniform = object->getUniform(name);
    if (uniform) out.push_back(uniform); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, getOrCreateUniform)
if (in.size() > 2)  // osg::StateSet::getOrCreateUniform(const std::string& name, Uniform::Type type, unsigned int num)
{
    std::string name; ARG_TO_VALUE(in[0], name);
    unsigned int type = 0, num = 0; ARG_TO_VALUE(in[1], type); ARG_TO_VALUE(in[2], num);
    osg::Uniform* uniform = object->getOrCreateUniform(name, (osg::Uniform::Type)type, num);
    if (uniform) out.push_back(uniform); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, addUniform)
if (in.size() > 1)  // osg::StateSet::addUniform(osg::Uniform* uniform, unsigned int flags)
{
    osg::Uniform* uniform = dynamic_cast<osg::Uniform*>(in[0].get());
    unsigned int flags = 0; ARG_TO_VALUE(in[1], flags);
    object->addUniform(uniform, flags); return true;
}
METHOD_END()

METHOD_BEGIN(osg, StateSet, removeUniform)
if (in.size() > 0)  // osg::StateSet::removeUniform(const std::string& name)
{
    std::string name; ARG_TO_VALUE(in[0], name);
    object->removeUniform(name); return true;
}
METHOD_END()

static void addStateSetMethods(osgDB::ObjectWrapper* wrapper)
{
    REGISTER_METHOD(wrapper, osg, StateSet, getAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, getTextureAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, getMode);
    REGISTER_METHOD(wrapper, osg, StateSet, getTextureMode);
    REGISTER_METHOD(wrapper, osg, StateSet, setAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, setTextureAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, setMode);
    REGISTER_METHOD(wrapper, osg, StateSet, setTextureMode);
    REGISTER_METHOD(wrapper, osg, StateSet, removeAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, removeTextureAttribute);
    REGISTER_METHOD(wrapper, osg, StateSet, removeMode);
    REGISTER_METHOD(wrapper, osg, StateSet, removeTextureMode);
    REGISTER_METHOD(wrapper, osg, StateSet, getUniform);
    REGISTER_METHOD(wrapper, osg, StateSet, getOrCreateUniform);
    REGISTER_METHOD(wrapper, osg, StateSet, addUniform);
    REGISTER_METHOD(wrapper, osg, StateSet, removeUniform);

    METHOD_INFO_IN1_OUT1(wrapper, getAttribute, ARG_INFO("type", RW_UINT), ARG_INFO("attribute", RW_OBJECT));
    METHOD_INFO_IN2_OUT1(wrapper, getTextureAttribute, ARG_INFO("unit", RW_UINT), ARG_INFO("type", RW_UINT),
                                                       ARG_INFO("attribute", RW_OBJECT));
    METHOD_INFO_IN1_OUT1(wrapper, getMode, ARG_INFO("mode", RW_UINT), ARG_INFO("value", RW_UINT));
    METHOD_INFO_IN2_OUT1(wrapper, getTextureMode, ARG_INFO("unit", RW_UINT), ARG_INFO("mode", RW_UINT),
                                                  ARG_INFO("value", RW_UINT));
    METHOD_INFO_IN2(wrapper, setAttribute, ARG_INFO("attribute", RW_OBJECT), ARG_INFO("flags", RW_UINT));
    METHOD_INFO_IN3(wrapper, setTextureAttribute, ARG_INFO("unit", RW_UINT), ARG_INFO("attribute", RW_OBJECT),
                                                  ARG_INFO("flags", RW_UINT));
    METHOD_INFO_IN2(wrapper, setMode, ARG_INFO("mode", RW_UINT), ARG_INFO("flags", RW_UINT));
    METHOD_INFO_IN3(wrapper, setTextureMode, ARG_INFO("unit", RW_UINT), ARG_INFO("mode", RW_UINT),
                                             ARG_INFO("flags", RW_UINT));
    METHOD_INFO_IN1(wrapper, removeAttribute, ARG_INFO("attribute", RW_OBJECT));
    METHOD_INFO_IN2(wrapper, removeTextureAttribute, ARG_INFO("unit", RW_UINT), ARG_INFO("attribute", RW_OBJECT));
    METHOD_INFO_IN1(wrapper, removeMode, ARG_INFO("mode", RW_UINT));
    METHOD_INFO_IN2(wrapper, removeTextureMode, ARG_INFO("unit", RW_UINT), ARG_INFO("mode", RW_UINT));
    METHOD_INFO_IN1_OUT1(wrapper, getUniform, ARG_INFO("name", RW_STRING), ARG_INFO("uniform", RW_OBJECT));
    METHOD_INFO_IN3_OUT1(wrapper, getOrCreateUniform, ARG_INFO("name", RW_STRING), ARG_INFO("type", RW_UINT),
                                                      ARG_INFO("numElements", RW_UINT), ARG_INFO("uniform", RW_OBJECT));
    METHOD_INFO_IN2(wrapper, addUniform, ARG_INFO("uniform", RW_OBJECT), ARG_INFO("flags", RW_UINT));
    METHOD_INFO_IN1(wrapper, removeUniform, ARG_INFO("name", RW_STRING));
}

/////////////////////////////// UNIFORM

void addUniformMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: set/get, setElement/getElement
}

/////////////////////////////// SHADER
void addShaderMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeShaderSource
}

/////////////////////////////// PROGRAM
void addProgramMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/setGeomProperty, get/setComputeGroups
}

/////////////////////////////// PROXYNODE
void addProxyNodeMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeFileName, get/setUserCenter
}

/////////////////////////////// LOD
void addLODMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeRange, get/setUserCenter
}

/////////////////////////////// PAGEDLOD
void addPagedLODMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeRangeData, get/setDatabasePath
}

/////////////////////////////// CAMERA
void addCameraMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: attach/detech/getAttachment, get/setRenderOrder
}

///////////////////////////////
namespace osgVerse
{
    OSGVERSE_WRAPPERS_EXPORT bool updateOsgBinaryWrappers(const std::string& libName)
    {
        osgDB::Registry* registry = osgDB::Registry::instance();
        std::string pluginLib = registry->createLibraryNameForExtension("serializers_" + libName);
        if (registry->loadLibrary(pluginLib) == osgDB::Registry::NOT_LOADED) return false;

        osgDB::ObjectWrapperManager::WrapperMap& wrappers = registry->getObjectWrapperManager()->getWrapperMap();
        for (osgDB::ObjectWrapperManager::WrapperMap::iterator itr = wrappers.begin();
            itr != wrappers.end(); ++itr)
        {
            const std::string& clsName = itr->first; osgDB::ObjectWrapper* w = itr->second.get();
            if (clsName == "osg::Node") METHOD_INFO_OUT1(w, getOrCreateStateSet, ARG_INFO("return", RW_OBJECT))
            else if (clsName == "osg::ProxyNode") addProxyNodeMethods(w);
            else if (clsName == "osg::LOD") addLODMethods(w);
            else if (clsName == "osg::PagedLOD") addPagedLODMethods(w);
            else if (clsName == "osg::Camera") addCameraMethods(w);
            else if (clsName == "osg::StateSet") addStateSetMethods(w);
            else if (clsName == "osg::Uniform") addUniformMethods(w);
            else if (clsName == "osg::Shader") addShaderMethods(w);
            else if (clsName == "osg::Program")
            {
                METHOD_INFO_OUT1(w, getNumShaders, ARG_INFO("return", RW_UINT));
                METHOD_INFO_IN1_OUT1(w, getShader, ARG_INFO("index", RW_UINT), ARG_INFO("shader", RW_OBJECT));
                METHOD_INFO_IN1(w, addShader, ARG_INFO("shader", RW_OBJECT));
                METHOD_INFO_IN1(w, removeShader, ARG_INFO("shader", RW_OBJECT));
                METHOD_INFO_IN2(w, addBindAttribLocation, ARG_INFO("name", RW_STRING), ARG_INFO("index", RW_UINT));
                METHOD_INFO_IN1(w, removeBindAttribLocation, ARG_INFO("name", RW_STRING));
                addProgramMethods(w);
            }
            else if (clsName == "osg::Geode")
            {
                METHOD_INFO_OUT1(w, getNumDrawables, ARG_INFO("return", RW_UINT));
                METHOD_INFO_IN1_OUT1(w, getDrawable, ARG_INFO("index", RW_UINT), ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN2(w, setDrawable, ARG_INFO("index", RW_UINT), ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN1(w, addDrawable, ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN1(w, removeDrawable, ARG_INFO("child", RW_OBJECT));
            }
            else if (clsName == "osg::Group")
            {
                METHOD_INFO_OUT1(w, getNumChildren, ARG_INFO("return", RW_UINT));
                METHOD_INFO_IN1_OUT1(w, getChild, ARG_INFO("index", RW_UINT), ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN2(w, setChild, ARG_INFO("index", RW_UINT), ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN1(w, addChild, ARG_INFO("child", RW_OBJECT));
                METHOD_INFO_IN1(w, removeChild, ARG_INFO("child", RW_OBJECT));
            }
            else if (clsName == "osg::Switch")
            {
                METHOD_INFO_IN1_OUT1(w, getValue, ARG_INFO("index", RW_UINT), ARG_INFO("enabled", RW_BOOL));
                METHOD_INFO_IN2(w, setValue, ARG_INFO("index", RW_UINT), ARG_INFO("enabled", RW_BOOL));
            }
            else if (clsName == "osg::TextureCubeMap")
            {
                METHOD_INFO_IN1_OUT1(w, getImage, ARG_INFO("face", RW_UINT), ARG_INFO("image", RW_IMAGE));
                METHOD_INFO_IN2(w, setImage, ARG_INFO("face", RW_UINT), ARG_INFO("image", RW_IMAGE));
            }
        }
        return fixOsgBinaryWrappers(libName);
    }
}
