extern "C"
{
#   include <3rdparty/tinyfiledialogs.h>
}
#include "Utilities.h"
using namespace osgVerse;

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

void FileDialog::notify(NotifyLevel n, const std::string& title, const std::string& msg,
                        ButtonGroup g, int defaultBtn)
{ tinyfd_messageBox(title.c_str(), msg.c_str(), obtainButtonType(g), obtainIconType(n), defaultBtn); }

std::string FileDialog::selectFolder(const std::string& title, const std::string& defPath)
{
    char* result = tinyfd_selectFolderDialog(title.c_str(), defPath.c_str());
    return result ? std::string(result) : "";
}

