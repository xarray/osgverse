#include "../ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <pinyin/Pinyin.h>
using namespace osgVerse;

/// VirtualKeyboard related

int VirtualKeyboard::getCandidatePages(const std::string& input)
{
    ime::pinyin::Pinyin* pinyin = (ime::pinyin::Pinyin*)imeInterface;
    if (pinyin && pinyin->hasInit() && pinyin->search(input))
        return pinyin->getCandidatePageCount();
    else return 0;
}

bool VirtualKeyboard::getCandidates(int pageId, std::vector<std::string>& results)
{
    ime::pinyin::Pinyin* pinyin = (ime::pinyin::Pinyin*)imeInterface;
    if (pinyin && pinyin->hasInit() && pageId < (int)pinyin->getCandidatePageCount())
        pinyin->getCandidateByPage(pageId, results);
    return !results.empty();
}

void VirtualKeyboard::create(const std::string& sysCikuPath, const std::string& learnCikuPath)
{
    if (keyList.empty())
    {
        // TODO: Default keyboard
    }

    if (!sysCikuPath.empty() && !learnCikuPath.empty())
    {
        ime::pinyin::Pinyin* pinyin = new ime::pinyin::Pinyin;
        pinyin->init(sysCikuPath, learnCikuPath);
        pinyin->setCandidatePageSize(5);
        pinyin->enableAICombineCandidate(false);
        pinyin->enableAssociateCandidate(true);
        imeInterface = pinyin;
    }
}

void VirtualKeyboard::destroy()
{
    ime::pinyin::Pinyin* pinyin = (ime::pinyin::Pinyin*)imeInterface;
    if (pinyin != NULL) { pinyin->deinit(); delete pinyin; }
    imeInterface = NULL;
}

bool VirtualKeyboard::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    for (size_t i = 0; i < keyList.size(); ++i)
    {
        KeyRowData& row = keyList[i];
        // TODO
    }
    return done;
}
