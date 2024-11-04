/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The OSG scene importer body
*/

#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include "importer.h"
#include "MaxConstructor.h"

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

/* MergeOrReplaceDlgProc */

static BOOL g_replaceScene = 0;
static INT_PTR CALLBACK MergeOrReplaceDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
    case WM_INITDIALOG:
        CheckRadioButton( hDlg, IDC_MERGE_SCENE, IDC_REPLACE_SCENE, IDC_MERGE_SCENE );
        CenterWindow( hDlg, GetParent(hDlg) );
        SetFocus( hDlg );
        break;
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDOK:
            g_replaceScene = IsDlgButtonChecked(hDlg, IDC_REPLACE_SCENE);
            EndDialog( hDlg, 1 );
            return TRUE;
        case IDCANCEL:
            EndDialog( hDlg, 0 );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* SceneImportOSG */

int SceneImportOSG::ExtCount()
{ return 5; }

const MCHAR* SceneImportOSG::Ext( int n )
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

const MCHAR* SceneImportOSG::LongDesc()
{ return GetString(IDS_LONGDESC); }

const MCHAR* SceneImportOSG::ShortDesc()
{ return GetString(IDS_SHORTDESC); }

const MCHAR* SceneImportOSG::AuthorName()
{ return GetString(IDS_AUTHORS); }

const MCHAR* SceneImportOSG::CopyrightMessage()
{ return GetString(IDS_COPYRIGHT); }

const MCHAR* SceneImportOSG::OtherMessage1()
{ return _M(""); }

const MCHAR* SceneImportOSG::OtherMessage2()
{ return _M(""); }

unsigned int SceneImportOSG::Version()
{ return OSG2MAX_VERSION; }

void SceneImportOSG::ShowAbout( HWND hWnd )
{
    // TODO
}

int SceneImportOSG::DoImport( const MCHAR* name, ImpInterface* ii, Interface* gi, BOOL suppressPrompts )
{
    g_disablePrompts = suppressPrompts;
    if ( !suppressPrompts )
    {
        // Show the 'merge or replace current scene' dialog box
        if (!ExecDialogBox(IDD_MERGE_OR_REPLACE, gi->GetMAXHWnd(), MergeOrReplaceDlgProc))
            return IMPEXP_CANCEL;
    }
    
    if ( g_replaceScene )
    {
        // Try to create a new scene for the reason of replacing current scene
        if ( !ii->NewScene() ) return IMPEXP_CANCEL;
    }
    //bool res = checkSafeNetDog(2);
	//if(!res) return IMPEXP_FAIL;

    // _UNICODE will never be defined, so simply treat TCHAR* as char* here
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile( ws2s(name) );
    if ( !scene ) return IMPEXP_FAIL;
    else scene->setName( osgDB::getStrippedName(ws2s(name)) );
    
    MaxConstructor constructor(ii, gi);
    constructor.pushPrefix( osgDB::getFilePath(ws2s(name)) );
    constructor.setConvertGeodeToMesh( true );
    scene->accept( constructor );
    
    constructor.finalize();
    if ( constructor.hasError() ) return IMPEXP_FAIL;
    
    ii->RedrawViews();
    return IMPEXP_SUCCESS;
}
