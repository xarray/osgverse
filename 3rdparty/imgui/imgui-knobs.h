#pragma once

#include <cstdlib>
#include <imgui.h>

typedef int ImGuiKnobFlags;

enum ImGuiKnobFlags_ {
    ImGuiKnobFlags_NoTitle = 1 << 0,
    ImGuiKnobFlags_NoInput = 1 << 1,
    ImGuiKnobFlags_ValueTooltip = 1 << 2,
    ImGuiKnobFlags_DragHorizontal = 1 << 3,
    ImGuiKnobFlags_DragVertical = 1 << 4,
    ImGuiKnobFlags_Logarithmic = 1 << 5,
    ImGuiKnobFlags_AlwaysClamp = 1 << 6
};

typedef int ImGuiKnobVariant;

enum ImGuiKnobVariant_ {
    ImGuiKnobVariant_Tick = 1 << 0,
    ImGuiKnobVariant_Dot = 1 << 1,
    ImGuiKnobVariant_Wiper = 1 << 2,
    ImGuiKnobVariant_WiperOnly = 1 << 3,
    ImGuiKnobVariant_WiperDot = 1 << 4,
    ImGuiKnobVariant_Stepped = 1 << 5,
    ImGuiKnobVariant_Space = 1 << 6,
};

namespace ImGuiKnobs {

    struct color_set {
        ImColor base;
        ImColor hovered;
        ImColor active;

        color_set(ImColor base, ImColor hovered, ImColor active)
            : base(base), hovered(hovered), active(active) {}

        color_set(ImColor color) {
            base = color;
            hovered = color;
            active = color;
        }
    };

    bool Knob(
            const char *label,
            float *p_value,
            float v_min,
            float v_max,
            float speed = 0,
            const char *format = "%.3f",
            ImGuiKnobVariant variant = ImGuiKnobVariant_Tick,
            float size = 0,
            ImGuiKnobFlags flags = 0,
            int steps = 10,
            float angle_min = -1,
            float angle_max = -1);
    bool KnobInt(
            const char *label,
            int *p_value,
            int v_min,
            int v_max,
            float speed = 0,
            const char *format = "%i",
            ImGuiKnobVariant variant = ImGuiKnobVariant_Tick,
            float size = 0,
            ImGuiKnobFlags flags = 0,
            int steps = 10,
            float angle_min = -1,
            float angle_max = -1);

}// namespace ImGuiKnobs
