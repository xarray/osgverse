#ifndef MANA_HIERARCHY_HPP
#define MANA_HIERARCHY_HPP

#include "defines.h"

namespace osgVerse
{ struct CommandData; }

class Hierarchy : public osgVerse::ImGuiComponentBase
{
public:
    Hierarchy();
    bool handleCommand(osgVerse::CommandData* cmd);
    bool handleItemCommand(osgVerse::CommandData* cmd);
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

    void addModelFromUrl(const std::string& url);

protected:
    osg::ref_ptr<osgVerse::Window> _treeWindow;
    osg::ref_ptr<osgVerse::TreeView> _treeView;
    osg::ref_ptr<osgVerse::TreeView::TreeData> _camTreeData, _sceneTreeData;
};

#endif
