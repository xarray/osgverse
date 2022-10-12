#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include "defines.h"

void EditorContentHandler::createEditorMenu3()
{
    osgVerse::MenuBar::MenuData utilityMenu(osgVerse::MenuBar::TR("Utility##menu06"));
    {
        osgVerse::MenuBar::MenuItemData skyboxesItem(osgVerse::MenuBar::TR("Skybox Manager##menu0601"));
        utilityMenu.items.push_back(skyboxesItem);

        osgVerse::MenuBar::MenuItemData maskItem(osgVerse::MenuBar::TR("Node Mask Manager##menu0602"));
        utilityMenu.items.push_back(maskItem);

        osgVerse::MenuBar::MenuItemData pluginsItem(osgVerse::MenuBar::TR("Plugin Manager##menu0603"));
        utilityMenu.items.push_back(pluginsItem);
    }
    _mainMenu->menuDataList.push_back(utilityMenu);

    osgVerse::MenuBar::MenuData windowMenu(osgVerse::MenuBar::TR("Window##menu07"));
    {
        osgVerse::MenuBar::MenuItemData showHieItem(osgVerse::MenuBar::TR("Show Hierarchy##menu0701"));
        showHieItem.selected = true; windowMenu.items.push_back(showHieItem);

        osgVerse::MenuBar::MenuItemData showPropItem(osgVerse::MenuBar::TR("Show Properties##menu0702"));
        showPropItem.selected = true; windowMenu.items.push_back(showPropItem);

        osgVerse::MenuBar::MenuItemData showLogicItem(osgVerse::MenuBar::TR("Show Scene Logic##menu0703"));
        showLogicItem.selected = true; windowMenu.items.push_back(showLogicItem);

        osgVerse::MenuBar::MenuItemData showPanelsItem(osgVerse::MenuBar::TR("Logic Panels##menu0704"));
        {
            osgVerse::MenuBar::MenuItemData showResItem(osgVerse::MenuBar::TR("Show Resources##menu070401"));
            showResItem.selected = true; showPanelsItem.subItems.push_back(showResItem);

            osgVerse::MenuBar::MenuItemData showTlItem(osgVerse::MenuBar::TR("Show Timeline##menu070402"));
            showTlItem.selected = true; showPanelsItem.subItems.push_back(showTlItem);

            osgVerse::MenuBar::MenuItemData showSpiItem(osgVerse::MenuBar::TR("Show Spider##menu070403"));
            showSpiItem.selected = true; showPanelsItem.subItems.push_back(showSpiItem);

            osgVerse::MenuBar::MenuItemData showConItem(osgVerse::MenuBar::TR("Show Console##menu070404"));
            showConItem.selected = true; showPanelsItem.subItems.push_back(showConItem);
        }
        windowMenu.items.push_back(showPanelsItem);

        windowMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData defWinItem(osgVerse::MenuBar::TR("Default Layout##menu0705"));
        windowMenu.items.push_back(defWinItem);

        osgVerse::MenuBar::MenuItemData tallWinItem(osgVerse::MenuBar::TR("Tall Layout##menu0706"));
        windowMenu.items.push_back(tallWinItem);

        osgVerse::MenuBar::MenuItemData noWinItem(osgVerse::MenuBar::TR("Clean Layout##menu0707"));
        windowMenu.items.push_back(noWinItem);

        windowMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData fullScrItem(osgVerse::MenuBar::TR("Full Screen##menu0708"));
        fullScrItem.selected = true; windowMenu.items.push_back(fullScrItem);
    }
    _mainMenu->menuDataList.push_back(windowMenu);

    osgVerse::MenuBar::MenuData helpMenu(osgVerse::MenuBar::TR("Help##menu08"));
    {
        osgVerse::MenuBar::MenuItemData aboutItem(osgVerse::MenuBar::TR("About...##menu0801"));
        helpMenu.items.push_back(aboutItem);
    }
    _mainMenu->menuDataList.push_back(helpMenu);
}
