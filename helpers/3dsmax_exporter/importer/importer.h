/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The scene importer
*/

#ifndef OSG2MAX_IMPORTER
#define OSG2MAX_IMPORTER

#include <max.h>
#include "resource.h"

#ifdef _MSC_VER
#   define strcasecmp _stricmp
#   define strncasecmp _strnicmp
#   define snprintf _snprintf
#   if (_MSC_VER<1500)
#       define vsnprintf _vsnprintf
#   endif
#endif

extern MCHAR* GetString(int id);
extern INT_PTR ExecDialogBox(WORD id, HWND parent, DLGPROC proc, LPARAM lParam=0);
extern void NotifyBox(LPCMSTR msg, ...);

class ImporterDesc : public ClassDesc
{
public:
    virtual int IsPublic() { return 1; }
    virtual SClass_ID SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
    virtual Class_ID ClassID() { return Class_ID(0x216222b7, 0x21b94821); }

    virtual void* Create(BOOL loading=FALSE);
    virtual const MCHAR* ClassName();
    virtual const MCHAR* Category();
};

class SceneImportOSG : public SceneImport
{
public:
    virtual int ExtCount();
    virtual const MCHAR* Ext(int n);
    virtual const MCHAR* LongDesc();
    virtual const MCHAR* ShortDesc();
    virtual const MCHAR* AuthorName();
    virtual const MCHAR* CopyrightMessage();
    virtual const MCHAR* OtherMessage1();
    virtual const MCHAR* OtherMessage2();
    virtual unsigned int Version();
    virtual void ShowAbout(HWND hWnd);
    virtual int DoImport(const MCHAR* name, ImpInterface* ii, Interface* gi, BOOL suppressPrompts=FALSE);
};

#endif
