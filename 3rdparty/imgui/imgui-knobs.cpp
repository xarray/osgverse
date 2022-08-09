#include "imgui-knobs.h"

#include <cmath>
#include <cstdlib>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#define IMGUIKNOBS_PI 3.14159265358979323846

namespace ImGuiKnobs {
    namespace detail {
        void draw_arc1(ImVec2 center, float radius, float start_angle, float end_angle, float thickness, ImColor color, int num_segments) {
            ImVec2 start = {
                    center[0] + cosf(start_angle) * radius,
                    center[1] + sinf(start_angle) * radius,
            };

            ImVec2 end = {
                    center[0] + cosf(end_angle) * radius,
                    center[1] + sinf(end_angle) * radius,
            };

            // Calculate bezier arc points
            auto ax = start[0] - center[0];
            auto ay = start[1] - center[1];
            auto bx = end[0] - center[0];
            auto by = end[1] - center[1];
            auto q1 = ax * ax + ay * ay;
            auto q2 = q1 + ax * bx + ay * by;
            auto k2 = (4.0f / 3.0f) * (sqrtf((2.0f * q1 * q2)) - q2) / (ax * by - ay * bx);
            auto arc1 = ImVec2{center[0] + ax - k2 * ay, center[1] + ay + k2 * ax};
            auto arc2 = ImVec2{center[0] + bx + k2 * by, center[1] + by - k2 * bx};

            auto *draw_list = ImGui::GetWindowDrawList();

            draw_list->AddBezierCurve(start, arc1, arc2, end, color, thickness, num_segments);
        }

        void draw_arc(ImVec2 center, float radius, float start_angle, float end_angle, float thickness, ImColor color, int num_segments, int bezier_count) {
            // Overlap and angle of ends of bezier curves needs work, only looks good when not transperant
            auto overlap = thickness * radius * 0.00001f * IMGUIKNOBS_PI;
            auto delta = end_angle - start_angle;
            auto bez_step = 1.0f / bezier_count;
            auto mid_angle = start_angle + overlap;

            for (auto i = 0; i < bezier_count - 1; i++) {
                auto mid_angle2 = delta * bez_step + mid_angle;
                draw_arc1(center, radius, mid_angle - overlap, mid_angle2 + overlap, thickness, color, num_segments);
                mid_angle = mid_angle2;
            }

            draw_arc1(center, radius, mid_angle - overlap, end_angle, thickness, color, num_segments);
        }

        template<typename DataType>
        struct knob {
            float radius;
            bool value_changed;
            ImVec2 center;
            bool is_active;
            bool is_hovered;
            float angle_min;
            float angle_max;
            float t;
            float angle;
            float angle_cos;
            float angle_sin;

            knob(const char *_label, ImGuiDataType data_type, DataType *p_value, DataType v_min, DataType v_max, float speed, float _radius, const char *format, ImGuiKnobFlags flags) {
                radius = _radius;
                t = ((float) *p_value - v_min) / (v_max - v_min);
                auto screen_pos = ImGui::GetCursorScreenPos();

                // Handle dragging
                ImGui::InvisibleButton(_label, {radius * 2.0f, radius * 2.0f});
                auto gid = ImGui::GetID(_label);
                ImGuiSliderFlags drag_flags = 0;
                if (!(flags & ImGuiKnobFlags_DragHorizontal)) {
                    drag_flags |= ImGuiSliderFlags_Vertical;
                }
                value_changed = ImGui::DragBehavior(gid, data_type, p_value, speed, &v_min, &v_max, format, drag_flags);

                angle_min = IMGUIKNOBS_PI * 0.75;
                angle_max = IMGUIKNOBS_PI * 2.25;
                center = {screen_pos[0] + radius, screen_pos[1] + radius};
                is_active = ImGui::IsItemActive();
                is_hovered = ImGui::IsItemHovered();
                angle = angle_min + (angle_max - angle_min) * t;
                angle_cos = cosf(angle);
                angle_sin = sinf(angle);
            }

            void draw_dot(float size, float radius, float angle, color_set color, bool filled, int segments) {
                auto dot_size = size * this->radius;
                auto dot_radius = radius * this->radius;

                ImGui::GetWindowDrawList()->AddCircleFilled(
                        {center[0] + cosf(angle) * dot_radius, center[1] + sinf(angle) * dot_radius},
                        dot_size,
                        is_active ? color.active : (is_hovered ? color.hovered : color.base),
                        segments);
            }

            void draw_tick(float start, float end, float width, float angle, color_set color) {
                auto tick_start = start * radius;
                auto tick_end = end * radius;
                auto angle_cos = cosf(angle);
                auto angle_sin = sinf(angle);

                ImGui::GetWindowDrawList()->AddLine(
                        {center[0] + angle_cos * tick_end, center[1] + angle_sin * tick_end},
                        {center[0] + angle_cos * tick_start, center[1] + angle_sin * tick_start},
                        is_active ? color.active : (is_hovered ? color.hovered : color.base),
                        width * radius);
            }

            void draw_circle(float size, color_set color, bool filled, int segments) {
                auto circle_radius = size * radius;

                ImGui::GetWindowDrawList()->AddCircleFilled(
                        center,
                        circle_radius,
                        is_active ? color.active : (is_hovered ? color.hovered : color.base));
            }

            void draw_arc(float radius, float size, float start_angle, float end_angle, color_set color, int segments, int bezier_count) {
                auto track_radius = radius * this->radius;
                auto track_size = size * this->radius * 0.5f + 0.0001f;

                detail::draw_arc(
                        center,
                        track_radius,
                        start_angle,
                        end_angle,
                        track_size,
                        is_active ? color.active : (is_hovered ? color.hovered : color.base),
                        segments,
                        bezier_count);
            }
        };

        template<typename DataType>
        knob<DataType> knob_with_drag(const char *label, ImGuiDataType data_type, DataType *p_value, DataType v_min, DataType v_max, float _speed, const char *format, float size, ImGuiKnobFlags flags) {
            auto speed = _speed == 0 ? (v_max - v_min) / 250.f : _speed;
            ImGui::PushID(label);
            auto width = size == 0 ? ImGui::GetTextLineHeight() * 4.0f : size * ImGui::GetIO().FontGlobalScale;
            ImGui::PushItemWidth(width);

            ImGui::BeginGroup();

            // There's an issue with `SameLine` and Groups, see https://github.com/ocornut/imgui/issues/4190.
            // This is probably not the best solution, but seems to work for now
            ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;

            // Draw title
            if (!(flags & ImGuiKnobFlags_NoTitle)) {
                auto title_size = ImGui::CalcTextSize(label, NULL, false, width);

                // Center title
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - title_size[0]) * 0.5f);

                ImGui::Text("%s", label);
            }

            // Draw knob
            knob<DataType> k(label, data_type, p_value, v_min, v_max, speed, width * 0.5f, format, flags);

            // Draw tooltip
            if (flags & ImGuiKnobFlags_ValueTooltip && (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemActive())) {
                ImGui::BeginTooltip();
                ImGui::Text(format, *p_value);
                ImGui::EndTooltip();
            }

            // Draw input
            if (!(flags & ImGuiKnobFlags_NoInput)) {
                ImGuiSliderFlags drag_flags = 0;
                if (!(flags & ImGuiKnobFlags_DragHorizontal)) {
                    drag_flags |= ImGuiSliderFlags_Vertical;
                }
                ImGui::DragScalar("###knob_drag", data_type, p_value, speed, &v_min, &v_max, format, drag_flags);
            }

            ImGui::EndGroup();
            ImGui::PopItemWidth();
            ImGui::PopID();

            return k;
        }

        color_set GetPrimaryColorSet() {
            auto *colors = ImGui::GetStyle().Colors;

            return {colors[ImGuiCol_ButtonActive], colors[ImGuiCol_ButtonHovered], colors[ImGuiCol_ButtonHovered]};
        }

        color_set GetSecondaryColorSet() {
            auto *colors = ImGui::GetStyle().Colors;
            auto active = ImVec4(
                    colors[ImGuiCol_ButtonActive].x * 0.5f,
                    colors[ImGuiCol_ButtonActive].y * 0.5f,
                    colors[ImGuiCol_ButtonActive].z * 0.5f,
                    colors[ImGuiCol_ButtonActive].w);

            auto hovered = ImVec4(
                    colors[ImGuiCol_ButtonHovered].x * 0.5f,
                    colors[ImGuiCol_ButtonHovered].y * 0.5f,
                    colors[ImGuiCol_ButtonHovered].z * 0.5f,
                    colors[ImGuiCol_ButtonHovered].w);

            return {active, hovered, hovered};
        }

        color_set GetTrackColorSet() {
            auto *colors = ImGui::GetStyle().Colors;

            return {colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBg]};
        }
    }// namespace detail


    template<typename DataType>
    bool BaseKnob(const char *label, ImGuiDataType data_type, DataType *p_value, DataType v_min, DataType v_max, float speed, const char *format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps = 10) {
        auto knob = detail::knob_with_drag(label, data_type, p_value, v_min, v_max, speed, format, size, flags);

        switch (variant) {
            case ImGuiKnobVariant_Tick: {
                knob.draw_circle(0.85, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_tick(0.5, 0.85, 0.08, knob.angle, detail::GetPrimaryColorSet());
                break;
            }
            case ImGuiKnobVariant_Dot: {
                knob.draw_circle(0.85, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_dot(0.12, 0.6, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }

            case ImGuiKnobVariant_Wiper: {
                knob.draw_circle(0.7, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_arc(0.8, 0.41, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 16, 2);

                if (knob.t > 0.01) {
                    knob.draw_arc(0.8, 0.43, knob.angle_min, knob.angle, detail::GetPrimaryColorSet(), 16, 2);
                }
                break;
            }
            case ImGuiKnobVariant_WiperOnly: {
                knob.draw_arc(0.8, 0.41, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 32, 2);

                if (knob.t > 0.01) {
                    knob.draw_arc(0.8, 0.43, knob.angle_min, knob.angle, detail::GetPrimaryColorSet(), 16, 2);
                }
                break;
            }
            case ImGuiKnobVariant_WiperDot: {
                knob.draw_circle(0.6, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_arc(0.85, 0.41, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 16, 2);
                knob.draw_dot(0.1, 0.85, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }
            case ImGuiKnobVariant_Stepped: {
                for (auto n = 0.f; n < steps; n++) {
                    auto a = n / (steps - 1);
                    auto angle = knob.angle_min + (knob.angle_max - knob.angle_min) * a;
                    knob.draw_tick(0.7, 0.9, 0.04, angle, detail::GetPrimaryColorSet());
                }

                knob.draw_circle(0.6, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_dot(0.12, 0.4, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }
            case ImGuiKnobVariant_Space: {
                knob.draw_circle(0.3 - knob.t * 0.1, detail::GetSecondaryColorSet(), true, 16);

                if (knob.t > 0.01) {
                    knob.draw_arc(0.4, 0.15, knob.angle_min - 1.0, knob.angle - 1.0, detail::GetPrimaryColorSet(), 16, 2);
                    knob.draw_arc(0.6, 0.15, knob.angle_min + 1.0, knob.angle + 1.0, detail::GetPrimaryColorSet(), 16, 2);
                    knob.draw_arc(0.8, 0.15, knob.angle_min + 3.0, knob.angle + 3.0, detail::GetPrimaryColorSet(), 16, 2);
                }
                break;
            }
        }

        return knob.value_changed;
    }

    bool Knob(const char *label, float *p_value, float v_min, float v_max, float speed, const char *format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps) {
        const char *_format = format == NULL ? "%.3f" : format;
        return BaseKnob(label, ImGuiDataType_Float, p_value, v_min, v_max, speed, _format, variant, size, flags, steps);
    }

    bool KnobInt(const char *label, int *p_value, int v_min, int v_max, float speed, const char *format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps) {
        const char *_format = format == NULL ? "%i" : format;
        return BaseKnob(label, ImGuiDataType_S32, p_value, v_min, v_max, speed, _format, variant, size, flags, steps);
    }

}// namespace ImGuiKnobs
