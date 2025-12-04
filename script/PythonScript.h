#ifndef MANA_SCRIPT_PYTHONSCRIPT_HPP
#define MANA_SCRIPT_PYTHONSCRIPT_HPP

#include "ScriptBase.h"

namespace osgVerse
{
    class PythonScript : public ScriptBase
    {
    public:
        PythonScript();

        bool execute(const std::string& code);

    protected:
        virtual ~PythonScript();
        osg::ref_ptr<osg::Referenced> _pythonObject;
    };
}

#endif
