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
            EXE_Creation, EXE_Set, EXE_Get,
            EXE_Remove, EXE_List
        };

        /** Json inputs:
        *   - EXE_Creation
        *     { 'class': ..., , 'properties': [{'...': '...'}] }
        *     { 'type': ..., 'uri': ..., 'properties': [{'...': '...'}] }
        *   - EXE_Set
        *     { 'object': ..., 'properties': [{'...': '...'}] }
        *     { 'object': ..., 'method': ..., 'properties': [..., ...] }
        *   - EXE_Get
        *     { 'object': ..., 'property': ... }
        *   - EXE_Remove
        *     { 'object': ... }
        *   - EXE_List
        *     { 'library': ... }, { 'library': ..., 'class': ... }, { 'object': ... }
        *   Json result:
        *     { 'code': ..., 'message': '...', 'value': ..., 'object': ... }
        */
        picojson::value execute(ExecutionType t, picojson::value in);
    };
}

#endif
