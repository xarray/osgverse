#ifndef MANA_SCRIPT_JSONSCRIPT_HPP
#define MANA_SCRIPT_JSONSCRIPT_HPP

#include <picojson.h>
#include "ScriptBase.h"

namespace osgVerse
{
    class JsonScript : public ScriptBase
    {
    public:
        enum ExecutionType
        {
            EXE_Creation, EXE_Set, EXE_Call,
            EXE_Get, EXE_Remove, EXE_List
        };

        /** Json inputs:
        *   - EXE_Creation
        *     { 'class': ..., 'type': ..., 'uri': ..., 'properties': [{'...': '...'}] }
        *   Json result:
        *     { 'code': ..., 'msg': '...', 'value': ..., 'id': ... }
        */
        picojson::value execute(ExecutionType t, picojson::value in);
    };
}

#endif
