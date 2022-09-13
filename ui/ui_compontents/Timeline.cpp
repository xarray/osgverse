#include "../ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <imgui/ImGuiFileDialog.h>
#include <imgui/ImSequencer.h>
#include <imgui/ImCurveEdit.h>
using namespace osgVerse;

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
    {
        return ImCurveEdit::CurveSmooth;
    }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        _points[curveIndex][pointIndex] = ImVec2(value.x, value.y);
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (_points[curveIndex][i].x == value.x) return (int)i;
        }
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
