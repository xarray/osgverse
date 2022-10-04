#ifndef MANA_SCENELOGIC_HPP
#define MANA_SCENELOGIC_HPP

#include "defines.h"

class SceneLogic : public osgVerse::ImGuiComponentBase
{
public:
    SceneLogic();
    osgVerse::Window* getWindow() { return _logicWindow.get(); }
    virtual bool show(osgVerse::ImGuiManager* mgr, osgVerse::ImGuiContentHandler* content);

    struct NotifyData
    {
        int level; std::string dateTime, text;
        NotifyData(int l, const std::string& t) : level(l), text(t) {}
    };
    void addNotify(const NotifyData& nd) { _notifyData.push_back(nd); }

protected:
    std::vector<NotifyData> _notifyData;
    osg::ref_ptr<osgVerse::Window> _logicWindow;
};

#endif
