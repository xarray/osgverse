#ifndef MANA_UI_UTILITIES_HPP
#define MANA_UI_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>
#include <osgGA/EventQueue>
#include <unordered_map>
#include <string>

namespace osgVerse
{

    /** Keyboard state buffering manager */
    class KeyboardCacher : public osg::Referenced
    {
    public:
        static KeyboardCacher* instance();
        void advance(const osgGA::GUIEventAdapter& ea);

        bool isKeyDown(int key) const;
        bool anyKeyDown(std::initializer_list<int> keys) const;
        bool allKeyDown(std::initializer_list<int> keys) const;

    protected:
        KeyboardCacher() {}
        std::unordered_map<int, bool> _keyStates;
    };

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
