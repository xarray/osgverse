#ifndef MANA_UI_SCENEHIERARCHY_HPP
#define MANA_UI_SCENEHIERARCHY_HPP

#include "ImGui.h"
#include "ImGuiComponents.h"
#include "../script/Entry.h"
#include <osgViewer/View>
#include <map>

namespace osgVerse
{
    struct SceneDataProxy : public osg::Referenced
    {
        osg::observer_ptr<osg::Object> data;
        SceneDataProxy(osg::Object* o) { data = o; }

        template<typename T>
        static T get(osg::Referenced* ud)
        {
            SceneDataProxy* p = static_cast<SceneDataProxy*>(ud);
            return p ? dynamic_cast<T>(p->data.get()) : NULL;
        }
    };

    class SceneHierarchy : public ImGuiComponentBase
    {
    public:
        SceneHierarchy();
        void setViewer(osgViewer::View* view, osg::Node* rootNode = NULL);

        TreeView* getTreeView() { return _treeView.get(); }
        TreeView::TreeData* getViewerRoot() { return _camTreeData.get(); }
        TreeView::TreeData* getSceneRoot() { return _sceneTreeData.get(); }

        typedef std::map<osg::Object*, osg::observer_ptr<TreeView::TreeData>> NodeToItemMapper;
        NodeToItemMapper& getNodeToItemMap() { return _nodeToItemMap; }

        typedef std::map<std::string, osg::ref_ptr<LibraryEntry>> EntryMapper;
        EntryMapper& getEntries() { return _entries; }

        typedef std::function<void(TreeView*, TreeView::TreeData*)> ActionCallback;
        void setItemClickAction(ActionCallback act) { _clickAction = act; }
        void setItemDoubleClickAction(ActionCallback act) { _dbClickAction = act; }

        TreeView::TreeData* addItem(TreeView::TreeData* parent, osg::Object* obj,
                                    bool asSubGraph = true, unsigned int parentMask = 0xffffffff);
        bool removeItem(TreeView::TreeData* parent, osg::Object* obj, bool asSubGraph = true);
        void refreshItem(TreeView::TreeData* parent = NULL);

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                           osgVerse::ImGuiContentHandler* content);

    protected:
        MenuBar::MenuItemData createPopupMenu(const std::string& name,
                                              ImGuiComponentBase::ActionCallback cb = NULL);
        void updateStateInformation(TreeView::TreeData* item, osg::Object* obj, unsigned int parentMask);
        void removeUnusedItem(TreeView::TreeData* parent, bool recursively);

        bool addOperation(TreeView::TreeData* item, const std::string& childType, bool isGeom = false);
        bool removeOperation(TreeView::TreeData* child);

        osg::ref_ptr<TreeView> _treeView;
        osg::ref_ptr<TreeView::TreeData> _camTreeData, _sceneTreeData;
        osg::ref_ptr<MenuBar> _popupBar;
        osg::observer_ptr<TreeView::TreeData> _selectedItem;
        std::vector<MenuBarBase::MenuItemData> _popupMenus;
        NodeToItemMapper _nodeToItemMap;
        EntryMapper _entries;

        ActionCallback _clickAction, _dbClickAction;
        std::string _postfix;
        int _stateFlags;  // small button display: 0: ss, 1: cb, 2: refcount
        bool _selectedItemPopupTriggered;
    };
}

#endif
