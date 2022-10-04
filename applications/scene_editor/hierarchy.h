#ifndef MANA_HIERARCHY_HPP
#define MANA_HIERARCHY_HPP

#include "defines.h"

namespace osgVerse
{ struct CommandData; }

class Hierarchy : public osgVerse::ImGuiComponentBase
{
public:
    Hierarchy();
    osgVerse::Window* getWindow() { return _treeWindow.get(); }

    bool handleCommand(osgVerse::CommandData* cmd);
    bool handleItemCommand(osgVerse::CommandData* cmd);

    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);
    void showPopupMenu(osgVerse::TreeView::TreeData* item, osgVerse::ImGuiManager* mgr,
                       osgVerse::ImGuiContentHandler* content);

    void addModelFromUrl(const std::string& url);

protected:
    osg::ref_ptr<osgVerse::Window> _treeWindow;
    osg::ref_ptr<osgVerse::TreeView> _treeView;
    osg::ref_ptr<osgVerse::TreeView::TreeData> _camTreeData, _sceneTreeData;
    osg::observer_ptr<osgVerse::TreeView::TreeData> _selectedItem;
    bool _selectedItemPopupTriggered;
};

#endif
