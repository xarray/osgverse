extern "C"
{
#   include <3rdparty/tinyfiledialogs.h>
}
#include "Utilities.h"
#include <iostream>
using namespace osgVerse;

/// KeyboardCacher ///
KeyboardCacher* KeyboardCacher::instance()
{
    static osg::ref_ptr<KeyboardCacher> s_instance = new KeyboardCacher;
    return s_instance.get();
}

void KeyboardCacher::advance(const osgGA::GUIEventAdapter& ea)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
    {
        int key = ea.getKey(); if (_keyStates[key]) return;
        _keyStates[key] = true;
    }
    else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
    {
        int key = ea.getKey(); if (!_keyStates[key]) return;
        _keyStates[key] = false;
    }
}

bool KeyboardCacher::isKeyDown(int key) const
{
    std::unordered_map<int, bool>::const_iterator it = _keyStates.find(key);
    if (it == _keyStates.end()) return false; else return it->second;
}

bool KeyboardCacher::anyKeyDown(std::initializer_list<int> keys) const
{
    for (int k : keys) { if (isKeyDown(k)) return true; }
    return false;
}

bool KeyboardCacher::allKeyDown(std::initializer_list<int> keys) const
{
    for (int k : keys) { if (!isKeyDown(k)) return false; }
    return true;
}

/// FileDialog ///
namespace
{
    static const char* obtainIconType(FileDialog::NotifyLevel n)
    {
        switch (n)
        {
        case FileDialog::Warn: return "warning";
        case FileDialog::Error: return "error";
        default: return "info";
        }
    }

    static const char* obtainButtonType(FileDialog::ButtonGroup g)
    {
        switch (g)
        {
        case FileDialog::OkCancel: return "okcancel";
        case FileDialog::YesNo: return "yesno";
        case FileDialog::YesNoCancel: return "yesnocancel";
        default: return "ok";
        }
    }
}

void FileDialog::notify(NotifyLevel n, const std::string& title, const std::string& msg,
                        ButtonGroup g, int defaultBtn)
{ tinyfd_messageBox(title.c_str(), msg.c_str(), obtainButtonType(g), obtainIconType(n), defaultBtn); }

std::string FileDialog::selectFolder(const std::string& title, const std::string& defPath)
{
    char* result = tinyfd_selectFolderDialog(title.c_str(), defPath.c_str());
    return result ? std::string(result) : "";
}

