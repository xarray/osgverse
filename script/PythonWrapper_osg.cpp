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

struct PythonWrapperObject
{
    PyObject_HEAD;
    osg::ref_ptr<LibraryEntry> entry;
    osg::ref_ptr<osg::Object> instance;
    std::string className;
    
    struct Property { std::string name, base; int type; Property():type(0) {} };

    static PyObject* allocate(PyTypeObject* type, PyObject* args, PyObject* kwds)
    {
        PyObject* cap = PyObject_GetAttrString((PyObject*)type, "_user");
        if (!cap) { PyErr_SetString(PyExc_TypeError, "Bad user capsule"); return NULL; }

        const char* libName = (const char*)PyCapsule_GetPointer(cap, "library");
        const char* className = (const char*)PyType_GetSlot(type, Py_tp_doc);
        osg::ref_ptr<LibraryEntry> entry = new LibraryEntry(libName);
        if (!entry || !className) { PyErr_SetString(PyExc_TypeError, "Bad entry pointer"); return NULL; }

        PythonWrapperObject* self = (PythonWrapperObject*)type->tp_alloc(type, 0);
        self->entry = entry; self->instance = entry->create(className);
        self->className = className; Py_DECREF(cap); return (PyObject*)self;
    }

    static void deallocate(PyObject* self0)
    {
        PythonWrapperObject* self = (PythonWrapperObject*)self0;
        self->instance = NULL; Py_TYPE(self)->tp_free(self);
    }

    static PyObject* getProperty(PyObject* self0, void* closure)
    {
        PythonWrapperObject* self = (PythonWrapperObject*)self0;
        PythonWrapperObject::Property* prop = (PythonWrapperObject::Property*)closure;
        if (!prop || !self->entry || !self->instance)
            { PyErr_SetString(PyExc_TypeError, "Empty wrapper data"); return NULL; }

#define DO_GET(T, v) T v; if (!self->entry->getProperty(self->instance.get(), prop->name, v)) break;
        switch (prop->type)
        {
        case osgDB::BaseSerializer::RW_BOOL: { DO_GET(bool, v); return PyBool_FromLong(v ? 1 : 0); } break;
        case osgDB::BaseSerializer::RW_SHORT: { DO_GET(short, v); return PyLong_FromLong((size_t)v); } break;
        case osgDB::BaseSerializer::RW_USHORT: { DO_GET(uint16_t, v); return PyLong_FromLong((size_t)v); } break;
        case osgDB::BaseSerializer::RW_INT: { DO_GET(int, v); return PyLong_FromLong((size_t)v); } break;
        case osgDB::BaseSerializer::RW_UINT: { DO_GET(uint32_t, v); return PyLong_FromLong((size_t)v); } break;
        case osgDB::BaseSerializer::RW_FLOAT: { DO_GET(float, v); return PyFloat_FromDouble((double)v); } break;
        case osgDB::BaseSerializer::RW_DOUBLE: { DO_GET(double, v); return PyFloat_FromDouble((double)v); } break;
        case osgDB::BaseSerializer::RW_VEC2F: { DO_GET(osg::Vec2f, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3F: { DO_GET(osg::Vec3f, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4F: { DO_GET(osg::Vec4f, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2D: { DO_GET(osg::Vec2d, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3D: { DO_GET(osg::Vec3d, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4D: { DO_GET(osg::Vec4d, v); return pybind11::cast(v).release().ptr(); } break;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        case osgDB::BaseSerializer::RW_VEC2B: { DO_GET(osg::Vec2b, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3B: { DO_GET(osg::Vec3b, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4B: { DO_GET(osg::Vec4b, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2UB: { DO_GET(osg::Vec2ub, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3UB: { DO_GET(osg::Vec3ub, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4UB: { DO_GET(osg::Vec4ub, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2S: { DO_GET(osg::Vec2s, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3S: { DO_GET(osg::Vec3s, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4S: { DO_GET(osg::Vec4s, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2US: { DO_GET(osg::Vec2us, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3US: { DO_GET(osg::Vec3us, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4US: { DO_GET(osg::Vec4us, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2I: { DO_GET(osg::Vec2i, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3I: { DO_GET(osg::Vec3i, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4I: { DO_GET(osg::Vec4i, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC2UI: { DO_GET(osg::Vec2ui, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC3UI: { DO_GET(osg::Vec3ui, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_VEC4UI: { DO_GET(osg::Vec4ui, v); return pybind11::cast(v).release().ptr(); } break;
#endif
        case osgDB::BaseSerializer::RW_QUAT: { DO_GET(osg::Quat, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_MATRIXF: { DO_GET(osg::Matrixf, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_MATRIXD: { DO_GET(osg::Matrixd, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_MATRIX: { DO_GET(osg::Matrix, v); return pybind11::cast(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_STRING: { DO_GET(std::string, v); return pybind11::str(v).release().ptr(); } break;
        case osgDB::BaseSerializer::RW_ENUM:
            // TODO
            break;
        case osgDB::BaseSerializer::RW_GLENUM:
            // TODO
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
        default: break;
        }
        PyErr_SetString(PyExc_ValueError, "Bad value type"); return NULL;
    }

    static int setProperty(PyObject* self0, PyObject* value, void* closure)
    {
        PythonWrapperObject* self = (PythonWrapperObject*)self0; pybind11::handle H(value);
        PythonWrapperObject::Property* prop = (PythonWrapperObject::Property*)closure;
        if (!prop || !value || value == Py_None)
            { PyErr_SetString(PyExc_TypeError, "Empty setter data"); return -1; }
        else if (!self->entry || !self->instance)
            { PyErr_SetString(PyExc_TypeError, "Empty wrapper data"); return -1; }

#define DO_SET(v) if (self->entry->setProperty(self->instance.get(), prop->name, (v))) return 0;
#define DO_CHECK(T, v) if (!pybind11::isinstance<T>(H)) break; T v = pybind11::cast<T>(H);
        switch (prop->type)
        {
        case osgDB::BaseSerializer::RW_BOOL:
            if (PyBool_Check(value)) { bool v = (value == Py_True); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_SHORT:
            if (PyLong_Check(value)) { short v = (short)PyLong_AsLong(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_USHORT:
            if (PyLong_Check(value)) { uint16_t v = (uint16_t)PyLong_AsLong(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_INT:
            if (PyLong_Check(value)) { int v = (int)PyLong_AsLong(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_UINT:
            if (PyLong_Check(value)) { uint32_t v = (uint32_t)PyLong_AsLong(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_FLOAT:
            if (PyFloat_Check(value)) { float v = (float)PyFloat_AsDouble(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_DOUBLE:
            if (PyFloat_Check(value)) { double v = (double)PyFloat_AsDouble(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2F: { DO_CHECK(osg::Vec2f, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3F: { DO_CHECK(osg::Vec3f, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4F: { DO_CHECK(osg::Vec4f, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2D: { DO_CHECK(osg::Vec2d, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3D: { DO_CHECK(osg::Vec3d, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4D: { DO_CHECK(osg::Vec4d, v); DO_SET(v); } break;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        case osgDB::BaseSerializer::RW_VEC2B: { DO_CHECK(osg::Vec2b, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3B: { DO_CHECK(osg::Vec3b, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4B: { DO_CHECK(osg::Vec4b, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2UB: { DO_CHECK(osg::Vec2ub, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3UB: { DO_CHECK(osg::Vec3ub, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4UB: { DO_CHECK(osg::Vec4ub, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2S: { DO_CHECK(osg::Vec2s, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3S: { DO_CHECK(osg::Vec3s, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4S: { DO_CHECK(osg::Vec4s, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2US: { DO_CHECK(osg::Vec2us, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3US: { DO_CHECK(osg::Vec3us, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4US: { DO_CHECK(osg::Vec4us, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2I: { DO_CHECK(osg::Vec2i, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3I: { DO_CHECK(osg::Vec3i, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4I: { DO_CHECK(osg::Vec4i, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC2UI: { DO_CHECK(osg::Vec2ui, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC3UI: { DO_CHECK(osg::Vec3ui, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_VEC4UI: { DO_CHECK(osg::Vec4ui, v); DO_SET(v); } break;
#endif
        case osgDB::BaseSerializer::RW_QUAT: { DO_CHECK(osg::Quat, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_MATRIXF: { DO_CHECK(osg::Matrixf, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_MATRIXD: { DO_CHECK(osg::Matrixd, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_MATRIX: { DO_CHECK(osg::Matrix, v); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_STRING:
            if (pybind11::isinstance<pybind11::str>(value))
            { std::string v = pybind11::cast<std::string>(value); DO_SET(v); } break;
        case osgDB::BaseSerializer::RW_ENUM:
            // TODO
            break;
        case osgDB::BaseSerializer::RW_GLENUM:
            // TODO
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
        default: break;
        }
        PyErr_SetString(PyExc_ValueError, "Bad value type"); return -1;
    }
};

static void createPythonClass(pybind11::module_& m, LibraryEntry* entry, const std::string& name)
{
    if (!m || !entry || name.empty()) return;
    std::string libName = entry->getLibraryName();
    std::string fullName = libName + "::" + name, fullNamePy = libName + "." + name;

    std::vector<PyGetSetDef>* propDefs = new std::vector<PyGetSetDef>;
    std::vector<LibraryEntry::Property> propNames = entry->getPropertyNames(name);
    for (size_t i = 0; i < propNames.size(); ++i)
    {
        LibraryEntry::Property& prop = propNames[i]; if (prop.outdated) continue;
        PythonWrapperObject::Property* propObj = new PythonWrapperObject::Property;
        propObj->name = prop.name; propObj->base = prop.ownerClass; propObj->type = (int)prop.type;

        PyGetSetDef def; def.closure = (void*)propObj;
        def.name = strdup(prop.name.c_str()); def.doc = NULL;
        def.get = &PythonWrapperObject::getProperty;
        def.set = &PythonWrapperObject::setProperty;
        propDefs->push_back(def);
    }
    propDefs->push_back({ nullptr });

    PyType_Slot slots[] =
    {
        { Py_tp_new, (void*)PythonWrapperObject::allocate },
        { Py_tp_dealloc, (void*)PythonWrapperObject::deallocate },
        { Py_tp_getset,  propDefs->data() }, //{ Py_tp_methods, methodDefs.data() },
        { Py_tp_doc, (void*)strdup(fullName.c_str()) }, { 0, nullptr }
    };

    PyType_Spec spec;
    spec.name = strdup(fullNamePy.c_str()); spec.basicsize = sizeof(PythonWrapperObject);
    spec.itemsize = 0; spec.slots = slots; spec.flags = Py_TPFLAGS_DEFAULT;

    PyObject* type = PyType_FromSpec(&spec);
    if (!type)
    {
        PyObject *type = NULL, *val = NULL, *tb = NULL;
        PyErr_Fetch(&type, &val, &tb); PyErr_NormalizeException(&type, &val, &tb);
        PyObject* str = PyUnicode_AsEncodedString(PyObject_Str(val), "utf-8", "strict");

        OSG_WARN << "[PythonScript] Failed create class: " << PyBytes_AS_STRING(str) << "\n";
        throw std::runtime_error(("PyType_FromSpec failed at " + name).c_str());
    }
    else
    {
        PyObject_SetAttrString(type, "_tables_prop", PyCapsule_New(propDefs, nullptr, [](PyObject* o)
            {
                std::vector<PyGetSetDef>* defs = (std::vector<PyGetSetDef>*)PyCapsule_GetPointer(o, nullptr);
                for (size_t i = 0; i < defs->size(); ++i)
                {
                    PythonWrapperObject::Property* p =
                        (PythonWrapperObject::Property*)defs->at(i).closure; delete p;
                }
                delete defs;
            }));
    }

    PyObject_SetAttrString(type, "_user", PyCapsule_New(strdup(libName.c_str()), "library", nullptr));
    if (PyModule_AddObject(m.ptr(), name.c_str(), type) < 0)
        throw std::runtime_error(("PyModule_AddObject failed at " + name).c_str());
}

PYBIND11_EMBEDDED_MODULE(osg, module)
{
    pybind11::class_<osg::Vec2f>(module, "Vec2f")
        .def(pybind11::init<float, float>()) PYBIND_VECTOR(osg::Vec2f);
    pybind11::class_<osg::Vec3f>(module, "Vec3f")
        .def(pybind11::init<float, float, float>()) PYBIND_VECTOR(osg::Vec3f);
    pybind11::class_<osg::Vec4f>(module, "Vec4f")
        .def(pybind11::init<float, float, float, float>()) PYBIND_VECTOR(osg::Vec4f);
    pybind11::class_<osg::Vec2d>(module, "Vec2d")
        .def(pybind11::init<float, float>()) PYBIND_VECTOR(osg::Vec2d);
    pybind11::class_<osg::Vec3d>(module, "Vec3d")
        .def(pybind11::init<float, float, float>()) PYBIND_VECTOR(osg::Vec3d);
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
    for (std::set<std::string>::const_iterator it = classes.begin();
         it != classes.end(); ++it) createPythonClass(module, entry.get(), *it);
}
#endif
