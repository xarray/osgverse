#ifndef MANA_HIERARCHY_HPP
#define MANA_HIERARCHY_HPP

#include "defines.h"

namespace osgVerse
{ struct CommandData; }

class Hierarchy : public osgVerse::ImGuiComponentBase
{
public:
    Hierarchy(EditorContentHandler* ech);
    osgVerse::Window* getWindow() { return _treeWindow.get(); }

    bool handleCommand(osgVerse::CommandData* cmd);
    bool handleItemCommand(osgVerse::CommandData* cmd);

    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);
    void showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                       osgVerse::ImGuiContentHandler* content);

    void addCreatedNode(osg::Node* node);
    void addCreatedDrawable(osg::Drawable* drawable);
    void addModelFromUrl(const std::string& url);

    void deleteSelectedNodes();

protected:
    osg::Group* getSelectedGroup();
    osg::Node* getOrCreateSelectedGeode();
    
    osg::ref_ptr<osgVerse::Window> _treeWindow;
    osg::ref_ptr<osgVerse::TreeView> _treeView;
    osg::ref_ptr<osgVerse::TreeView::TreeData> _camTreeData, _sceneTreeData;
    std::vector<osgVerse::MenuBarBase::MenuItemData> _popupMenus;
    osg::observer_ptr<osgVerse::TreeView::TreeData> _selectedItem;
    bool _selectedItemPopupTriggered;
};

#endif
