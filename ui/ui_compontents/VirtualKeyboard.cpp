#include "../ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <pinyin/Pinyin.h>
using namespace osgVerse;

static void setupAlphabeticKeys(VirtualKeyboard* vk, ImGuiComponentBase* capsObj, bool caps)
{
    for (size_t i = 0; i < vk->keyList.size(); ++i)
    {
        VirtualKeyboard::KeyRowData& row = vk->keyList[i];
        for (size_t j = 0; j < row.keys.size(); ++j)
        {
            VirtualKeyboard::KeyData& kd = row.keys[j];
            Button* btn = dynamic_cast<Button*>(kd.button.get());
            if (!btn || kd.key == 0) continue;

            if (vk->capslock && !caps)
            {
                if (kd.key >= 'A' && kd.key <= 'Z')
                { kd.key = 'a' + (kd.key - 'A'); btn->name = (char)kd.key; btn->name += "##vk0"; }
            }
            else if (!vk->capslock && caps)
            {
                if (kd.key >= 'a' && kd.key <= 'z')
                { kd.key = 'A' + (kd.key - 'a'); btn->name = (char)kd.key; btn->name += "##vk0"; }
            }
        }
    }

    if (capsObj != NULL)
    {
        Button* capsButton = static_cast<Button*>(capsObj);
        capsButton->name = caps ? "CAPS LOCK##vk0" : "Caps Lock##vk0";
    }
    vk->capslock = caps;
}

static void setupShiftedKeys(VirtualKeyboard* vk, ImGuiComponentBase* shiftObj, bool shifted)
{
    for (size_t i = 0; i < vk->keyList.size(); ++i)
    {
        VirtualKeyboard::KeyRowData& row = vk->keyList[i];
        for (size_t j = 0; j < row.keys.size(); ++j)
        {
            VirtualKeyboard::KeyData& kd = row.keys[j];
            Button* btn = dynamic_cast<Button*>(kd.button.get());
            if (!btn || kd.key == 0) continue;

            if (shifted)
            {
                switch (kd.key)
                {
                case osgGA::GUIEventAdapter::KEY_Backquote: kd.key = '~'; btn->name = "~##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_1:
                    kd.key = osgGA::GUIEventAdapter::KEY_Exclaim; btn->name = "!##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_2:
                    kd.key = osgGA::GUIEventAdapter::KEY_At; btn->name = "@##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_3:
                    kd.key = osgGA::GUIEventAdapter::KEY_Hash; btn->name = " # ##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_4:
                    kd.key = osgGA::GUIEventAdapter::KEY_Dollar; btn->name = "$##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_5:
                    kd.key = osgGA::GUIEventAdapter::KEY_Period; btn->name = "%##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_6:
                    kd.key = osgGA::GUIEventAdapter::KEY_Caret; btn->name = "^##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_7:
                    kd.key = osgGA::GUIEventAdapter::KEY_Ampersand; btn->name = "&##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_8:
                    kd.key = osgGA::GUIEventAdapter::KEY_Asterisk; btn->name = "*##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_9:
                    kd.key = osgGA::GUIEventAdapter::KEY_Leftparen; btn->name = "(##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_0:
                    kd.key = osgGA::GUIEventAdapter::KEY_Rightparen; btn->name = ")##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Minus:
                    kd.key = osgGA::GUIEventAdapter::KEY_Underscore; btn->name = "_##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Equals:
                    kd.key = osgGA::GUIEventAdapter::KEY_Plus; btn->name = "+##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Leftbracket: kd.key = '{'; btn->name = "{##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Rightbracket: kd.key = '}'; btn->name = "}##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Backslash: kd.key = '|'; btn->name = "|##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Comma:
                    kd.key = osgGA::GUIEventAdapter::KEY_Less; btn->name = "<##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Colon:
                    kd.key = osgGA::GUIEventAdapter::KEY_Greater; btn->name = ">##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Slash:
                    kd.key = osgGA::GUIEventAdapter::KEY_Question; btn->name = "?##vk0"; break;
                }
            }
            else
            {
                switch (kd.key)
                {
                case '~': kd.key = osgGA::GUIEventAdapter::KEY_Backquote; btn->name = "`##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Exclaim:
                    kd.key = osgGA::GUIEventAdapter::KEY_1; btn->name = "1##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_At:
                    kd.key = osgGA::GUIEventAdapter::KEY_2; btn->name = "2##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Hash:
                    kd.key = osgGA::GUIEventAdapter::KEY_3; btn->name = "3##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Dollar:
                    kd.key = osgGA::GUIEventAdapter::KEY_4; btn->name = "4##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Period:
                    kd.key = osgGA::GUIEventAdapter::KEY_5; btn->name = "5##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Caret:
                    kd.key = osgGA::GUIEventAdapter::KEY_6; btn->name = "6##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Ampersand:
                    kd.key = osgGA::GUIEventAdapter::KEY_7; btn->name = "7##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Asterisk:
                    kd.key = osgGA::GUIEventAdapter::KEY_8; btn->name = "8##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Leftparen:
                    kd.key = osgGA::GUIEventAdapter::KEY_9; btn->name = "9##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Rightparen:
                    kd.key = osgGA::GUIEventAdapter::KEY_0; btn->name = "0##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Underscore:
                    kd.key = osgGA::GUIEventAdapter::KEY_Minus; btn->name = "-##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Plus:
                    kd.key = osgGA::GUIEventAdapter::KEY_Equals; btn->name = "=##vk0"; break;
                case '{': kd.key = osgGA::GUIEventAdapter::KEY_Leftbracket; btn->name = "[##vk0"; break;
                case '}': kd.key = osgGA::GUIEventAdapter::KEY_Rightbracket; btn->name = "]##vk0"; break;
                case '|': kd.key = osgGA::GUIEventAdapter::KEY_Backslash; btn->name = "\\##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Less:
                    kd.key = osgGA::GUIEventAdapter::KEY_Comma; btn->name = ",##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Greater:
                    kd.key = osgGA::GUIEventAdapter::KEY_Colon; btn->name = ".##vk0"; break;
                case osgGA::GUIEventAdapter::KEY_Question:
                    kd.key = osgGA::GUIEventAdapter::KEY_Slash; btn->name = "/##vk0"; break;
                }
            }
        }
    }
    vk->shifted = shifted;
}

/// VirtualKeyboard related

Button* VirtualKeyboard::KeyRowData::addKey(
    const std::string& n, int key, int modkey, const osg::Vec2& size)
{
    osg::ref_ptr<Button> button = new Button(n);
    button->size = size;

    KeyData kd(n); kd.button = button;
    kd.key = key; kd.modKey = modkey;
    keys.push_back(kd); return button.get();
}

Button* VirtualKeyboard::KeyRowData::addKeyEx(
    const std::string& n, int key, int modkey, const osg::Vec2& size,
    const ImColor& style, ActionCallback cb)
{
    osg::ref_ptr<Button> button = new Button(n);
    button->size = size; button->styleNormal = style;
    button->styleHovered = ImColor(0.5f, 0.5f, 0.8f);
    button->styleActive = ImColor(0.8f, 0.8f, 0.8f);
    button->callback = cb; button->styled = true;

    KeyData kd(n); kd.button = button;
    kd.key = key; kd.modKey = modkey;
    keys.push_back(kd); return button.get();
}

int VirtualKeyboard::getCandidatePages(const std::string& input)
{
    ime::pinyin::Pinyin* pinyin = (ime::pinyin::Pinyin*)imeInterface;
    if (pinyin && pinyin->hasInit() && pinyin->search(input))
        return pinyin->getCandidatePageCount();
    else return 0;
}

bool VirtualKeyboard::getCandidates(int pageId, std::vector<std::string>& results)
{
    ime::pinyin::Pinyin* pinyin = (ime::pinyin::Pinyin*)imeInterface; results.clear();
    if (pinyin && pinyin->hasInit() && pageId < (int)pinyin->getCandidatePageCount())
        pinyin->getCandidateByPage(pageId, results);
    return !results.empty();
}

void VirtualKeyboard::resetCandidates()
{
    imeCandicatePages = 0; currentPage = 0;
    imeCandicates.assign(5, "");
}

void VirtualKeyboard::create(const std::string& sysCikuPath, const std::string& learnCikuPath)
{
    if (keyList.empty())
    {
        // Default keyboard layout
        osg::Vec2 defSize = osg::Vec2(40.0f, 32.0f);
        if (!sysCikuPath.empty() && !learnCikuPath.empty())
        {
            KeyRowData rowC; rowC.indentX = 0.0f;
            {
                osg::ref_ptr<InputField> input = new InputField("##vk0InputResult");
                input->size = osg::Vec2(312.0f, 32.0f);
                KeyData kd("##vk0imeResult"); kd.button = input;
                rowC.keys.push_back(kd);

                rowC.addKeyEx("OK##vk0imeOK", 0, 0, osg::Vec2(48.0f, 32.0f), ImColor(0.2f, 0.2f, 0.6f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    {
                        if (msgCallback) msgCallback(result + totalInput);
                        resetCandidates(); totalInput = ""; result = "";
                    });
                rowC.addKeyEx("Clear##vk0imeClear", 0, 0, osg::Vec2(48.0f, 32.0f), ImColor(0.2f, 0.2f, 0.6f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { resetCandidates(); totalInput = ""; result = ""; });

                rowC.addKeyEx("##vk0imeC1", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.2f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { totalInput = ""; result += imeCandicates[0]; resetCandidates(); });
                rowC.addKeyEx("##vk0imeC2", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.2f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { totalInput = ""; result += imeCandicates[1]; resetCandidates(); });
                rowC.addKeyEx("##vk0imeC3", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.2f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { totalInput = ""; result += imeCandicates[2]; resetCandidates(); });
                rowC.addKeyEx("##vk0imeC4", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.2f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { totalInput = ""; result += imeCandicates[3]; resetCandidates(); });
                rowC.addKeyEx("##vk0imeC5", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.2f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    { totalInput = ""; result += imeCandicates[4]; resetCandidates(); });

                rowC.addKeyEx("<##vk0ime", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.6f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    {
                        if (currentPage > 0) currentPage--;
                        getCandidates(currentPage, imeCandicates);
                        while (imeCandicates.size() < 5) imeCandicates.push_back("");
                    });
                rowC.addKeyEx(">##vk0ime", 0, 0, defSize, ImColor(0.2f, 0.2f, 0.6f),
                    [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                    {
                        if (currentPage < imeCandicatePages - 1) currentPage++;
                        getCandidates(currentPage, imeCandicates);
                        while (imeCandicates.size() < 5) imeCandicates.push_back("");
                    });
            }

            keyList.push_back(rowC);
        }

        KeyRowData row0; row0.indentX = 0.0f;
        {
            row0.addKey("Esc##vk0", osgGA::GUIEventAdapter::KEY_Escape, 0, osg::Vec2(64.0f, 32.0f));
            row0.addKey("`##vk0", osgGA::GUIEventAdapter::KEY_Backquote, 0, defSize);
            row0.addKey("1##vk0", osgGA::GUIEventAdapter::KEY_1, 0, defSize);
            row0.addKey("2##vk0", osgGA::GUIEventAdapter::KEY_2, 0, defSize);
            row0.addKey("3##vk0", osgGA::GUIEventAdapter::KEY_3, 0, defSize);
            row0.addKey("4##vk0", osgGA::GUIEventAdapter::KEY_4, 0, defSize);
            row0.addKey("5##vk0", osgGA::GUIEventAdapter::KEY_5, 0, defSize);
            row0.addKey("6##vk0", osgGA::GUIEventAdapter::KEY_6, 0, defSize);
            row0.addKey("7##vk0", osgGA::GUIEventAdapter::KEY_7, 0, defSize);
            row0.addKey("8##vk0", osgGA::GUIEventAdapter::KEY_8, 0, defSize);
            row0.addKey("9##vk0", osgGA::GUIEventAdapter::KEY_9, 0, defSize);
            row0.addKey("0##vk0", osgGA::GUIEventAdapter::KEY_0, 0, defSize);
            row0.addKey("-##vk0", osgGA::GUIEventAdapter::KEY_Minus, 0, defSize);
            row0.addKey("=##vk0", osgGA::GUIEventAdapter::KEY_Equals, 0, defSize);
            row0.addKey("BkSp##vk0", osgGA::GUIEventAdapter::KEY_BackSpace, 0, osg::Vec2(64.0f, 32.0f));
        }
        keyList.push_back(row0);

        KeyRowData row1; row1.indentX = 0.0f;
        {
            row1.addKey("Tab##vk0", osgGA::GUIEventAdapter::KEY_Tab, 0, osg::Vec2(80.0f, 32.0f));
            row1.addKey("q##vk0", 'q', 0, defSize); row1.addKey("w##vk0", 'w', 0, defSize);
            row1.addKey("e##vk0", 'e', 0, defSize); row1.addKey("r##vk0", 'r', 0, defSize);
            row1.addKey("t##vk0", 't', 0, defSize); row1.addKey("y##vk0", 'y', 0, defSize);
            row1.addKey("u##vk0", 'u', 0, defSize); row1.addKey("i##vk0", 'i', 0, defSize);
            row1.addKey("o##vk0", 'o', 0, defSize); row1.addKey("p##vk0", 'p', 0, defSize);
            row1.addKey("[##vk0", osgGA::GUIEventAdapter::KEY_Leftbracket, 0, defSize);
            row1.addKey("]##vk0", osgGA::GUIEventAdapter::KEY_Rightbracket, 0, defSize);
            row1.addKey("\\##vk0", osgGA::GUIEventAdapter::KEY_Backslash, 0, defSize);
            row1.addKey("Del##vk0", osgGA::GUIEventAdapter::KEY_Delete, 0, osg::Vec2(48.0f, 32.0f));
        }
        keyList.push_back(row1);

        KeyRowData row2; row2.indentX = 0.0f;
        {
            row2.addKeyEx("Caps Lock##vk0", osgGA::GUIEventAdapter::KEY_Caps_Lock, 0, osg::Vec2(100.0f, 32.0f),
                ImColor(0.8f, 0.5f, 0.0f), [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                { setupAlphabeticKeys(this, me, !capslock); });
            row2.addKey("a##vk0", 'a', 0, defSize); row2.addKey("s##vk0", 's', 0, defSize);
            row2.addKey("d##vk0", 'd', 0, defSize); row2.addKey("f##vk0", 'f', 0, defSize);
            row2.addKey("g##vk0", 'g', 0, defSize); row2.addKey("h##vk0", 'h', 0, defSize);
            row2.addKey("j##vk0", 'j', 0, defSize); row2.addKey("k##vk0", 'k', 0, defSize);
            row2.addKey("l##vk0", 'l', 0, defSize);
            row2.addKey(";##vk0", osgGA::GUIEventAdapter::KEY_Semicolon, 0, defSize);
            row2.addKey("'##vk0", osgGA::GUIEventAdapter::KEY_Quote, 0, defSize);
            row2.addKey("Enter##vk0", osgGA::GUIEventAdapter::KEY_Return, 0, osg::Vec2(124.0f, 32.0f));
        }
        keyList.push_back(row2);

        KeyRowData row3; row3.indentX = 0.0f;
        {
            static Button *shiftBtn[2];
            shiftBtn[0] = row3.addKeyEx("Shift##vk0", 0, osgGA::GUIEventAdapter::MODKEY_LEFT_SHIFT,
                osg::Vec2(112.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                {
                    setupAlphabeticKeys(this, NULL, !shifted); setupShiftedKeys(this, me, !shifted); 
                    shiftBtn[0]->name = shifted ? "SHIFT##vk0" : "Shift##vk0";
                    shiftBtn[1]->name = shifted ? "SHIFT##vk1" : "Shift##vk1";
                });
            row3.addKey("z##vk0", 'z', 0, defSize); row3.addKey("x##vk0", 'x', 0, defSize);
            row3.addKey("c##vk0", 'c', 0, defSize); row3.addKey("v##vk0", 'v', 0, defSize);
            row3.addKey("b##vk0", 'b', 0, defSize); row3.addKey("n##vk0", 'n', 0, defSize);
            row3.addKey("m##vk0", 'm', 0, defSize);
            row3.addKey(",##vk0", osgGA::GUIEventAdapter::KEY_Comma, 0, defSize);
            row3.addKey(".##vk0", osgGA::GUIEventAdapter::KEY_Colon, 0, defSize);
            row3.addKey("/##vk0", osgGA::GUIEventAdapter::KEY_Slash, 0, defSize);
            shiftBtn[1] = row3.addKeyEx("Shift##vk1", 0, osgGA::GUIEventAdapter::MODKEY_RIGHT_SHIFT,
                osg::Vec2(112.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                {
                    setupAlphabeticKeys(this, NULL, !shifted); setupShiftedKeys(this, me, !shifted); 
                    shiftBtn[0]->name = shifted ? "SHIFT##vk0" : "Shift##vk0";
                    shiftBtn[1]->name = shifted ? "SHIFT##vk1" : "Shift##vk1";
                });
            row3.addKey("PgUp##vk0", osgGA::GUIEventAdapter::KEY_Page_Up, 0, defSize);
        }
        keyList.push_back(row3);

        KeyRowData row4; row4.indentX = 0.0f;
        {
            static Button *ctrlBtn[2], *altBtn[2];
            ctrlBtn[0] = row4.addKeyEx("Ctrl##vk0", 0, osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL,
                osg::Vec2(64.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                {
                    ctrled = !ctrled; ctrlBtn[0]->name = ctrled ? "CTRL##vk0" : "Ctrl##vk0";
                    ctrlBtn[1]->name = ctrled ? "CTRL##vk1" : "Ctrl##vk1";
                });
            altBtn[0] = row4.addKeyEx("Alt##vk0", 0, osgGA::GUIEventAdapter::MODKEY_LEFT_ALT,
                osg::Vec2(64.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                {
                    alted = !alted; altBtn[0]->name = alted ? "ALT##vk0" : "Alt##vk0";
                    altBtn[1]->name = alted ? "ALT##vk1" : "Alt##vk1";
                });
            row4.addKey(" ##vk0", osgGA::GUIEventAdapter::KEY_Space, 0, osg::Vec2(232.0f, 32.0f));
            altBtn[1] = row4.addKeyEx("Alt##vk1", 0, osgGA::GUIEventAdapter::MODKEY_RIGHT_ALT,
                osg::Vec2(64.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                {
                    alted = !alted; altBtn[0]->name = alted ? "ALT##vk0" : "Alt##vk0";
                    altBtn[1]->name = alted ? "ALT##vk1" : "Alt##vk1";
                });
            ctrlBtn[1] = row4.addKeyEx("Ctrl##vk1", 0, osgGA::GUIEventAdapter::MODKEY_RIGHT_CTRL,
                osg::Vec2(64.0f, 32.0f), ImColor(0.8f, 0.5f, 0.0f),
                [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
                {
                    ctrled = !ctrled; ctrlBtn[0]->name = ctrled ? "CTRL##vk0" : "Ctrl##vk0";
                    ctrlBtn[1]->name = ctrled ? "CTRL##vk1" : "Ctrl##vk1";
                });
            row4.addKeyEx("En##vk0", 0, 0, defSize,
                ImColor(0.8f, 0.5f, 0.0f), [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
                {
                    chsMode = !chsMode; resetCandidates();
                    (static_cast<Button*>(me))->name = chsMode ? u8"жа##vk0" : "En##vk0";
                });
            row4.addKey("Home##vk0", osgGA::GUIEventAdapter::KEY_Home, 0, defSize);
            row4.addKey("End##vk0", osgGA::GUIEventAdapter::KEY_End, 0, defSize);
            row4.addKey("Ins##vk0", osgGA::GUIEventAdapter::KEY_Insert, 0, defSize);
            row4.addKey("PgDn##vk0", osgGA::GUIEventAdapter::KEY_Page_Down, 0, defSize);
        }
        keyList.push_back(row4);
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
    bool done = false, backspaced = false, newCh = false; lastInput = "";
    for (size_t i = 0; i < keyList.size(); ++i)
    {
        KeyRowData& row = keyList[i];
        VirtualKeyboard::adjustLine(true, false, row.indentX);
        for (size_t j = 0; j < row.keys.size(); ++j)
        {
            KeyData& kd = row.keys[j];
            if (kd.name.find("##vk0ime") != std::string::npos)
            {
                if (kd.name.find("Result") != std::string::npos)
                {
                    InputField* in = static_cast<InputField*>(kd.button.get());
                    if (in) in->value = result + totalInput;
                }
                else
                {
                    Button* btn = static_cast<Button*>(kd.button.get());
                    if (kd.name.find("C1") != std::string::npos) btn->name = imeCandicates[0] + "##vk0imeC1";
                    else if (kd.name.find("C2") != std::string::npos) btn->name = imeCandicates[1] + "##vk0imeC2";
                    else if (kd.name.find("C3") != std::string::npos) btn->name = imeCandicates[2] + "##vk0imeC3";
                    else if (kd.name.find("C4") != std::string::npos) btn->name = imeCandicates[3] + "##vk0imeC4";
                    else if (kd.name.find("C5") != std::string::npos) btn->name = imeCandicates[4] + "##vk0imeC5";
                }
                done = kd.button->show(mgr, content);
            }
            else if (kd.button->show(mgr, content))
            {
                if (kd.key >= osgGA::GUIEventAdapter::KEY_Space &&
                    kd.key <= osgGA::GUIEventAdapter::KEY_Backquote) { lastInput += kd.key; }
                else if (kd.key >= 'a' && kd.key <= 'z') { lastInput += kd.key; }
                else if (kd.key >= 'A' && kd.key <= 'Z') { lastInput += kd.key; }
                else if (kd.key == osgGA::GUIEventAdapter::KEY_BackSpace) backspaced = true;
                if (keyCallback) keyCallback(mgr, content, kd); done = true;
            }
            if (j < row.keys.size() - 1) ImGui::SameLine();
        }
    }

    if (backspaced && !totalInput.empty()) { totalInput.pop_back(); newCh = true; }
    else if (!lastInput.empty()) { totalInput += lastInput; newCh = true; }
    if (newCh)
    {
        if (chsMode)
        {
            imeCandicatePages = getCandidatePages(totalInput);
            currentPage = 0; getCandidates(currentPage, imeCandicates);
            while (imeCandicates.size() < 5) imeCandicates.push_back("");
        }
        else { result += totalInput; totalInput = ""; }
    }
    return done;
}
