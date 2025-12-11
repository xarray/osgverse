#ifndef MANA_UI_UTILITIES_HPP
#define MANA_UI_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#include <string>

namespace osgVerse
{

    /** File and message dialog object */
    class FileDialog : public osg::Referenced
    {
    public:
        enum NotifyLevel { Info, Warn, Error };
        enum ButtonGroup { Ok, OkCancel, YesNo, YesNoCancel };

        static void notify(NotifyLevel n, const std::string& title, const std::string& msg,
                           ButtonGroup group, int defaultBtn = 0);
        static std::string selectFolder(const std::string& title, const std::string& defPath = "");
    };

}

#endif
