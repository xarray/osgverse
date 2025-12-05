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

PythonScript::PythonScript(const wchar_t* pythonPath)
{
#ifndef VERSE_WASM
    if (pythonPath != NULL) Py_SetPythonHome(pythonPath);
#endif
    try
    {
        pythonWrapperOsg();
        _pythonObject = new PythonObject;
    }
    catch (const std::exception& e)
    {
        OSG_WARN << "[PythonScript] Initialization error: " << e.what() << std::endl;
    }
}

PythonScript::~PythonScript()
{
    PythonObject* pobj = static_cast<PythonObject*>(_pythonObject.get());
    if (pobj) pobj->modules.clear();  // clear modules first
}

bool PythonScript::execute(const std::string& code)
{
#ifdef WITH_PYTHON
    try
    {
        pybind11::exec(code);
        // pybind11::eval_file("script.py");
    }
    catch (const pybind11::error_already_set& e)
    {
        OSG_WARN << "[PythonScript] Execution error: " << e.what() << std::endl;
        return false;
    }
    return true;
#else
    OSG_WARN << "[PythonScript] Not implemented" << std::endl;
    return false;
#endif
}
