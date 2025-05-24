#include <osg/Version>
#include <iostream>
#include "PythonScript.h"
using namespace osgVerse;

extern void pythonWrapperOsg();

#ifdef WITH_PYTHON
#   include <Python.h>
#   include "pybind11/pybind11.h"
#   include "pybind11/embed.h"

struct PythonObject : public osg::Referenced
{
    std::map<std::string, pybind11::module_> modules;
    pybind11::scoped_interpreter guard;

    PythonObject()
    {
        try
        {
            modules["osg"] = pybind11::module_::import("osg");
            // TODO
        }
        catch (std::exception& e)
        {
            OSG_WARN << "[PythonScript] Failed importing modules: "
                     << e.what() << std::endl;
        }
    }
};
#else
struct PythonObject : public osg::Referenced
{
    std::map<std::string, std::string> modules;
};
#endif

PythonScript::PythonScript()
{
    pythonWrapperOsg();
    _pythonObject = new PythonObject;
}

PythonScript::~PythonScript()
{
    PythonObject* pobj = static_cast<PythonObject*>(_pythonObject.get());
    if (pobj) pobj->modules.clear();  // clear modules first
}
