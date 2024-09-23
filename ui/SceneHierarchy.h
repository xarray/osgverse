#ifndef MANA_UI_SCENEHIERARCHY_HPP
#define MANA_UI_SCENEHIERARCHY_HPP

#include "ImGui.h"
#include "ImGuiComponents.h"
#include "../script/Entry.h"
#include <osgViewer/View>
#include <map>

namespace osgVerse
{
    class SceneHierarchy : public ImGuiComponentBase
    {
    public:
        SceneHierarchy();
        void setViewer(osgViewer::View* view);

        void addItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph);
        void removeItem(TreeView::TreeData* parent, osg::Object* obj);

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                           osgVerse::ImGuiContentHandler* content);

    protected:
        osg::ref_ptr<Window> _treeWindow;
        osg::ref_ptr<TreeView> _treeView;
        osg::ref_ptr<TreeView::TreeData> _camTreeData, _sceneTreeData;
        std::vector<MenuBarBase::MenuItemData> _popupMenus;
        osg::observer_ptr<TreeView::TreeData> _selectedItem;
        std::string _postfix;
        bool _selectedItemPopupTriggered;
    };
}

#endif
