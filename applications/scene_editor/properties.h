#ifndef MANA_PROPERTIES_HPP
#define MANA_PROPERTIES_HPP

#include "defines.h"

namespace osgVerse
{
    struct CommandData;
    class PropertyItem;
}

class Properties : public osgVerse::ImGuiComponentBase
{
public:
    Properties(EditorContentHandler* ech);
    osgVerse::Window* getWindow() { return _propWindow.get(); }

    bool handleCommand(osgVerse::CommandData* cmd);
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);
    void showPopupMenu(osgVerse::PropertyItem* item, osgVerse::ImGuiManager* mgr,
                       osgVerse::ImGuiContentHandler* content);

protected:
    osg::ref_ptr<osgVerse::Window> _propWindow;
    std::vector<osg::ref_ptr<osgVerse::PropertyItem>> _properties;
    std::vector<osgVerse::MenuBarBase::MenuItemData> _popupMenus;
    int _selectedProperty;
};

#endif
