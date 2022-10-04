#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "hierarchy.h"
#include "properties.h"
#include "scenelogic.h"
#include "defines.h"

void EditorContentHandler::createEditorMenu2()
{
    osgVerse::MenuBar::MenuData assetMenu(osgVerse::MenuBar::TR("Assets##menu03"));
    {
        osgVerse::MenuBar::MenuItemData transNodeItem(osgVerse::MenuBar::TR("New Node##menu0301"));
        assetMenu.items.push_back(transNodeItem);

        osgVerse::MenuBar::MenuItemData new3dItem(osgVerse::MenuBar::TR("New Drawable##menu0302"));
        {
            osgVerse::MenuBar::MenuItemData boxItem(osgVerse::MenuBar::TR("Box##menu030201"));
            new3dItem.subItems.push_back(boxItem);

            osgVerse::MenuBar::MenuItemData sphereItem(osgVerse::MenuBar::TR("Sphere##menu030202"));
            new3dItem.subItems.push_back(sphereItem);

            osgVerse::MenuBar::MenuItemData cylinderItem(osgVerse::MenuBar::TR("Cylinder##menu030203"));
            new3dItem.subItems.push_back(cylinderItem);

            osgVerse::MenuBar::MenuItemData coneItem(osgVerse::MenuBar::TR("Cone##menu030204"));
            new3dItem.subItems.push_back(coneItem);

            osgVerse::MenuBar::MenuItemData capsuleItem(osgVerse::MenuBar::TR("Capsule##menu030205"));
            new3dItem.subItems.push_back(capsuleItem);

            osgVerse::MenuBar::MenuItemData quadItem(osgVerse::MenuBar::TR("Quad##menu030206"));
            new3dItem.subItems.push_back(quadItem);
        }
        assetMenu.items.push_back(new3dItem);

        osgVerse::MenuBar::MenuItemData newFxItem(osgVerse::MenuBar::TR("New Object##menu0303"));
        {
            osgVerse::MenuBar::MenuItemData camItem(osgVerse::MenuBar::TR("Camera##menu030301"));
            newFxItem.subItems.push_back(camItem);

            osgVerse::MenuBar::MenuItemData lightItem(osgVerse::MenuBar::TR("Light##menu030302"));
            newFxItem.subItems.push_back(lightItem);
        }
        assetMenu.items.push_back(newFxItem);

        osgVerse::MenuBar::MenuItemData importItem(osgVerse::MenuBar::TR("Import Scene##menu0304"));
        importItem.callback = [&](osgVerse::ImGuiManager*, osgVerse::ImGuiContentHandler*,
            osgVerse::ImGuiComponentBase* me)
        {
            _currentDialogName = "OpenModelFile##ed00";
            osgVerse::ImGuiComponentBase::registerFileDialog(
                _currentDialogName, osgVerse::ImGuiComponentBase::TR("Select 3D model file"),
                true, ".", ".*,.osgb,.fbx,.gltf");
        };
        assetMenu.items.push_back(importItem);

        assetMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData enNodeItem(osgVerse::MenuBar::TR("Enable Node##menu0305"));
        assetMenu.items.push_back(enNodeItem);

        osgVerse::MenuBar::MenuItemData renNodeItem(osgVerse::MenuBar::TR("Rename Node##menu0306"));
        assetMenu.items.push_back(renNodeItem);

        osgVerse::MenuBar::MenuItemData delNodeItem(osgVerse::MenuBar::TR("Delete Node##menu0307"));
        assetMenu.items.push_back(delNodeItem);

        assetMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData addResItem(osgVerse::MenuBar::TR("Add Resource##menu0308"));
        {
            osgVerse::MenuBar::MenuItemData resModelItem(osgVerse::MenuBar::TR("3D Models##menu030801"));
            addResItem.subItems.push_back(resModelItem);

            osgVerse::MenuBar::MenuItemData resTexItem(osgVerse::MenuBar::TR("Textures##menu030802"));
            addResItem.subItems.push_back(resTexItem);

            osgVerse::MenuBar::MenuItemData resMediaItem(osgVerse::MenuBar::TR("Multimedia##menu030803"));
            addResItem.subItems.push_back(resMediaItem);

            osgVerse::MenuBar::MenuItemData resUiItem(osgVerse::MenuBar::TR("UI Page##menu030804"));
            addResItem.subItems.push_back(resUiItem);
        }
        assetMenu.items.push_back(addResItem);

        osgVerse::MenuBar::MenuItemData delResItem(osgVerse::MenuBar::TR("Delete Resource##menu0309"));
        assetMenu.items.push_back(delResItem);

        osgVerse::MenuBar::MenuItemData renResItem(osgVerse::MenuBar::TR("Rename Resource##menu03010"));
        assetMenu.items.push_back(renResItem);

        osgVerse::MenuBar::MenuItemData updateResItem(osgVerse::MenuBar::TR("Update Resource##menu0311"));
        assetMenu.items.push_back(updateResItem);

        osgVerse::MenuBar::MenuItemData findResItem(osgVerse::MenuBar::TR("Find Resource##menu0312"));
        assetMenu.items.push_back(findResItem);

        assetMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData exportItem(osgVerse::MenuBar::TR("Export Scene##menu03010"));
        assetMenu.items.push_back(exportItem);
    }
    _mainMenu->menuDataList.push_back(assetMenu);

    osgVerse::MenuBar::MenuData compMenu(osgVerse::MenuBar::TR("Components##menu04"));
    {
        osgVerse::MenuBar::MenuItemData addTexItem(osgVerse::MenuBar::TR("Add Texture##menu0401"));
        compMenu.items.push_back(addTexItem);

        osgVerse::MenuBar::MenuItemData addShaderItem(osgVerse::MenuBar::TR("Add Shader##menu0402"));
        compMenu.items.push_back(addShaderItem);

        osgVerse::MenuBar::MenuItemData addStateSetItem(osgVerse::MenuBar::TR("Add StateSet##menu0403"));
        compMenu.items.push_back(addStateSetItem);

        osgVerse::MenuBar::MenuItemData addSpiderItem(osgVerse::MenuBar::TR("Add Spider##menu0403"));
        compMenu.items.push_back(addSpiderItem);

        osgVerse::MenuBar::MenuItemData addTimelineItem(osgVerse::MenuBar::TR("Add Timeline##menu0403"));
        compMenu.items.push_back(addTimelineItem);

        osgVerse::MenuBar::MenuItemData addCustomItem(osgVerse::MenuBar::TR("Add Custom##menu0404"));
        {
            osgVerse::MenuBar::MenuItemData addTimelineItem(osgVerse::MenuBar::TR("Lua Coder##menu040401"));
            addCustomItem.subItems.push_back(addTimelineItem);
        }
        compMenu.items.push_back(addCustomItem);

        compMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData enableItem(osgVerse::MenuBar::TR("Enable Component##menu0405"));
        compMenu.items.push_back(enableItem);

        osgVerse::MenuBar::MenuItemData copyItem(osgVerse::MenuBar::TR("Copy Component##menu0406"));
        compMenu.items.push_back(copyItem);

        osgVerse::MenuBar::MenuItemData pasteItem(osgVerse::MenuBar::TR("Paste Component##menu0407"));
        compMenu.items.push_back(pasteItem);

        osgVerse::MenuBar::MenuItemData deleteItem(osgVerse::MenuBar::TR("Delete Component##menu0408"));
        compMenu.items.push_back(deleteItem);

        compMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData editCodeItem(osgVerse::MenuBar::TR("Edit Code##menu0409"));
        compMenu.items.push_back(editCodeItem);
    }
    _mainMenu->menuDataList.push_back(compMenu);

    osgVerse::MenuBar::MenuData viewMenu(osgVerse::MenuBar::TR("View##menu05"));
    {
        osgVerse::MenuBar::MenuItemData viewportItem(osgVerse::MenuBar::TR("Change View##menu0501"));
        {
            osgVerse::MenuBar::MenuItemData view1Item(osgVerse::MenuBar::TR("Perspective##menu050101"));
            viewportItem.subItems.push_back(view1Item);

            osgVerse::MenuBar::MenuItemData viewTopItem(osgVerse::MenuBar::TR("Top View##menu050102"));
            viewportItem.subItems.push_back(viewTopItem);

            osgVerse::MenuBar::MenuItemData viewFrontItem(osgVerse::MenuBar::TR("Front View##menu050103"));
            viewportItem.subItems.push_back(viewFrontItem);

            osgVerse::MenuBar::MenuItemData viewLeftItem(osgVerse::MenuBar::TR("Left View##menu050104"));
            viewportItem.subItems.push_back(viewLeftItem);

            osgVerse::MenuBar::MenuItemData view4Item(osgVerse::MenuBar::TR("Four View##menu050105"));
            viewportItem.subItems.push_back(view4Item);
        }
        viewMenu.items.push_back(viewportItem);

        osgVerse::MenuBar::MenuItemData maniItem(osgVerse::MenuBar::TR("Change Manipulator##menu0502"));
        {
            osgVerse::MenuBar::MenuItemData arcballItem(osgVerse::MenuBar::TR("Arcball##menu050201"));
            maniItem.subItems.push_back(arcballItem);

            osgVerse::MenuBar::MenuItemData firstPerItem(osgVerse::MenuBar::TR("First Person##menu050202"));
            maniItem.subItems.push_back(firstPerItem);
        }
        viewMenu.items.push_back(maniItem);

        viewMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData setHomeItem(osgVerse::MenuBar::TR("Home Positions##menu0503"));
        {
            osgVerse::MenuBar::MenuItemData clearHomeItem(osgVerse::MenuBar::TR("Clear All##menu050301"));
            setHomeItem.subItems.push_back(clearHomeItem);
        }
        viewMenu.items.push_back(setHomeItem);

        osgVerse::MenuBar::MenuItemData recHomeItem(osgVerse::MenuBar::TR("Record Home##menu0504"));
        viewMenu.items.push_back(recHomeItem);

        osgVerse::MenuBar::MenuItemData goHomeItem(osgVerse::MenuBar::TR("Go Home##menu0505"));
        viewMenu.items.push_back(goHomeItem);

        viewMenu.items.push_back(osgVerse::MenuBar::MenuItemData::separator);

        osgVerse::MenuBar::MenuItemData showWireItem(osgVerse::MenuBar::TR("Show Wireframe##menu0506"));
        viewMenu.items.push_back(showWireItem);

        osgVerse::MenuBar::MenuItemData showFpsItem(osgVerse::MenuBar::TR("Show FPS##menu0507"));
        viewMenu.items.push_back(showFpsItem);
    }
    _mainMenu->menuDataList.push_back(viewMenu);
}
