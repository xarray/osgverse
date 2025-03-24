#include <osg/Version>
#include <osg/Quat>
#include <osg/Matrix>

#include "PythonScript.h"
#ifdef WITH_PYTHON
#   include <Python.h>
#   include "pybind11/pybind11.h"
#   include "pybind11/embed.h"
#endif

using namespace osgVerse;
struct DummyClass
{
    osg::observer_ptr<LibraryEntry> entry;
    osg::ref_ptr<osg::Object> object;
};

#ifdef WITH_PYTHON
static bool runScript(const std::string& script)
{
    pybind11::scoped_interpreter guard {};
    try
    {
        pybind11::module_ script = pybind11::module_::import("__main__");
        script.attr("exec")(script);
        return true;
    }
    catch (pybind11::error_already_set& e)
    {
        OSG_WARN << "Python error: " << e.what() << std::endl;
        return false;
    }
}

static void createPythonClass(pybind11::module_& m, LibraryEntry* entry, const std::string& name)
{
    pybind11::class_<DummyClass> classObj(m, name.c_str());
    classObj.def(pybind11::init([&]()
        {
            DummyClass* dummy = new DummyClass;
            dummy->object = entry->create(name);
            dummy->entry = entry; return dummy;
        }));

    std::vector<LibraryEntry::Property> propNames = entry->getPropertyNames(name);
    for (size_t i = 0; i < propNames.size(); ++i)
    {
        LibraryEntry::Property& prop = propNames[i];
        switch (prop.type)
        {
        case osgDB::BaseSerializer::RW_FLOAT:
            classObj.def(("set" + prop.name).c_str(), [&](DummyClass* s, float v)
                { s->entry->setProperty(s->object.get(), prop.name, v); });
            classObj.def(("get" + prop.name).c_str(), [&](DummyClass* s)
                { float v = 0.0f; s->entry->getProperty(s->object.get(), prop.name, v); return v; });
            break;
        case osgDB::BaseSerializer::RW_VEC3F:
            classObj.def(("set" + prop.name).c_str(), [&](DummyClass* s, const osg::Vec3f& v)
                { s->entry->setProperty(s->object.get(), prop.name, v); });
            classObj.def(("get" + prop.name).c_str(), [&](DummyClass* s)
                { osg::Vec3f v; s->entry->getProperty(s->object.get(), prop.name, v); return v; });
            break;
        default:
            OSG_NOTICE << "[PythonScript] Python binding of class " << name << "'s property "
                       << prop.name << " is not implemented" << std::endl;
            break;
        }
    }
}

PYBIND11_MODULE(osg, module)
{
    pybind11::class_<osg::Vec3f>(module, "Vec3f")
        .def(pybind11::init()).def(pybind11::init<float, float, float>())
        .def("valid", &osg::Vec3f::valid).def("length", &osg::Vec3f::length)
        .def("__getitem__", [](const osg::Vec3f& s, size_t i) { return s[i]; })
        .def("__setitem__", [](osg::Vec3f& s, size_t i, float v) { s[i] = v; });

    osg::ref_ptr<LibraryEntry> entry = new LibraryEntry("osg");
    const std::set<std::string>& classes = entry->getClasses();
    for (std::set<std::string>::const_iterator itr = classes.begin();
         itr != classes.end(); ++itr)
    { createPythonClass(module, entry.get(), *itr); }
}

PYBIND11_MODULE(osgVerse, module)
{
    // TODO
}
#endif
