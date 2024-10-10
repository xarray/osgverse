#include <osg/Geometry>
#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include "Utilities.h"

/////////////////////////////// STATESET
struct StateSet_GetAttribute : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // getAttribute(type) / getTextureAttribute(unit, type)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        unsigned int unit = 0, type = osg::StateAttribute::Type::TEXTURE;
        if (in.size() > 1)
        {
            osg::ValueObject* unitObj = in[0]->asValueObject();
            osg::ValueObject* typeObj = in[1]->asValueObject();
            if (unitObj) unitObj->getScalarValue(unit);
            if (typeObj) typeObj->getScalarValue(type);
        }
        else
        {
            osg::ValueObject* typeObj = in[0]->asValueObject();
            if (typeObj) typeObj->getScalarValue(type);
        }

        osg::StateAttribute* sa = NULL;
        if (_withTextureUnit) sa = ss->getTextureAttribute(unit, (osg::StateAttribute::Type)type);
        else sa = ss->getAttribute((osg::StateAttribute::Type)type);
        out.push_back(sa); return true;
    }

    StateSet_GetAttribute(bool withTex) : _withTextureUnit(withTex) {}
    bool _withTextureUnit;
};

struct StateSet_GetMode : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // getMode(mode) / getTextureMode(unit, mode)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        unsigned int unit = 0, mode = GL_NONE;
        if (in.size() > 1)
        {
            osg::ValueObject* unitObj = in[0]->asValueObject();
            osg::ValueObject* modeObj = in[1]->asValueObject();
            if (unitObj) unitObj->getScalarValue(unit);
            if (modeObj) modeObj->getScalarValue(mode);
        }
        else
        {
            osg::ValueObject* modeObj = in[0]->asValueObject();
            if (modeObj) modeObj->getScalarValue(mode);
        }

        if (_withTextureUnit)
            out.push_back(new osg::UIntValueObject("return", ss->getTextureMode(unit, (GLenum)mode)));
        else
            out.push_back(new osg::UIntValueObject("return", ss->getMode((GLenum)mode))); return true;
    }

    StateSet_GetMode(bool withTex) : _withTextureUnit(withTex) {}
    bool _withTextureUnit;
};

struct StateSet_SetAttribute : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // setAttribute(obj, [flags])
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;
        osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[0].get());
        if (!sa) return false;

        unsigned int flags = osg::StateAttribute::ON;
        if (in.size() > 1)
        {
            osg::ValueObject* flagsObj = in[1]->asValueObject();
            if (flagsObj) flagsObj->getScalarValue(flags);
        }
        if (_withModes) ss->setAttributeAndModes(sa, flags);
        else ss->setAttribute(sa, flags); return true;
    }

    StateSet_SetAttribute(bool withModes) : _withModes(withModes) {}
    bool _withModes;
};

struct StateSet_SetTextureAttribute : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // setTextureAttribute(unit, obj, [flags])
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 2) return false;

        unsigned int unit = 0, flags = osg::StateAttribute::ON;
        osg::ValueObject* unitObj = in[0]->asValueObject();
        osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[1].get());
        if (unitObj) unitObj->getScalarValue(unit); if (!sa) return false;

        if (in.size() > 2)
        {
            osg::ValueObject* flagsObj = in[2]->asValueObject();
            if (flagsObj) flagsObj->getScalarValue(flags);
        }
        if (_withModes) ss->setTextureAttributeAndModes(unit, sa, flags);
        else ss->setTextureAttribute(unit, sa, flags); return true;
    }

    StateSet_SetTextureAttribute(bool withModes) : _withModes(withModes) {}
    bool _withModes;
};

struct StateSet_RemoveAttribute : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // removeAttribute(string/type) / removeTextureAttribute(unit, string/type)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        unsigned int unit = 0, tID = 0, type = osg::StateAttribute::Type::TEXTURE;
        if (in.size() > 1)
        {
            osg::ValueObject* unitObj = in[0]->asValueObject();
            if (unitObj) unitObj->getScalarValue(unit); tID = 1;
        }

        osg::StateAttribute* sa = dynamic_cast<osg::StateAttribute*>(in[tID].get());
        osg::ValueObject* tObj = in[tID]->asValueObject(); if (tObj) tObj->getScalarValue(type);
        if (_withTextureUnit)
        {
            if (sa) ss->removeTextureAttribute(unit, sa);
            else ss->removeTextureAttribute(unit, (osg::StateAttribute::Type)type);
        }
        else
        {
            if (sa) ss->removeAttribute(sa);
            else ss->removeAttribute((osg::StateAttribute::Type)type);
        }
        return true;
    }

    StateSet_RemoveAttribute(bool withTex) : _withTextureUnit(withTex) {}
    bool _withTextureUnit;
};

struct StateSet_RemoveMode : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // removeMode(mode) / removeTextureMode(unit, mode)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        unsigned int unit = 0, tID = 0, type = GL_NONE;
        if (in.size() > 1)
        {
            osg::ValueObject* unitObj = in[0]->asValueObject();
            if (unitObj) unitObj->getScalarValue(unit); tID = 1;
        }

        osg::ValueObject* tObj = in[tID]->asValueObject(); if (tObj) tObj->getScalarValue(type);
        if (_withTextureUnit) ss->removeTextureMode(unit, (GLenum)type);
        else ss->removeMode((GLenum)type); return true;
    }

    StateSet_RemoveMode(bool withTex) : _withTextureUnit(withTex) {}
    bool _withTextureUnit;
};

struct StateSet_GetUniform : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // getUniform(string)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        osg::StringValueObject* nameObj = dynamic_cast<osg::StringValueObject*>(in[0].get());
        osg::Uniform* uniform = nameObj ? ss->getUniform(nameObj->getValue()) : NULL;
        out.push_back(uniform); return true;
    }
};

struct StateSet_AddUniform : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // addUniform(obj, [value])
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        unsigned int flags = osg::StateAttribute::ON;
        if (in.size() > 1)
        {
            osg::ValueObject* flagsObj = in[1]->asValueObject();
            if (flagsObj) flagsObj->getScalarValue(flags);
        }
        osg::Uniform* uniform = dynamic_cast<osg::Uniform*>(in[0].get());
        ss->addUniform(uniform, flags); return true;
    }
};

struct StateSet_RemoveUniform : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // removeUniform(obj/string)
        osg::StateSet* ss = reinterpret_cast<osg::StateSet*>(objectPtr);
        if (in.size() < 1) return false;

        osg::StringValueObject* nameObj = dynamic_cast<osg::StringValueObject*>(in[0].get());
        osg::Uniform* uniform = nameObj ? NULL : dynamic_cast<osg::Uniform*>(in[0].get());
        if (nameObj) ss->removeUniform(nameObj->getValue());
        else if (uniform) ss->removeUniform(uniform); return true;
    }
};

static void addStateSetMethods(osgDB::ObjectWrapper* wrapper)
{
    wrapper->addMethodObject("getAttribute", new StateSet_GetAttribute(false));
    wrapper->addMethodObject("getTextureAttribute", new StateSet_GetAttribute(true));
    wrapper->addMethodObject("getMode", new StateSet_GetMode(false));
    wrapper->addMethodObject("getTextureMode", new StateSet_GetMode(true));
    wrapper->addMethodObject("setAttribute", new StateSet_SetAttribute(false));
    wrapper->addMethodObject("setAttributeAndModes", new StateSet_SetAttribute(true));
    wrapper->addMethodObject("setTextureAttribute", new StateSet_SetTextureAttribute(false));
    wrapper->addMethodObject("setTextureAttributeAndModes", new StateSet_SetTextureAttribute(true));
    wrapper->addMethodObject("removeAttribute", new StateSet_RemoveAttribute(false));
    wrapper->addMethodObject("removeTextureAttribute", new StateSet_RemoveAttribute(true));
    wrapper->addMethodObject("removeMode", new StateSet_RemoveMode(false));
    wrapper->addMethodObject("removeTextureMode", new StateSet_RemoveMode(true));
    wrapper->addMethodObject("getUniform", new StateSet_GetUniform);
    wrapper->addMethodObject("addUniform", new StateSet_AddUniform);
    wrapper->addMethodObject("removeUniform", new StateSet_RemoveUniform);
}

/////////////////////////////// UNIFORM
struct Uniform_GetElement : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // getElement(index)
        osg::Uniform* uniform = reinterpret_cast<osg::Uniform*>(objectPtr);
        if (in.size() < 1) return false;
        unsigned int index = 0; osg::ValueObject* idObj = in[0]->asValueObject();
        if (idObj) idObj->getScalarValue(index);

        unsigned int num = uniform->getTypeNumComponents(uniform->getType());
        unsigned int j = index * num;
        for (unsigned int i = 0; i < num; ++i)
        {
            std::string name = std::to_string(i);
            if (uniform->getFloatArray() != NULL)
                out.push_back(new osg::FloatValueObject(name, (*uniform->getFloatArray())[j + i]));
            else if (uniform->getDoubleArray() != NULL)
                out.push_back(new osg::DoubleValueObject(name, (*uniform->getDoubleArray())[j + i]));
            else if (uniform->getIntArray() != NULL)
                out.push_back(new osg::IntValueObject(name, (*uniform->getIntArray())[j + i]));
            else if (uniform->getUIntArray() != NULL)
                out.push_back(new osg::UIntValueObject(name, (*uniform->getUIntArray())[j + i]));
        }
        return true;
    }
};

struct Uniform_SetElement : public osgDB::MethodObject
{
    virtual bool run(void* objectPtr, osg::Parameters& in, osg::Parameters& out) const
    {
        // setElement(index, v0, v1, ...)
        osg::Uniform* uniform = reinterpret_cast<osg::Uniform*>(objectPtr);
        unsigned int num = uniform->getTypeNumComponents(uniform->getType());
        if (in.size() < (1 + num)) return false;

        unsigned int index = 0; osg::ValueObject* idObj = in[0]->asValueObject();
        if (idObj) idObj->getScalarValue(index);

        unsigned int j = index * num;
        for (unsigned int i = 0; i < num; ++i)
        {
            osg::ValueObject* vObj = in[1 + i]->asValueObject(); if (!vObj) continue;
            if (uniform->getFloatArray() != NULL)
                vObj->getScalarValue((*uniform->getFloatArray())[j + i]);
            else if (uniform->getDoubleArray() != NULL)
                vObj->getScalarValue((*uniform->getDoubleArray())[j + i]);
            else if (uniform->getIntArray() != NULL)
                vObj->getScalarValue((*uniform->getIntArray())[j + i]);
            else if (uniform->getUIntArray() != NULL)
                vObj->getScalarValue((*uniform->getUIntArray())[j + i]);
        }
        return true;
    }
};

void addUniformMethods(osgDB::ObjectWrapper* wrapper)
{
    wrapper->addMethodObject("getElement", new Uniform_GetElement);
    wrapper->addMethodObject("setElement", new Uniform_SetElement);
}

/////////////////////////////// SHADER
void addShaderMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeShaderSource
}

/////////////////////////////// PROGRAM
void addProgramMethods(osgDB::ObjectWrapper* wrapper)
{
    // TODO: get/add/removeShader, set/getAttrBinding, set/getFragBinding,
    //       get/setGeomProperty, get/setComputeGroups
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
    bool updateOsgBinaryWrappers(const std::string& libName)
    {
        osgDB::Registry* registry = osgDB::Registry::instance();
        std::string pluginLib = registry->createLibraryNameForExtension("serializers_" + libName);
        if (registry->loadLibrary(pluginLib) == osgDB::Registry::NOT_LOADED) return false;

        osgDB::ObjectWrapperManager::WrapperMap& wrappers = registry->getObjectWrapperManager()->getWrapperMap();
        for (osgDB::ObjectWrapperManager::WrapperMap::iterator itr = wrappers.begin();
            itr != wrappers.end(); ++itr)
        {
            if (itr->first == "osg::StateSet") addStateSetMethods(itr->second.get());
            else if (itr->first == "osg::Uniform") addUniformMethods(itr->second.get());
            else if (itr->first == "osg::Program") addProgramMethods(itr->second.get());
            else if (itr->first == "osg::Shader") addShaderMethods(itr->second.get());
            else if (itr->first == "osg::ProxyNode") addProxyNodeMethods(itr->second.get());
            else if (itr->first == "osg::LOD") addLODMethods(itr->second.get());
            else if (itr->first == "osg::PagedLOD") addPagedLODMethods(itr->second.get());
            else if (itr->first == "osg::Camera") addCameraMethods(itr->second.get());
        }
        return fixOsgBinaryWrappers(libName);
    }
}
