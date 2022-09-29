#ifndef MANA_SCENELOGIC_HPP
#define MANA_SCENELOGIC_HPP

#include "defines.h"

class SceneLogic : public osgVerse::ImGuiComponentBase
{
public:
    SceneLogic();
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

protected:
    osg::ref_ptr<osgVerse::Window> _logicWindow;
};

#endif
