/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The DLL entry
*/

#include "importer.h"

static HINSTANCE g_hInstance;
static ImporterDesc g_osg2max;

MCHAR* GetString( int id )
{
    static MCHAR buf[256];
    if (g_hInstance)
        return LoadStringW(g_hInstance, id, buf, sizeof(buf)) ? buf : NULL;
    return NULL;
}

INT_PTR ExecDialogBox( WORD id, HWND parent, DLGPROC proc, LPARAM lParam )
{
    if (lParam)
        return DialogBoxParam(g_hInstance, MAKEINTRESOURCE(id), parent, proc, lParam);
    else
        return DialogBox(g_hInstance, MAKEINTRESOURCE(id), parent, proc);
}

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.
BOOL WINAPI DllMain( HINSTANCE hinstDLL,ULONG fdwReason, LPVOID /*lpvReserved*/ )
{
    if( fdwReason==DLL_PROCESS_ATTACH )
    {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(g_hInstance);
    }
    return TRUE;
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const MCHAR* LibDescription()
{ return GetString(IDS_LIBDESCRIPTION); }

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) int LibNumberClasses()
{ return 1; }

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc( int i )
{
    switch (i)
    {
    case 0: return &g_osg2max;
    default: return 0;
    }
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{ return VERSION_3DSMAX; }

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{ return 1; }

/* ImporterDesc */

void* ImporterDesc::Create( BOOL loading )
{ return new SceneImportOSG; }

const MCHAR* ImporterDesc::ClassName()
{ return GetString(IDS_CLASS_NAME); }

const MCHAR* ImporterDesc::Category()
{ return GetString(IDS_CATEGORY); }
