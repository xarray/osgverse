#include "ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <imgui/ImGuiFileDialog.h>
#include <imgui/ImSequencer.h>
#include <imgui/ImCurveEdit.h>
#include <imgui/node-editor/imgui_node_editor.h>
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
    bool done = ImGui::CheckboxFlags(name.c_str(), &value, 0xffffffff);
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

    if (lastValue != value) callback(mgr, content, this);
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
                        if (mid.callback) (mid.callback)(mgr, content, this);
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

/// Timeline related

struct RampCurveEdit : public ImCurveEdit::Delegate
{
    RampCurveEdit()
    {
        _points[0][0] = ImVec2(-10.f, 0);
        _points[0][1] = ImVec2(20.f, 0.6f);
        _points[0][2] = ImVec2(25.f, 0.2f);
        _points[0][3] = ImVec2(70.f, 0.4f);
        _points[0][4] = ImVec2(120.f, 1.f);
        _pointCount[0] = 5;

        _points[1][0] = ImVec2(-50.f, 0.2f);
        _points[1][1] = ImVec2(33.f, 0.7f);
        _points[1][2] = ImVec2(80.f, 0.2f);
        _points[1][3] = ImVec2(82.f, 0.8f);
        _pointCount[1] = 4;

        _points[2][0] = ImVec2(40.f, 0);
        _points[2][1] = ImVec2(60.f, 0.1f);
        _points[2][2] = ImVec2(90.f, 0.82f);
        _points[2][3] = ImVec2(150.f, 0.24f);
        _points[2][4] = ImVec2(200.f, 0.34f);
        _points[2][5] = ImVec2(250.f, 0.12f);
        _pointCount[2] = 6;

        _max = ImVec2(1.f, 1.f); _min = ImVec2(0.f, 0.f);
        _visible[0] = _visible[1] = _visible[2] = true;
    }

    ImVec2* GetPoints(size_t curveIndex) { return _points[curveIndex]; }
    bool IsVisible(size_t curveIndex) { return _visible[curveIndex]; }
    size_t GetPointCount(size_t curveIndex) { return _pointCount[curveIndex]; }
    size_t GetCurveCount() { return 3; }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
        return cols[curveIndex];
    }

    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const
    { return ImCurveEdit::CurveSmooth; }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        _points[curveIndex][pointIndex] = ImVec2(value.x, value.y);
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        { if (_points[curveIndex][i].x == value.x) return (int)i; }
        return pointIndex;
    }

    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (_pointCount[curveIndex] >= 8) return;
        _points[curveIndex][_pointCount[curveIndex]++] = value;
        SortValues(curveIndex);
    }

    virtual ImVec2& GetMax() { return _min; }
    virtual ImVec2& GetMin() { return _max; }
    virtual unsigned int GetBackgroundColor() { return 0; }

    ImVec2 _points[3][8], _min, _max;
    size_t _pointCount[3];
    bool _visible[3];

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(_points[curveIndex]);
        auto e = std::begin(_points[curveIndex]) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
    }
};

struct InternalSequence : public ImSequencer::SequenceInterface
{
    InternalSequence(Timeline* t) : timeline(t) {}
    RampCurveEdit rampEdit;
    Timeline* timeline;

    virtual int GetFrameMin() const { return timeline->frameRange[0]; }
    virtual int GetFrameMax() const { return timeline->frameRange[1]; }
    virtual int GetItemCount() const { return (int)timeline->items.size(); }

    virtual int GetItemTypeCount() const { return 1; }
    virtual const char* GetItemTypeName(int typeIndex) const { return "Normal"; }
    virtual const char* GetItemLabel(int index) { return timeline->items[index]->name.c_str(); }
    virtual size_t GetCustomHeight(int i) { return timeline->items[i]->expanded ? 300 : 0; }

    virtual void BeginEdit(int index) {}
    virtual void EndEdit() {}

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        Timeline::SequenceItem& item = *(timeline->items[index]);
        if (color) *color = 0xFFAA8080; // same color for everyone?
        if (start) *start = &item.range[0];
        if (end) *end = &item.range[1];
        if (type) *type = item.type;
    }

    virtual void Add(int type)
    {
        timeline->items.push_back(new Timeline::SequenceItem(
            "Seq" + std::to_string(timeline->items.size()), type, 0, 10));
    }

    virtual void Del(int index) { timeline->items.erase(timeline->items.begin() + index); }
    virtual void Duplicate(int index) { timeline->items.push_back(timeline->items[index]); }
    virtual void Copy() {}
    virtual void Paste() {}

    virtual void DoubleClick(int index)
    {
        if (!timeline->items[index]->expanded)
        {
            for (auto& item : timeline->items) item->expanded = false;
            timeline->items[index]->expanded = !timeline->items[index]->expanded;
        }
        else
            timeline->items[index]->expanded = false;
    }

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc,
                            const ImRect& legendRect, const ImRect& clippingRect,
                            const ImRect& legendClippingRect)
    {
        static const char* labels[] = { "Translation", "Rotation" , "Scale" };
        rampEdit._max = ImVec2(float(timeline->frameRange[1]), 1.f);
        rampEdit._min = ImVec2(float(timeline->frameRange[0]), 0.f);

        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);

            draw_list->AddText(pta, rampEdit._visible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                rampEdit._visible[i] = !rampEdit._visible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(rampEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
    }

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc,
                                   const ImRect& clippingRect)
    {
        rampEdit._max = ImVec2(float(timeline->frameRange[1]), 1.f);
        rampEdit._min = ImVec2(float(timeline->frameRange[0]), 0.f);

        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < rampEdit._pointCount[i]; j++)
            {
                float p = rampEdit._points[i][j].x;
                if (p < timeline->items[index]->range[0] ||
                    p > timeline->items[index]->range[1]) continue;

                float r = (p - (float)timeline->frameRange[0])
                        / float(timeline->frameRange[1] - timeline->frameRange[0]);
                float x = ImLerp(rc.Min.x, rc.Max.x, r);
                draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            }
        }
        draw_list->PopClipRect();
    }
};

bool Timeline::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (!seqInterface) seqInterface = new InternalSequence(this);
    InternalSequence* seq = (InternalSequence*)seqInterface;
    if (flags == 0)
        flags = ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL
              | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME;
    
    bool done = ImSequencer::Sequencer(seq, &currentFrame, &expanded, &selectedIndex, &firstFrame, flags);
    if (selectedIndex != -1 && callback) callback(mgr, content, this);
    return done;
}
