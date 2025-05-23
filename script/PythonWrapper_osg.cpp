#include <osg/Version>
#include <osg/Quat>
#include <osg/Matrix>
#include <iostream>

#include "PythonScript.h"
#ifdef WITH_PYTHON
#   include <Python.h>
#   include "pybind11/pybind11.h"
#   include "pybind11/embed.h"
#endif

using namespace osgVerse;
void pythonWrapperOsg() {}

/*struct OsgBase
{
    osg::observer_ptr<LibraryEntry> entry;
    osg::ref_ptr<osg::Object> object;
};*/

#ifdef WITH_PYTHON

#define PYBIND_VECTOR(type) \
    .def("length", & type##::length) \
    .def("__getitem__", [](const type & s, size_t i) { return s[i]; }) \
    .def("__setitem__", [](type & s, size_t i, float v) { s[i] = v; })

#define PYBIND_MATRIX(type, vec_type, value_type) \
    .def("valid", & type##::valid).def("makeIdentity", & type##::makeIdentity) \
    .def("makeTranslate", pybind11::overload_cast<const vec_type &>(& type##::makeTranslate), pybind11::arg("t")) \
    .def("makeRotate", pybind11::overload_cast<const osg::Quat&>(& type##::makeRotate), pybind11::arg("r")) \
    .def("makeScale", pybind11::overload_cast<const vec_type &>(& type##::makeScale), pybind11::arg("s")) \
    .def("preMultiply", pybind11::overload_cast<const type &>(& type##::preMult), pybind11::arg("m")) \
    .def("postMultiply", pybind11::overload_cast<const type &>(& type##::postMult), pybind11::arg("m")) \
    .def("__getitem__", [](const type & self, pybind11::tuple id) \
        { if (id.size() >= 2) return self(id[0].cast<int>(), id[1].cast<int>()); else return (value_type)0.0; }) \
    .def("__setitem__", [](type & self, pybind11::tuple id, value_type v) \
        { if (id.size() >= 2) self(id[0].cast<int>(), id[1].cast<int>()) = v; })

#define PYBIND_OSG_OBJECT(classObj, type) \
    classObj.def(("set" + prop.name).c_str(), [&](OsgBase<N>* s, const type & v) \
        { s->entry->setProperty(s->object.get(), prop.name, v); }) \
            .def(("get" + prop.name).c_str(), [&](OsgBase<N>* s) \
        { type v; s->entry->getProperty(s->object.get(), prop.name, v); return v; })

template<int N>
static void createPythonClass(pybind11::module_& m, LibraryEntry* entry, const std::string& name)
{
    pybind11::class_<OsgBase<N>> classObj(m, name.c_str());
    classObj.def(pybind11::init([&]()
    {
        OsgBase<N>* dummy = new OsgBase<N>();
        dummy->object = entry->create(name);
        dummy->entry = entry; return dummy;
    }));

    std::vector<LibraryEntry::Property> propNames = entry->getPropertyNames(name);
    for (size_t i = 0; i < propNames.size(); ++i)
    {
        LibraryEntry::Property& prop = propNames[i];
        switch (prop.type)
        {
        case osgDB::BaseSerializer::RW_BOOL: PYBIND_OSG_OBJECT(classObj, bool); break;
        case osgDB::BaseSerializer::RW_SHORT: PYBIND_OSG_OBJECT(classObj, short); break;
        case osgDB::BaseSerializer::RW_USHORT: PYBIND_OSG_OBJECT(classObj, unsigned short); break;
        case osgDB::BaseSerializer::RW_INT: PYBIND_OSG_OBJECT(classObj, int); break;
        case osgDB::BaseSerializer::RW_UINT: PYBIND_OSG_OBJECT(classObj, unsigned int); break;
        case osgDB::BaseSerializer::RW_GLENUM: PYBIND_OSG_OBJECT(classObj, unsigned int); break;
        case osgDB::BaseSerializer::RW_FLOAT: PYBIND_OSG_OBJECT(classObj, float); break;
        case osgDB::BaseSerializer::RW_DOUBLE: PYBIND_OSG_OBJECT(classObj, double); break;
        case osgDB::BaseSerializer::RW_VEC2F: PYBIND_OSG_OBJECT(classObj, osg::Vec2f); break;
        case osgDB::BaseSerializer::RW_VEC3F: PYBIND_OSG_OBJECT(classObj, osg::Vec3f); break;
        case osgDB::BaseSerializer::RW_VEC4F: PYBIND_OSG_OBJECT(classObj, osg::Vec4f); break;
        case osgDB::BaseSerializer::RW_VEC2D: PYBIND_OSG_OBJECT(classObj, osg::Vec2d); break;
        case osgDB::BaseSerializer::RW_VEC3D: PYBIND_OSG_OBJECT(classObj, osg::Vec3d); break;
        case osgDB::BaseSerializer::RW_VEC4D: PYBIND_OSG_OBJECT(classObj, osg::Vec4d); break;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        case osgDB::BaseSerializer::RW_VEC2B: PYBIND_OSG_OBJECT(classObj, osg::Vec2b); break;
        case osgDB::BaseSerializer::RW_VEC3B: PYBIND_OSG_OBJECT(classObj, osg::Vec3b); break;
        case osgDB::BaseSerializer::RW_VEC4B: PYBIND_OSG_OBJECT(classObj, osg::Vec4b); break;
        case osgDB::BaseSerializer::RW_VEC2UB: PYBIND_OSG_OBJECT(classObj, osg::Vec2ub); break;
        case osgDB::BaseSerializer::RW_VEC3UB: PYBIND_OSG_OBJECT(classObj, osg::Vec3ub); break;
        case osgDB::BaseSerializer::RW_VEC4UB: PYBIND_OSG_OBJECT(classObj, osg::Vec4ub); break;
        case osgDB::BaseSerializer::RW_VEC2S: PYBIND_OSG_OBJECT(classObj, osg::Vec2s); break;
        case osgDB::BaseSerializer::RW_VEC3S: PYBIND_OSG_OBJECT(classObj, osg::Vec3s); break;
        case osgDB::BaseSerializer::RW_VEC4S: PYBIND_OSG_OBJECT(classObj, osg::Vec4s); break;
        case osgDB::BaseSerializer::RW_VEC2US: PYBIND_OSG_OBJECT(classObj, osg::Vec2us); break;
        case osgDB::BaseSerializer::RW_VEC3US: PYBIND_OSG_OBJECT(classObj, osg::Vec3us); break;
        case osgDB::BaseSerializer::RW_VEC4US: PYBIND_OSG_OBJECT(classObj, osg::Vec4us); break;
        case osgDB::BaseSerializer::RW_VEC2I: PYBIND_OSG_OBJECT(classObj, osg::Vec2i); break;
        case osgDB::BaseSerializer::RW_VEC3I: PYBIND_OSG_OBJECT(classObj, osg::Vec3i); break;
        case osgDB::BaseSerializer::RW_VEC4I: PYBIND_OSG_OBJECT(classObj, osg::Vec4i); break;
        case osgDB::BaseSerializer::RW_VEC2UI: PYBIND_OSG_OBJECT(classObj, osg::Vec2ui); break;
        case osgDB::BaseSerializer::RW_VEC3UI: PYBIND_OSG_OBJECT(classObj, osg::Vec3ui); break;
        case osgDB::BaseSerializer::RW_VEC4UI: PYBIND_OSG_OBJECT(classObj, osg::Vec4ui); break;
#endif
        case osgDB::BaseSerializer::RW_QUAT: PYBIND_OSG_OBJECT(classObj, osg::Quat); break;
        case osgDB::BaseSerializer::RW_MATRIXF: PYBIND_OSG_OBJECT(classObj, osg::Matrixf); break;
        case osgDB::BaseSerializer::RW_MATRIXD: PYBIND_OSG_OBJECT(classObj, osg::Matrixd); break;
        case osgDB::BaseSerializer::RW_MATRIX: PYBIND_OSG_OBJECT(classObj, osg::Matrixd); break;
        case osgDB::BaseSerializer::RW_STRING: PYBIND_OSG_OBJECT(classObj, std::string); break;
        case osgDB::BaseSerializer::RW_ENUM:
            classObj.def(("set" + prop.name).c_str(), [&](OsgBase<N>* s, const std::string& v)
            { s->entry->setEnumProperty(s->object.get(), prop.name, v); });
            classObj.def(("get" + prop.name).c_str(), [&](OsgBase<N>* s)
            { return s->entry->getEnumProperty(s->object.get(), prop.name); });
            break;
        case osgDB::BaseSerializer::RW_OBJECT:
            // TODO
            break;
        case osgDB::BaseSerializer::RW_IMAGE:
            // TODO
            break;
        case osgDB::BaseSerializer::RW_VECTOR:
            // TODO
            break;
        case osgDB::BaseSerializer::RW_USER:
            // TODO
            break;
        default:
            OSG_NOTICE << "[PythonScript] Python binding of class " << name << "'s property "
                       << prop.name << " is not implemented" << std::endl;
            break;
        }
    }
}

PYBIND11_EMBEDDED_MODULE(osg, module)
{
    pybind11::class_<osg::Vec2f>(module, "Vec2f")
        .def(pybind11::init<float, float>()) PYBIND_VECTOR(osg::Vec2f);
    pybind11::class_<osg::Vec2d>(module, "Vec2d")
        .def(pybind11::init<float, float>()) PYBIND_VECTOR(osg::Vec2d);
    pybind11::class_<osg::Vec3f>(module, "Vec3f")
        .def(pybind11::init<float, float, float>()) PYBIND_VECTOR(osg::Vec3f);
    pybind11::class_<osg::Vec3d>(module, "Vec3d")
        .def(pybind11::init<float, float, float>()) PYBIND_VECTOR(osg::Vec3d);
    pybind11::class_<osg::Vec4f>(module, "Vec4f")
        .def(pybind11::init<float, float, float, float>()) PYBIND_VECTOR(osg::Vec4f);
    pybind11::class_<osg::Vec4d>(module, "Vec4d")
        .def(pybind11::init<float, float, float, float>()) PYBIND_VECTOR(osg::Vec4d);
    pybind11::class_<osg::Quat>(module, "Quat")
        .def(pybind11::init<float, float, float, float>()) PYBIND_VECTOR(osg::Quat);
    pybind11::class_<osg::Matrixf>(module, "Matrixf")
        .def(pybind11::init()) PYBIND_MATRIX(osg::Matrixf, osg::Vec3f, float);
    pybind11::class_<osg::Matrixd>(module, "Matrixd")
        .def(pybind11::init()) PYBIND_MATRIX(osg::Matrixd, osg::Vec3d, double);

    osg::ref_ptr<LibraryEntry> entry = new LibraryEntry("osg");
    const std::set<std::string>& classes = entry->getClasses();

    std::vector<std::string> classList(classes.begin(), classes.end());
    ////
}
#endif
