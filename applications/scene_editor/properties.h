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
    Properties();
    bool handleCommand(osgVerse::CommandData* cmd);
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

protected:
    osg::ref_ptr<osgVerse::Window> _propWindow;
    std::vector<osg::ref_ptr<osgVerse::PropertyItem>> _properties;
};

#endif
