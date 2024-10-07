#ifndef MANA_UI_IMGUICOMPONENTS_HPP
#define MANA_UI_IMGUICOMPONENTS_HPP

#define IMGUI_DEFINE_MATH_OPERATORS
#include <osg/Version>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include "3rdparty/any.hpp"
#include "3rdparty/imgui/imgui.h"
#include "3rdparty/imgui/ImGuizmo.h"
#include "pipeline/Global.h"
#include "ImGui.h"
#include <string>
#include <map>

namespace osgVerse
{
    /// Variable 'name' is required as unique, may add '##' to hide the display
    /** Components:
        - Window
        - Label
        - Button
        - ImageButton
        - CheckBox
        - ComboBox
        - RadioButtonGroup
        - InputField
        - InputValueField
        - InputVectorField (V2, V3, V4, Color)
        - Slider (H, V, Knob)
        - MenuBar
        - ListView
        - TreeView
        - Timeline
        - CollapsingHeader: ImGui::CollapsingHeader()
        - Tab: ImGui::BeginTabBar(), ImGui::BeginTabItem(), ImGui::EndTabItem(), ImGui::EndTabBar()
        - Popup: ImGui::BeginPopup(), ImGui::EndPopup(); ImGui::OpenPopup()
        - Table: ImGui::BeginTable(), ImGui::TableNextRow(), ImGui::TableNextColumn(), ImGui::EndTable()
        - MainMenu: ImGui::BeginMainMenuBar(), ImGui::EndMainMenuBar()

        TODO
        // ImSequencer: make it usable
        // Code editor
        // Node editor
        // ImGuiFileDialog utf8 problem...
        // Input component need IME
        // Multi-language support
     */

    struct ImGuiComponentBase : public osg::Referenced
    {
        typedef std::function<void (ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)> ActionCallback;
        typedef std::function<void (ImGuiManager*, ImGuiContentHandler*,
                                    ImGuiComponentBase*, const std::string&)> ActionCallback2;
        typedef std::function<void (const std::string&)> FileDialogCallback;
        typedef std::function<void(bool)> ConfirmDialogCallback;
        
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content) = 0;
        virtual void showEnd() { /* nothing to do by default */ }
        virtual void showTooltip(const std::string& desc, const std::string& t = "(?)", float wrapPos = 10.0f);
        osg::ref_ptr<osg::Referenced> userData;

        static std::string TR(const std::string& s);  // multi-language support
        static void setWidth(float width, bool fromLeft = true);
        static void adjustLine(bool newLine, bool sep = false, float indentX = 0.0f, float indentY = 0.0f);

        static void registerFileDialog(
            FileDialogCallback cb, const std::string& name, const std::string& title, bool modal,
            const std::string& dir = ".", const std::string& filters=".*");
        static bool showFileDialog(std::string& result);
        static struct FileDialogData
        {
            std::string name;
            FileDialogCallback callback;
        } s_fileDialogRunner;

        static void registerConfirmDialog(
            ConfirmDialogCallback cb, const std::string& name, const std::string& title, bool modal,
            const std::string& btn0 = "OK", const std::string& btn1 = "");
        static bool showConfirmDialog(bool& result);
        static struct ConfirmDialogData
        {
            std::string name, title, btn0, btn1; bool modal, init;
            ConfirmDialogCallback callback;
        } s_confirmDialogRunner;
    };

    struct Window : public ImGuiComponentBase
    {
        std::string name; float alpha;
        osg::Vec2 pos, pivot, size; osg::Vec4 rectRT;  // [0, 1]
        bool isOpen, collapsed, useMenuBar, sizeApplied;
        ImGuiWindowFlags flags;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        virtual void showEnd() { ImGui::End(); }
        void resize(const osg::Vec2& p, const osg::Vec2& s);
        osg::Vec4 getCurrentRectangle() const { return rectRT; }
        Window(const std::string& n)
        :   name(n), alpha(1.0f), isOpen(true), collapsed(false),
            useMenuBar(false), sizeApplied(false), flags(0) {}
    };

    struct Label : public ImGuiComponentBase
    {
        struct TextData
        {
            bool useBullet, disabled, wrapped; osg::Vec3 color;
            TextData(bool d = false) : useBullet(false), disabled(d), wrapped(true) {}
        };
        std::vector<TextData> attributes;
        std::vector<std::string> texts;
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
    };

    struct Button : public ImGuiComponentBase
    {
        std::string name, tooltip;
        osg::Vec2 size; bool repeatable, styled;
        ImColor styleNormal, styleHovered, styleActive;
        ActionCallback callback;
        
        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        Button(const std::string& n)
            : name(n), repeatable(false), styled(false), callback(ActionCallback()) {}
    };

    struct ImageButton : public ImGuiComponentBase
    {
        std::string name, tooltip;
        osg::Vec2 size, uv0, uv1; bool imageOnly;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        ImageButton(const std::string& n)
            : name(n), uv1(1.0f, 1.0f), imageOnly(false), callback(ActionCallback()) {}
    };

    struct CheckBox : public ImGuiComponentBase
    {
        std::string name, tooltip; bool value;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        CheckBox(const std::string& n, bool v) : name(n), value(v), callback(ActionCallback()) {}
    };

    struct ComboBox : public ImGuiComponentBase
    {
        std::vector<std::string> items;
        std::string name, tooltip; int index;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        ComboBox(const std::string& n) : name(n), index(0), callback(ActionCallback()) {}
    };

    struct RadioButtonGroup : public ImGuiComponentBase
    {
        struct RadioData { std::string name, tooltip; };
        std::vector<RadioData> buttons;
        int value; bool inSameLine;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        RadioButtonGroup() : value(0), inSameLine(true), callback(ActionCallback()) {}
    };

    struct InputField : public ImGuiComponentBase
    {
        std::string name, value, tooltip, placeholder;
        ImGuiInputTextFlags flags; osg::Vec2 size;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        InputField(const std::string& n) : name(n), flags(0), callback(ActionCallback()) {}
    };

    struct InputValueField : public ImGuiComponentBase
    {
        enum Type { IntValue, UIntValue, FloatValue, DoubleValue } type;
        std::string name, format, tooltip;
        double value, minValue, maxValue, step;
        ImGuiInputTextFlags flags;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        InputValueField(const std::string& n)
        :   type(DoubleValue), name(n), value(0),
            minValue(0), maxValue(0), step(1), flags(0), callback(ActionCallback()) {}
    };

    struct InputVectorField : public InputValueField
    {
        osg::Vec4d vecValue;
        int vecNumber; bool asColor;
        // flags = ImGuiColorEditFlags

        template<typename T> void setVector(const T& vec)
        {
            int num = std::min<int>(vec.num_components, vecNumber);
            for (int i = 0; i < num; ++i) vecValue[i] = (double)vec[i];
        }

        template<typename T> void getVector(T& vec) const
        {
            int num = std::min<int>(vec.num_components, vecNumber);
            for (int i = 0; i < num; ++i) 
                vec[i] = static_cast<typename T::value_type>(vecValue[i]);
        }

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        InputVectorField(const std::string& n)
            : InputValueField(n), vecNumber(4), asColor(false) { step = 0; }
    };

    struct Slider : public ImGuiComponentBase
    {
        enum Type { IntValue, FloatValue } type;
        enum Shape { Horizontal, Vertical, Knob } shape;
        std::string name, format, tooltip;
        double value, minValue, maxValue; osg::Vec2 size;
        ImGuiSliderFlags flags;  // or ImGuiKnobFlags
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        Slider(const std::string& n)
        :   type(IntValue), shape(Horizontal), name(n), value(0), minValue(0), maxValue(100),
            size(24, 120), flags(0), callback(ActionCallback()) {}
    };

    struct MenuBarBase : public ImGuiComponentBase
    {
        struct MenuItemData
        {
            bool enabled, selected, checkable;
            std::string name, shortcut, tooltip;
            ActionCallback callback;
            std::vector<MenuItemData> subItems;

            static MenuItemData separator;
            MenuItemData(const std::string& n)
            :   name(n), enabled(true), selected(false),
                checkable(false), callback(ActionCallback()) {}
        };

        struct MenuData
        {
            bool enabled; std::string name;
            std::vector<MenuItemData> items;
            MenuData(const std::string& n) : name(n), enabled(true) {}
        };

        bool findItemByName(const std::string& name, const MenuItemData& parent, MenuItemData& item);
        bool findItemByName(const std::string& name, MenuData& parent, MenuItemData& item);
        ActionCallback getItemCallback(const std::string& name);

        void showMenuItem(MenuItemData& mid, ImGuiManager* mgr, ImGuiContentHandler* content);
        void showMenu(ImGuiManager* mgr, ImGuiContentHandler* content);
        std::vector<MenuData> menuDataList;
    };

    struct MenuBar : public MenuBarBase
    { virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content); };
    struct MainMenuBar : public MenuBarBase
    { virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content); };

    struct ListView : public ImGuiComponentBase
    {
        struct ListData : public osg::Referenced
        {
            std::string name;
            osg::ref_ptr<osg::Referenced> userData;
        };
        std::vector<osg::ref_ptr<ListData>> items;
        std::string name, tooltip;
        int index, rows;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        ListView(const std::string& n) : name(n), index(0), rows(5), callback(ActionCallback()) {}
    };

    struct TreeView : public ImGuiComponentBase
    {
        struct TreeData : public osg::Referenced
        {
            ImGuiTreeNodeFlags flags; unsigned int color;
            std::string id, name, state, tooltip;
            std::vector<osg::ref_ptr<TreeData>> children;
            osg::ref_ptr<osg::Referenced> userData;
            TreeData() : flags(ImGuiTreeNodeFlags_DefaultOpen | /*ImGuiTreeNodeFlags_Framed |*/
                               ImGuiTreeNodeFlags_OpenOnArrow), color(0xffffffff) {}
        };
        std::vector<osg::ref_ptr<TreeData>> treeDataList;
        std::string selectedItemID;
        ActionCallback2 callback, callbackR, callbackSB;

        std::vector<TreeData*> findParents(TreeData* child) const;
        std::vector<TreeData*> findByName(const std::string& name) const;
        std::vector<TreeData*> findByUserData(osg::Referenced* ud) const;
        TreeData* findByID(const std::string& id) const;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void showRecursively(TreeData& td, ImGuiManager*, ImGuiContentHandler*);
        TreeView() : callback(ActionCallback2()) {}
    };

    struct SpiderEditor : public ImGuiComponentBase
    {
        struct NodeItem;
        struct LinkItem;

        struct PinItem : public osg::Referenced
        {
            int nodeId; std::string name; linb::any value;
            PinItem() : nodeId(-1) {}  // name = data type
        };

        struct NodeItem : public osg::Referenced
        {
            int id; std::string name;  // name = object property/method
            std::map<int, osg::ref_ptr<PinItem>> inPins, outPins;
            std::map<int, osg::observer_ptr<LinkItem>> outLinks;
            osg::observer_ptr<osg::Object> owner;

            int findPin(const std::string& n, bool isOut) const;
            NodeItem(osg::Object* obj = NULL) : id(-1), owner(obj) {}
        };

        struct LinkItem : public osg::Referenced
        {
            int id, inPin, outPin; osg::Vec4 color;
            osg::observer_ptr<NodeItem> inNode, outNode;
            LinkItem() : id(-1), inPin(-1), outPin(-1), color(1.0f, 1.0f, 1.0f, 1.0f) {}
        };

        std::map<int, osg::ref_ptr<NodeItem>> nodes;
        std::map<int, osg::ref_ptr<LinkItem>> links;
        std::string name, config; osg::Vec2 size;
        void* editorContext;

        NodeItem* createNode(const std::string& n);
        PinItem* createPin(NodeItem* it, const std::string& n, bool isOut);
        LinkItem* createLink(NodeItem* src, const std::string& srcPin,
                             NodeItem* dst, const std::string& dstPin);
        void createEditor(const std::string& cfg);
        void destroyEditor();

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void showPin(ImGuiManager* mgr, ImGuiContentHandler* content, PinItem* pin);
        SpiderEditor(const std::string& n) : name(n), editorContext(NULL) {}
    };

    struct Timeline : public ImGuiComponentBase
    {
        struct SequenceItem : public osg::Referenced
        {
            osg::Vec2i range;
            int type; bool expanded;
            std::string name;
            osg::ref_ptr<osg::Referenced> userData;

            SequenceItem() : type(0), expanded(false) {}
            SequenceItem(const std::string& n, int t, int s, int e)
                : range(s, e), type(t), expanded(false), name(n) {}
        };
        std::vector<osg::ref_ptr<SequenceItem>> items;

        osg::Vec2i frameRange;
        int firstFrame, currentFrame, selectedIndex, flags;
        bool expanded; void* seqInterface;
        ActionCallback callback;

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        Timeline()
        :   frameRange(0, 100), firstFrame(0), currentFrame(0), selectedIndex(-1),
            flags(0), expanded(true), seqInterface(NULL), callback(ActionCallback()) {}
    };

    struct VirtualKeyboard : public ImGuiComponentBase
    {
        struct KeyData
        {
            osg::ref_ptr<ImGuiComponentBase> button;
            std::string name; int key, modKey, extraCode;
            KeyData(const std::string& n = "") : name(n), key(0), modKey(0), extraCode(0) {}
        };

        struct KeyRowData
        {
            Button* addKey(const std::string& n, int key, int modkey, const osg::Vec2& size);
            Button* addKeyEx(const std::string& n, int key, int modkey, const osg::Vec2& size,
                             const ImColor& style, ActionCallback cb);
            std::vector<KeyData> keys; float indentX;
        };
        std::vector<KeyRowData> keyList;
        bool capslock, shifted, ctrled, alted, chsMode;
        std::string lastInput, totalInput, result;
        int imeCandicatePages, currentPage;
        std::vector<std::string> imeCandicates;
        void* imeInterface;

        typedef std::function<void(ImGuiManager*, ImGuiContentHandler*, const KeyData&)> KeyCallback;
        typedef std::function<void(const std::string&)> MessageCallback;
        KeyCallback keyCallback; MessageCallback msgCallback;

        int getCandidatePages(const std::string& input);
        bool getCandidates(int pageId, std::vector<std::string>& results);
        void resetCandidates(); void destroy();

        virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content);
        void create(const std::string& sysCikuPath, const std::string& learnCikuPath);
        virtual ~VirtualKeyboard() { destroy(); }
        VirtualKeyboard() : capslock(false), shifted(false), ctrled(false), alted(false), chsMode(false),
                            imeCandicatePages(0), currentPage(0), imeInterface(NULL),
                            keyCallback(KeyCallback()), msgCallback(MessageCallback()) { resetCandidates(); }
    };
}

#endif
