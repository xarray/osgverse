/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The OSG scene importer body
*/

#include <osgDB/WriteFile>
#include "exporter.h"
#include "MaxEnumerator.h"

static BOOL g_disablePrompts = 0;
void NotifyBox( LPCMSTR msg, ... )
{
    if ( g_disablePrompts ) return;
    
    va_list arguments;
    va_start( arguments, msg );
    
    wchar_t realMessage[1024];
    _vsnwprintf( realMessage, sizeof(realMessage), msg, arguments );
    va_end( arguments );
    
    MessageBoxW( NULL, realMessage, GetString(IDS_CATEGORY), MB_OK );
}

/* SceneExportOSG */

int SceneExportOSG::ExtCount()
{ return 5; }

const MCHAR* SceneExportOSG::Ext( int n )
{
    switch (n)
    {
    case 0: return _M("OSG");
    case 1: return _M("OSGT");
    case 2: return _M("OSGB");
    case 3: return _M("OSGX");
    case 4: return _M("IVE");
    default: return _M("");
    }
}

const MCHAR* SceneExportOSG::LongDesc()
{ return GetString(IDS_LONGDESC); }

const MCHAR* SceneExportOSG::ShortDesc()
{ return GetString(IDS_SHORTDESC); }

const MCHAR* SceneExportOSG::AuthorName()
{ return GetString(IDS_AUTHORS); }

const MCHAR* SceneExportOSG::CopyrightMessage()
{ return GetString(IDS_COPYRIGHT); }

const MCHAR* SceneExportOSG::OtherMessage1()
{ return _M(""); }

const MCHAR* SceneExportOSG::OtherMessage2()
{ return _M(""); }

unsigned int SceneExportOSG::Version()
{ return OSG2MAX_VERSION; }

void SceneExportOSG::ShowAbout( HWND hWnd )
{
    // TODO
}

int SceneExportOSG::DoExport( const MCHAR* name, ExpInterface* ei, Interface* gi, BOOL suppressPrompts, DWORD options )
{
    g_disablePrompts = suppressPrompts;
    if ( !suppressPrompts )
    {
        // TODO
    }
    //bool res = checkSafeNetDog(2);
	//if(!res) return IMPEXP_FAIL;

    MaxEnumerator enumerator(ei, gi, options);
    enumerator.setExportSelected( options&SCENE_EXPORT_SELECTED );
    
    enumerator.traverseMaterials( gi );
    if ( enumerator.hasError() ) return IMPEXP_FAIL;
    enumerator.traverseNodes( gi );
    if ( enumerator.hasError() ) return IMPEXP_FAIL;
    
    // _UNICODE will never be defined, so simply treat TCHAR* as char* here
    bool ok = osgDB::writeNodeFile( *enumerator.output(), ws2s(name) );
    if ( !ok ) return IMPEXP_FAIL;
    return IMPEXP_SUCCESS;
}

BOOL SceneExportOSG::SupportsOptions( int ext, DWORD options )
{
    return (options==SCENE_EXPORT_SELECTED ? TRUE : FALSE);
}
