#include "ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <imgui/ImGuiFileDialog.h>
using namespace osgVerse;

std::string ImGuiComponentBase::TR(const std::string& s)
{
    // TODO
    return s;
}

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

void ImGuiComponentBase::showTooltip(const std::string& desc, const std::string& t, float wrapPos)
{
    ImGui::SameLine(); ImGui::TextDisabled(TR(t).c_str());
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * wrapPos);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ImGuiComponentBase::registerFileDialog(
    FileDialogCallback cb, const std::string& name, const std::string& title, bool modal,
    const std::string& dir, const std::string& filters)
{
    ImGuiFileDialogFlags flags = (modal ? ImGuiFileDialogFlags_Modal : 0);
    ImGuiFileDialog::Instance()->OpenDialog(  // FIXME: more options
        name, title, filters.c_str(), dir, 1, NULL,
        flags | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
    s_fileDialogRunner.name = name; s_fileDialogRunner.callback = cb;
}

bool ImGuiComponentBase::showFileDialog(std::string& result)
{
    if (s_fileDialogRunner.name.empty()) return false;
    if (ImGuiFileDialog::Instance()->Display(s_fileDialogRunner.name))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            result = ImGuiFileDialog::Instance()->GetFilePathName();
        ImGuiFileDialog::Instance()->Close();
        if (!result.empty() && s_fileDialogRunner.callback)
            s_fileDialogRunner.callback(result);
        s_fileDialogRunner.name = ""; return true;
    }
    return false;
}

void ImGuiComponentBase::registerConfirmDialog(
    ConfirmDialogCallback cb, const std::string& name, const std::string& title, bool modal,
    const std::string& btn0, const std::string& btn1)
{
    s_confirmDialogRunner.name = name; s_confirmDialogRunner.title = title;
    s_confirmDialogRunner.btn0 = btn0; s_confirmDialogRunner.btn1 = btn1;
    s_confirmDialogRunner.modal = modal; s_confirmDialogRunner.init = true;
    s_confirmDialogRunner.callback = cb;
}

bool ImGuiComponentBase::showConfirmDialog(bool& result)
{
    bool displayed = false, closed = false;
    if (s_confirmDialogRunner.name.empty()) return false;
    else if (s_confirmDialogRunner.init)
    {
        ImGui::OpenPopup(s_confirmDialogRunner.name.c_str());
        s_confirmDialogRunner.init = false;
    }

    if (s_confirmDialogRunner.modal)
        displayed = ImGui::BeginPopupModal(s_confirmDialogRunner.name.c_str(),
                                           NULL, ImGuiWindowFlags_AlwaysAutoResize);
    else
        displayed = ImGui::BeginPopup(s_confirmDialogRunner.name.c_str(), ImGuiWindowFlags_AlwaysAutoResize);

    if (displayed)
    {
        ImGui::Text(s_confirmDialogRunner.title.c_str()); ImGui::Separator();
        if (ImGui::Button(s_confirmDialogRunner.btn0.c_str(), ImVec2(120, 0)))
        { ImGui::CloseCurrentPopup(); result = true; closed = true; }
        
        if (!s_confirmDialogRunner.btn0.empty())
        {
            ImGui::SetItemDefaultFocus(); ImGui::SameLine();
            if (ImGui::Button(s_confirmDialogRunner.btn1.c_str(), ImVec2(120, 0)))
            { ImGui::CloseCurrentPopup(); result = false; closed = true; }
        }

        if (closed)
        {
            if (s_confirmDialogRunner.callback) s_confirmDialogRunner.callback(result);
            s_confirmDialogRunner.name = "";
        }
        ImGui::EndPopup();
    }
    return closed;
}

ImGuiComponentBase::FileDialogData ImGuiComponentBase::s_fileDialogRunner;
ImGuiComponentBase::ConfirmDialogData ImGuiComponentBase::s_confirmDialogRunner;

void Window::resize(const osg::Vec2& p, const osg::Vec2& s)
{ pos = p; size = s; sizeApplied = false; }

bool Window::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!isOpen) return false;
    if (!sizeApplied)
    {
        ImGui::SetNextWindowPos(
            ImVec2(vp->WorkPos[0] + pos[0] * vp->WorkSize[0],
                vp->WorkPos[1] + pos[1] * vp->WorkSize[1]), 0, ImVec2(pivot[0], pivot[1]));
        if (size.length2() > 0.0f)
            ImGui::SetNextWindowSize(ImVec2(size[0] * vp->WorkSize[0], size[1] * vp->WorkSize[1]));
        sizeApplied = true;
    }

    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::SetNextWindowCollapsed(collapsed);
    if (useMenuBar) flags |= ImGuiWindowFlags_MenuBar;
    
    bool done = ImGui::Begin(name.c_str(), &isOpen, flags);
    if (done)
    {
        ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
        rectRT.set((p[0] - vp->WorkPos[0]) / vp->WorkSize[0], (p[1] - vp->WorkPos[1]) / vp->WorkSize[1],
                   s[0] / vp->WorkSize[0], s[1] / vp->WorkSize[1]);
    }
    return done;
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
    if (done && callback) callback(mgr, content, this);
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
    if (done && callback) callback(mgr, content, this);
    return done;
}

bool CheckBox::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = ImGui::Checkbox(name.c_str(), &value);
    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) callback(mgr, content, this);
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
    if (done && callback) callback(mgr, content, this);
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

    if (lastValue != value && callback) callback(mgr, content, this);
    return lastValue != value;
}

bool InputField::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false; size_t size = value.size() + 10;
    if (size > 128) size = 128; value.resize(size);
    if (placeholder.empty())
        done = ImGui::InputText(name.c_str(), &value[0], size, flags);
    else
        done = ImGui::InputTextWithHint(name.c_str(), placeholder.c_str(),
                                        &value[0], size, flags);

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) callback(mgr, content, this);
    return done;
}

bool InputValueField::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool done = false;
    switch (type)
    {
    case IntValue: case UIntValue:
        {
            int valueI = (type == UIntValue) ? (unsigned int)value : (int)value;
            done = ImGui::InputInt(name.c_str(), &valueI, (int)step, (int)step * 5, flags);
            if (done) value = (double)valueI;
            if (minValue < maxValue) value = osg::clampBetween((double)valueI, minValue, maxValue);
        }
        break;
    case FloatValue:
        {
            float valueF = (float)value;
            if (format.empty()) format = "%.5f";
            done = ImGui::InputFloat(name.c_str(), &valueF, (float)step, (float)step * 5,
                                     format.c_str(), flags);
            if (done) value = (double)valueF;
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
    if (done && callback) callback(mgr, content, this);
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
        if (done && callback) callback(mgr, content, this);
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
    if (done && callback) callback(mgr, content, this);
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
    if (done && callback) callback(mgr, content, this);
    return done;
}

void MenuBarBase::showMenuItem(MenuItemData& mid, ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (mid.name.empty()) ImGui::Separator();
    else if (mid.subItems.empty())
    {
        bool selected = mid.selected;
        ImGui::MenuItem(
            mid.name.c_str(), (mid.shortcut.empty() ? NULL : mid.shortcut.c_str()),
            &mid.selected, mid.enabled);
        if (!mid.tooltip.empty()) showTooltip(mid.tooltip);

        if (selected != mid.selected)
        {
            if (mid.callback) (mid.callback)(mgr, content, this);
            if (!mid.checkable) mid.selected = false;
        }
    }
    else if (ImGui::BeginMenu(mid.name.c_str(), mid.enabled))
    {
        for (size_t j = 0; j < mid.subItems.size(); ++j)
        {
            MenuItemData& subMid = mid.subItems[j];
            showMenuItem(subMid, mgr, content);
        }
        ImGui::EndMenu();
    }
}

void MenuBarBase::showMenu(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    for (size_t i = 0; i < menuDataList.size(); ++i)
    {
        MenuData& md = menuDataList[i];
        if (ImGui::BeginMenu(md.name.c_str(), md.enabled))
        {
            for (size_t j = 0; j < md.items.size(); ++j)
            {
                MenuItemData& mid = md.items[j];
                showMenuItem(mid, mgr, content);
            }
            ImGui::EndMenu();
        }
    }
}

bool MenuBar::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool began = ImGui::BeginMenuBar();  // parent window must have ImGuiWindowFlags_MenuBar flag
    if (began) { showMenu(mgr, content); ImGui::EndMenuBar(); } return began;
}

MenuBar::MenuItemData MenuBar::MenuItemData::separator("");

bool MainMenuBar::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    bool began = ImGui::BeginMainMenuBar();
    if (began) { showMenu(mgr, content); ImGui::EndMainMenuBar(); } return began;
}

bool ListView::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    std::vector<const char*> itemValues(items.size());
    for (size_t i = 0; i < items.size(); ++i) itemValues[i] = items[i]->name.c_str();

    bool done = false;
    if (itemValues.empty())
        done = ImGui::ListBox(name.c_str(), &index, NULL, 0, rows);
    else
        done = ImGui::ListBox(name.c_str(), &index, &itemValues[0], (int)items.size(), rows);

    if (!tooltip.empty()) showTooltip(tooltip);
    if (done && callback) callback(mgr, content, this);
    return done;
}

/// TreeView related

static void findTreeDataRecursively(TreeView::TreeData& td, std::vector<TreeView::TreeData*>& output,
                                    const std::string& id, const std::string& name, osg::Referenced* ud)
{
    if (!id.empty()) { if (id == td.id) { output.push_back(&td); return; } }
    else if (!name.empty()) { if (name == td.name) output.push_back(&td); }
    else if (ud) { if (ud == td.userData) output.push_back(&td); }
    for (size_t i = 0; i < td.children.size(); ++i)
        findTreeDataRecursively(*td.children[i], output, id, name, ud);
}

std::vector<TreeView::TreeData*> TreeView::findByName(const std::string& name) const
{
    std::vector<TreeView::TreeData*> output;
    for (size_t i = 0; i < treeDataList.size(); ++i)
        findTreeDataRecursively(*treeDataList[i], output, "", name, NULL);
    return output;
}

std::vector<TreeView::TreeData*> TreeView::findByUserData(osg::Referenced* ud) const
{
    std::vector<TreeView::TreeData*> output;
    for (size_t i = 0; i < treeDataList.size(); ++i)
        findTreeDataRecursively(*treeDataList[i], output, "", "", ud);
    return output;
}

TreeView::TreeData* TreeView::findByID(const std::string& id) const
{
    std::vector<TreeView::TreeData*> output;
    for (size_t i = 0; i < treeDataList.size(); ++i)
        findTreeDataRecursively(*treeDataList[i], output, id, "", NULL);
    return output.empty() ? NULL : output[0];
}

bool TreeView::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    for (size_t i = 0; i < treeDataList.size(); ++i)
    {
        TreeData& td = *treeDataList[i];
        showRecursively(td, mgr, content);
    }
    return !selectedItemID.empty();
}

void TreeView::showRecursively(TreeData& td, ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (td.id.empty()) td.id = td.name;
    if (!selectedItemID.empty() && selectedItemID != td.id)
        td.flags &= ~ImGuiTreeNodeFlags_Selected;
    if (td.children.empty()) td.flags |= ImGuiTreeNodeFlags_Leaf;
    else td.flags &= ~ImGuiTreeNodeFlags_Leaf;

    if (ImGui::TreeNodeEx(td.name.c_str(), td.flags))
    {
        if (ImGui::IsItemClicked(0))
        {
            td.flags |= ImGuiTreeNodeFlags_Selected; selectedItemID = td.id;
            if (callback) callback(mgr, content, this, td.id);
        }
        else if (ImGui::IsItemClicked(1))
        {
            td.flags |= ImGuiTreeNodeFlags_Selected; selectedItemID = td.id;
            if (callback) callback(mgr, content, this, td.id);
            if (callbackR) callbackR(mgr, content, this, td.id);
        }

        if (!td.tooltip.empty()) showTooltip(td.tooltip);
        for (size_t i = 0; i < td.children.size(); ++i)
            showRecursively(*td.children[i], mgr, content);
        ImGui::TreePop();
    }
}
