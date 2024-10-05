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

        TreeView* getTreeView() { return _treeView.get(); }
        TreeView::TreeData* getViewerRoot() { return _camTreeData.get(); }
        TreeView::TreeData* getSceneRoot() { return _sceneTreeData.get(); }

        typedef std::map<osg::Object*, osg::observer_ptr<TreeView::TreeData>> NodeToItemMapper;
        NodeToItemMapper& getNodeToItemMap() { return _nodeToItemMap; }

        typedef std::function<void(TreeView*, TreeView::TreeData*)> ActionCallback;
        void setItemClickAction(ActionCallback act) { _clickAction = act; }
        void setItemDoubleClickAction(ActionCallback act) { _dbClickAction = act; }

        TreeView::TreeData* addItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph = true);
        bool removeItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph = true);

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                           osgVerse::ImGuiContentHandler* content);

    protected:
        osg::ref_ptr<TreeView> _treeView;
        osg::ref_ptr<TreeView::TreeData> _camTreeData, _sceneTreeData;
        osg::observer_ptr<TreeView::TreeData> _selectedItem;
        std::vector<MenuBarBase::MenuItemData> _popupMenus;
        NodeToItemMapper _nodeToItemMap;

        ActionCallback _clickAction, _dbClickAction;
        std::string _postfix;
        bool _selectedItemPopupTriggered;
    };
}

#endif
