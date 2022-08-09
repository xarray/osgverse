#include "ImGuiComponents.h"
#include <imgui/imgui-knobs.h>
#include <imgui/ImGuiFileDialog.h>
#include <imgui/ImSequencer.h>
#include <imgui/node-editor/imgui_node_editor.h>
using namespace osgVerse;

void ImGuiComponentBase::setWidth(float width, bool fromLeft)
{
    if (width == 0.0f) width = FLT_MAX;
    if (!fromLeft) width = -width;
    ImGui::SetNextItemWidth(width);
}

void ImGuiComponentBase::adjustLine(bool newLine, bool sep, float indentX, float indentY)
{
    if (indentX > 0.0f && indentY > 0.0f)
        ImGui::Dummy(ImVec2(indentX, indentY));
    else if (newLine)
    {
        if (indentX > 0.0f) ImGui::Indent(indentX);
        else if (indentX < 0.0f) ImGui::Unindent(-indentX);
        else ImGui::Spacing();
    }
    else
        ImGui::SameLine(indentX);
    if (sep) ImGui::Separator();
}

void ImGuiComponentBase::showTooltip(const std::string& desc, float wrapPos)
{
    ImGui::SameLine(); ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * wrapPos);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ImGuiComponentBase::openFileDialog(const std::string& name, const std::string& title,
                                        const std::string& dir, const std::string& filters)
{
    ImGuiFileDialog::Instance()->OpenDialog(name, title, filters.c_str(), dir);
}

bool ImGuiComponentBase::showFileDialog(const std::string& name, std::string& result)
{
    if (ImGuiFileDialog::Instance()->Display(name))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            result = ImGuiFileDialog::Instance()->GetFilePathName();
        ImGuiFileDialog::Instance()->Close();
        return !result.empty();
    }
    return false;
}

bool Window::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    const ImGuiViewport* view = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(view->WorkPos.x + pos[0], view->WorkPos.y + pos[1]),
                            0, ImVec2(pivot[0], pivot[1]));
    if (sizeMin.length2() > 0.0f && sizeMax.length2() > 0.0f)
        ImGui::SetNextWindowSizeConstraints(ImVec2(sizeMin[0], sizeMin[1]), ImVec2(sizeMax[0], sizeMax[1]));
    else if (sizeMin.length2() > 0.0f)
        ImGui::SetNextWindowSize(ImVec2(sizeMin[0], sizeMin[1]));

    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::SetNextWindowCollapsed(collapsed);
    if (useMenuBar) flags |= ImGuiWindowFlags_MenuBar;
    return ImGui::Begin(name.c_str(), &isOpen, flags);
}

bool Label::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    size_t attrSize = attributes.size();
    for (size_t i = 0; i < texts.size(); ++i)
    {
        const std::string& t = texts[i];
        if (i < attrSize)
        {
            TextData& td = attributes[i];
            bool colored = td.color.length2() > 0.0f;
            if (td.useBullet) ImGui::Bullet();

            if (colored)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(td.color[0], td.color[1], td.color[2], 1.0f));
            if (td.wrapped) ImGui::TextWrapped(t.c_str());
            else if (td.disabled) ImGui::TextDisabled(t.c_str());
            else ImGui::Text(t.c_str());
            if (colored) ImGui::PopStyleColor();
        }
        else
            ImGui::Text(t.c_str());
    }
    return true;
}

bool Button::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    if (repeatable) ImGui::PushButtonRepeat(true);
    if (styled)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)styleNormal);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)styleHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)styleActive);
        done = ImGui::Button(name.c_str(), ImVec2(size[0], size[1]));
        ImGui::PopStyleColor(3);
    }
    else
        done = ImGui::Button(name.c_str(), ImVec2(size[0], size[1]));
    
    if (repeatable) ImGui::PopButtonRepeat();
    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool ImageButton::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    ImTextureID img = content->ImGuiTextures[name];
    if (imageOnly)
        ImGui::Image(img, ImVec2(size[0], size[1]), ImVec2(uv0[0], uv0[1]), ImVec2(uv1[0], uv1[1]));
    else
        done = ImGui::ImageButton(img, ImVec2(size[0], size[1]),
                                  ImVec2(uv0[0], uv0[1]), ImVec2(uv1[0], uv1[1]));

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool CheckBox::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = ImGui::CheckboxFlags(name.c_str(), &value, 0xffffffff);
    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool ComboBox::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    std::vector<const char*> itemValues(items.size());
    for (size_t i = 0; i < items.size(); ++i) itemValues[i] = items[i].c_str();
    
    bool done = false;
    if (itemValues.empty())
        done = ImGui::Combo(name.c_str(), &index, (const char**)NULL, 0);
    else
        done = ImGui::Combo(name.c_str(), &index, &itemValues[0], (int)items.size());
    
    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool RadioButtonGroup::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    int lastValue = value;
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        RadioData& td = buttons[i];
        ImGui::RadioButton(td.name.c_str(), &value, i);
        if (inSameLine && i < buttons.size() - 1) ImGui::SameLine();
        if (!td.tooltip.empty()) showTooltip(td.tooltip);
    }

    if (lastValue != value) (*callback)(mgr, content, this);
    return lastValue != value;
}

bool InputField::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false; size_t size = value.size() + 10;
    if (size < 128) size = 128; value.resize(size);
    if (placeholder.empty())
        done = ImGui::InputText(name.c_str(), value.data(), size, flags);
    else
        done = ImGui::InputTextWithHint(name.c_str(), placeholder.c_str(),
                                        value.data(), size, flags);

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool InputValueField::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    switch (type)
    {
    case IntValue:
        {
            int valueI = (int)value;
            done = ImGui::InputInt(name.c_str(), &valueI, (int)step, (int)step * 5, flags);
            if (minValue < maxValue) value = osg::clampBetween((double)valueI, minValue, maxValue);
        }
        break;
    case FloatValue:
        {
            float valueF = (float)value;
            if (format.empty()) format = "%.5f";
            done = ImGui::InputFloat(name.c_str(), &valueF, (float)step, (float)step * 5,
                                     format.c_str(), flags);
            if (minValue < maxValue) value = osg::clampBetween((double)valueF, minValue, maxValue);
        }
        break;
    case DoubleValue:
        if (format.empty()) format = "%.5f";
        done = ImGui::InputDouble(name.c_str(), &value, step, step * 5, format.c_str(), flags);
        if (minValue < maxValue) value = osg::clampBetween(value, minValue, maxValue);
        break;
    }

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool InputVectorField::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    if (asColor)
    {
        float valueF[4] = { (float)vecValue[0], (float)vecValue[1],
                            (float)vecValue[2], (float)vecValue[3] };
        done = ImGui::ColorEdit4(name.c_str(), valueF, (ImGuiColorEditFlags)flags);
        vecValue.set((double)valueF[0], (double)valueF[1], (double)valueF[2], (double)valueF[3]);

        if (!tooltip.empty()) showTooltip(tooltip);
        if (done && callback) (*callback)(mgr, content, this);
        return done;
    }

    switch (type)
    {
    case IntValue:
        {
            int valueI[4] = { (int)vecValue[0], (int)vecValue[1],
                              (int)vecValue[2], (int)vecValue[3] };
            done = ImGui::InputScalarN(name.c_str(), ImGuiDataType_S32, valueI, vecNumber,
                                       (step > 0 ? &step : NULL), NULL, "%d", flags);
            vecValue.set((double)valueI[0], (double)valueI[1], (double)valueI[2], (double)valueI[3]);
        }
        break;
    case FloatValue:
        {
            float valueF[4] = { (float)vecValue[0], (float)vecValue[1],
                                (float)vecValue[2], (float)vecValue[3] };
            if (format.empty()) format = "%.5f";
            done = ImGui::InputScalarN(name.c_str(), ImGuiDataType_Float, valueF, vecNumber,
                                       (step > 0 ? &step : NULL), NULL, format.c_str(), flags);
            vecValue.set((double)valueF[0], (double)valueF[1], (double)valueF[2], (double)valueF[3]);
        }
        break;
    case DoubleValue:
        if (format.empty()) format = "%.5f";
        done = ImGui::InputScalarN(name.c_str(), ImGuiDataType_Double, vecValue.ptr(), vecNumber,
                                   (step > 0 ? &step : NULL), NULL, format.c_str(), flags);
        break;
    }

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool Slider::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    switch (type)
    {
    case IntValue:
        {
            int valueI = (int)value;
            if (format.empty()) format = "%d";
            if (shape == Knob)
                done = ImGuiKnobs::KnobInt(name.c_str(), &valueI, (int)minValue, (int)maxValue,
                                           0.0f, format.c_str(), 1, size[1], flags);
            else if (shape == Vertical)
                done = ImGui::VSliderInt(name.c_str(), ImVec2(size[0], size[1]), &valueI,
                                         (int)minValue, (int)maxValue, format.c_str(), flags);
            else
                done = ImGui::SliderInt(name.c_str(), &valueI, (int)minValue, (int)maxValue,
                                        format.c_str(), flags);
            value = (double)valueI;
        }
        break;
    case FloatValue:
        {
            float valueF = (float)value;
            if (format.empty()) format = "%.3f";
            if (shape == Knob)
                done = ImGuiKnobs::Knob(name.c_str(), &valueF, (float)minValue, (float)maxValue,
                                        0.0f, format.c_str(), 1, size[1], flags);
            else if (shape == Vertical)
                done = ImGui::VSliderFloat(name.c_str(), ImVec2(size[0], size[1]), &valueF,
                                          (float)minValue, (float)maxValue, format.c_str(), flags);
            else
                done = ImGui::SliderFloat(name.c_str(), &valueF, (float)minValue, (float)maxValue,
                                          format.c_str(), flags);
            value = (double)valueF;
        }
        break;
    }

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool MenuBar::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool began = ImGui::BeginMenuBar();
    if (began)  // parent window must have ImGuiWindowFlags_MenuBar flag
    {
        for (size_t i = 0; i < menuDataList.size(); ++i)
        {
            MenuData& md = menuDataList[i];
            if (ImGui::BeginMenu(md.name.c_str(), md.enabled))
            {
                for (size_t j = 0; j < md.items.size(); ++j)
                {
                    MenuItemData& mid = md.items[j];
                    bool selected = mid.selected;
                    ImGui::MenuItem(
                        mid.name.c_str(), (mid.shortcut.empty() ? NULL : mid.shortcut.c_str()),
                        &mid.selected, mid.enabled);
                    if (!mid.tooltip.empty()) showTooltip(mid.tooltip);

                    if (selected != mid.selected)
                    {
                        if (mid.callback) (*mid.callback)(mgr, content, this);
                        if (!mid.checkable) mid.selected = false;
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenuBar();
    }
    return began;
}

bool ListView::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    std::vector<const char*> itemValues(items.size());
    for (size_t i = 0; i < items.size(); ++i) itemValues[i] = items[i].c_str();

    bool done = false;
    if (itemValues.empty())
        done = ImGui::ListBox(name.c_str(), &index, NULL, 0, rows);
    else
        done = ImGui::ListBox(name.c_str(), &index, &itemValues[0], (int)items.size(), rows);

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) (*callback)(mgr, content, this);
    return done;
}

bool TreeView::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    for (size_t i = 0; i < treeDataList.size(); ++i)
    {
        TreeData& td = treeDataList[i];
        showRecursively(td, mgr, content);
    }
    return !selectedItemID.empty();
}

void TreeView::showRecursively(TreeData& td, ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (td.id.empty()) td.id = td.name;
    if (!selectedItemID.empty() && selectedItemID != td.id)
        td.flags &= ~ImGuiTreeNodeFlags_Selected;

    if (ImGui::TreeNodeEx(td.name.c_str(), td.flags))
    {
        if (ImGui::IsItemClicked())
        {
            td.flags |= ImGuiTreeNodeFlags_Selected; selectedItemID = td.id;
            if (callback) (*callback)(mgr, content, this, td.id);
        }

        if (!td.tooltip.empty()) showTooltip(td.tooltip);
        for (size_t i = 0; i < td.children.size(); ++i)
            showRecursively(td.children[i], mgr, content);
        ImGui::TreePop();
    }
}

struct InternalSequence : public ImSequencer::SequenceInterface
{
    InternalSequence(Timeline* t) : timeline(t) {}
    Timeline* timeline;

    virtual int GetFrameMin() const { return timeline->frameRange[0]; }
    virtual int GetFrameMax() const { return timeline->frameRange[1]; }
    virtual int GetItemCount() const { return (int)timeline->items.size(); }

    virtual void BeginEdit(int index) {}
    virtual void EndEdit() {}
    virtual int GetItemTypeCount() const { return 0; }
    virtual const char* GetItemTypeName(int typeIndex) const { return ""; }
    virtual const char* GetItemLabel(int index) { return ""; }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {}

    virtual void Add(int type) {}
    virtual void Del(int index) {}
    virtual void Duplicate(int index) {}
    virtual void Copy() {}
    virtual void Paste() {}

    virtual size_t GetCustomHeight(int index) { return 0; }
    virtual void DoubleClick(int index) {}

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc,
                            const ImRect& legendRect, const ImRect& clippingRect,
                            const ImRect& legendClippingRect)
    {}

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc,
                                   const ImRect& clippingRect)
    {}
};

bool Timeline::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (!seqInterface) seqInterface = new InternalSequence(this);
    InternalSequence* seq = (InternalSequence*)seqInterface;
    if (flags == 0)
        flags = ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL
              | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME;
    
    bool done = ImSequencer::Sequencer(seq, &currentFrame, &expanded, &selectedIndex, &firstFrame, flags);
    if (selectedIndex != -1 && callback) (*callback)(mgr, content, this);
    return done;
}
