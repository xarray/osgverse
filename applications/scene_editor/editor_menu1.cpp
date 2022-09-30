#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include "defines.h"

void EditorContentHandler::createEditorMenu1()
{
    osgVerse::MenuBar::MenuData projMenu(osgVerse::MenuBar::TR("File##menu01"));
    {
        osgVerse::MenuBar::MenuItemData newItem(osgVerse::MenuBar::TR("New Scene##menu0101"));
        projMenu.items.push_back(newItem);

        osgVerse::MenuBar::MenuItemData openItem(osgVerse::MenuBar::TR("Open Scene##menu0102"));
        projMenu.items.push_back(openItem);

        osgVerse::MenuBar::MenuItemData openRecentItem(osgVerse::MenuBar::TR("Open Recently##menu0103"));
        {
            osgVerse::MenuBar::MenuItemData clearRecentItem(osgVerse::MenuBar::TR("Clear All##menu010301"));
            openRecentItem.subItems.push_back(clearRecentItem);
        }
        projMenu.items.push_back(openRecentItem);

        osgVerse::MenuBar::MenuItemData saveItem(osgVerse::MenuBar::TR("Save Scene##menu0104"));
        projMenu.items.push_back(saveItem);

        osgVerse::MenuBar::MenuItemData saveAsItem(osgVerse::MenuBar::TR("Save Scene As##menu0105"));
        projMenu.items.push_back(saveAsItem);

        projMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData workspaceItem(osgVerse::MenuBar::TR("Open Workspace##menu0106"));
        projMenu.items.push_back(workspaceItem);

        osgVerse::MenuBar::MenuItemData settingItem(osgVerse::MenuBar::TR("Build Settings##menu0107"));
        projMenu.items.push_back(settingItem);

        osgVerse::MenuBar::MenuItemData buildItem(osgVerse::MenuBar::TR("Build App##menu0108"));
        projMenu.items.push_back(buildItem);

        projMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData quitItem(osgVerse::MenuBar::TR("Exit##menu0109"));
        projMenu.items.push_back(quitItem);
    }
    _mainMenu->menuDataList.push_back(projMenu);

    osgVerse::MenuBar::MenuData editMenu(osgVerse::MenuBar::TR("Edit##menu02"));
    {
        osgVerse::MenuBar::MenuItemData undoItem(osgVerse::MenuBar::TR("Undo##menu0201"));
        editMenu.items.push_back(undoItem);

        osgVerse::MenuBar::MenuItemData redoItem(osgVerse::MenuBar::TR("Redo##menu0202"));
        editMenu.items.push_back(redoItem);

        osgVerse::MenuBar::MenuItemData undoListItem(osgVerse::MenuBar::TR("Undo List##menu0203"));
        editMenu.items.push_back(undoListItem);

        editMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData multiSelItem(osgVerse::MenuBar::TR("Multi-Select Mode##menu0204"));
        editMenu.items.push_back(multiSelItem);

        osgVerse::MenuBar::MenuItemData selAllItem(osgVerse::MenuBar::TR("Select All##menu0205"));
        editMenu.items.push_back(selAllItem);

        osgVerse::MenuBar::MenuItemData deselAllItem(osgVerse::MenuBar::TR("Deselect All##menu0206"));
        editMenu.items.push_back(deselAllItem);

        osgVerse::MenuBar::MenuItemData invSelItem(osgVerse::MenuBar::TR("Select Inverted##menu0207"));
        editMenu.items.push_back(invSelItem);

        editMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData cutItem(osgVerse::MenuBar::TR("Cut##menu0208"));
        editMenu.items.push_back(cutItem);

        osgVerse::MenuBar::MenuItemData copyItem(osgVerse::MenuBar::TR("Copy##menu0209"));
        editMenu.items.push_back(copyItem);

        osgVerse::MenuBar::MenuItemData shareItem(osgVerse::MenuBar::TR("Copy As Share##menu0210"));
        editMenu.items.push_back(shareItem);

        osgVerse::MenuBar::MenuItemData pasteItem(osgVerse::MenuBar::TR("Paste##menu0211"));
        editMenu.items.push_back(pasteItem);

        osgVerse::MenuBar::MenuItemData unshareItem(osgVerse::MenuBar::TR("Unshare##menu0212"));
        editMenu.items.push_back(unshareItem);

        editMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData playItem(osgVerse::MenuBar::TR("Play##menu0213"));
        editMenu.items.push_back(playItem);

        osgVerse::MenuBar::MenuItemData pauseItem(osgVerse::MenuBar::TR("Pause##menu0214"));
        editMenu.items.push_back(pauseItem);

        osgVerse::MenuBar::MenuItemData stopItem(osgVerse::MenuBar::TR("Stop##menu0215"));
        editMenu.items.push_back(stopItem);

        editMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData findItem(osgVerse::MenuBar::TR("Find##menu0216"));
        editMenu.items.push_back(findItem);

        editMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData perfItem(osgVerse::MenuBar::TR("Perferences##menu0217"));
        editMenu.items.push_back(perfItem);

    }
    _mainMenu->menuDataList.push_back(editMenu);
}
