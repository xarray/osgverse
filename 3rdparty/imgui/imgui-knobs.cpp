#include "imgui-knobs.h"

#include <cmath>
#include <cstdlib>
#include <imgui.h>
#include <imgui_internal.h>

#define IMGUIKNOBS_PI 3.14159265358979323846f

static inline float ImLog(int x) { return ImLog(static_cast<float>(x)); }

namespace ImGuiKnobs {
    namespace detail {
        void draw_arc(ImVec2 center, float radius, float start_angle, float end_angle, float thickness, ImColor color) {
            auto *draw_list = ImGui::GetWindowDrawList();

            draw_list->PathArcTo(center, radius, start_angle, end_angle);
            draw_list->PathStroke(color, 0, thickness);
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

            knob(const char *_label,
                 ImGuiDataType data_type,
                 DataType *p_value,
                 DataType v_min,
                 DataType v_max,
                 float speed,
                 float _radius,
                 const char *format,
                 ImGuiKnobFlags flags,
                 float _angle_min,
                 float _angle_max) {
                radius = _radius;
                if (flags & ImGuiKnobFlags_Logarithmic) {
                    float v = ImMax(ImMin(*p_value, v_max), v_min);
                    t = (ImLog(ImAbs(v)) - ImLog(ImAbs(v_min))) / (ImLog(ImAbs(v_max)) - ImLog(ImAbs(v_min)));
                } else {
                    t = ((float) *p_value - v_min) / (v_max - v_min);
                }
                auto screen_pos = ImGui::GetCursorScreenPos();

                // Handle dragging
                ImGui::InvisibleButton(_label, {radius * 2.0f, radius * 2.0f});

                // Handle drag: if DragVertical or DragHorizontal flags are set, only the given direction is
                // used, otherwise use the drag direction with the highest delta
                ImGuiIO &io = ImGui::GetIO();
                bool drag_vertical =
                        !(flags & ImGuiKnobFlags_DragHorizontal) &&
                        (flags & ImGuiKnobFlags_DragVertical || ImAbs(io.MouseDelta[ImGuiAxis_Y]) > ImAbs(io.MouseDelta[ImGuiAxis_X]));

                auto gid = ImGui::GetID(_label);
                ImGuiSliderFlags drag_behaviour_flags = 0;
                if (drag_vertical) {
                    drag_behaviour_flags |= ImGuiSliderFlags_Vertical;
                }
                if (flags & ImGuiKnobFlags_AlwaysClamp) {
                    drag_behaviour_flags |= ImGuiSliderFlags_AlwaysClamp;
                }
                if (flags & ImGuiKnobFlags_Logarithmic) {
                    drag_behaviour_flags |= ImGuiSliderFlags_Logarithmic;
                }
                value_changed = ImGui::DragBehavior(
                        gid,
                        data_type,
                        p_value,
                        speed,
                        &v_min,
                        &v_max,
                        format,
                        drag_behaviour_flags);

                angle_min = _angle_min < 0 ? IMGUIKNOBS_PI * 0.75f : _angle_min;
                angle_max = _angle_max < 0 ? IMGUIKNOBS_PI * 2.25f : _angle_max;

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
                        {center[0] + cosf(angle) * dot_radius,
                         center[1] + sinf(angle) * dot_radius},
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
                        {center[0] + angle_cos * tick_start,
                         center[1] + angle_sin * tick_start},
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

            void draw_arc(float radius, float size, float start_angle, float end_angle, color_set color) {
                auto track_radius = radius * this->radius;
                auto track_size = size * this->radius * 0.5f + 0.0001f;

                detail::draw_arc(center, track_radius, start_angle, end_angle, track_size, is_active ? color.active : (is_hovered ? color.hovered : color.base));
            }
        };

        template<typename DataType>
        knob<DataType> knob_with_drag(
                const char *label,
                ImGuiDataType data_type,
                DataType *p_value,
                DataType v_min,
                DataType v_max,
                float _speed,
                const char *format,
                float size,
                ImGuiKnobFlags flags,
                float angle_min,
                float angle_max) {
            if (flags & ImGuiKnobFlags_Logarithmic && v_min <= 0.0 && v_max >= 0.0) {
                // we must handle the cornercase if a client specifies a logarithmic range that contains zero
                // for this we clamp lower limit to avoid hitting zero like it is done in ImGui::SliderBehaviorT
                const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
                const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 1;
                v_min = ImPow(0.1f, (float) decimal_precision);
                v_max = ImMax(v_min, v_max); // this ensures that in the cornercase v_max is still at least ge v_min
                *p_value = ImMax(ImMin(*p_value, v_max), v_min); // this ensures that in the cornercase p_value is within the range
            }

            auto speed = _speed == 0 ? (v_max - v_min) / 250.f : _speed;
            ImGui::PushID(label);

#if IMGUI_VERSION_NUM < 19197
            auto font_scale = ImGui::GetIO().FontGlobalScale;
#else
            auto font_scale = ImGui::GetStyle().FontScaleMain;
#endif
            auto width = size == 0 ? ImGui::GetTextLineHeight() * 4.0f : size * font_scale;
            ImGui::PushItemWidth(width);

            ImGui::BeginGroup();

            // There's an issue with `SameLine` and Groups, see
            // https://github.com/ocornut/imgui/issues/4190. This is probably not the best
            // solution, but seems to work for now
            ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;

            // Draw title
            if (!(flags & ImGuiKnobFlags_NoTitle)) {
                auto title_size = ImGui::CalcTextSize(label, NULL, false, width);

                // Center title
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     (width - title_size[0]) * 0.5f);

                ImGui::Text("%s", label);
            }

            // Draw knob
            knob<DataType> k(label, data_type, p_value, v_min, v_max, speed, width * 0.5f, format, flags, angle_min, angle_max);

            // Draw tooltip
            if (flags & ImGuiKnobFlags_ValueTooltip &&
                (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) ||
                 ImGui::IsItemActive())) {
                ImGui::BeginTooltip();
                ImGui::Text(format, *p_value);
                ImGui::EndTooltip();
            }

            // Draw input
            if (!(flags & ImGuiKnobFlags_NoInput)) {
                ImGuiSliderFlags drag_scalar_flags = 0;
                if (flags & ImGuiKnobFlags_AlwaysClamp) {
                    drag_scalar_flags |= ImGuiSliderFlags_AlwaysClamp;
                }
                if (flags & ImGuiKnobFlags_Logarithmic) {
                    drag_scalar_flags |= ImGuiSliderFlags_Logarithmic;
                }
                auto changed = ImGui::DragScalar("###knob_drag", data_type, p_value, speed, &v_min, &v_max, format, drag_scalar_flags);
                if (changed) {
                    k.value_changed = true;
                }
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
            auto active = ImVec4(colors[ImGuiCol_ButtonActive].x * 0.5f,
                                 colors[ImGuiCol_ButtonActive].y * 0.5f,
                                 colors[ImGuiCol_ButtonActive].z * 0.5f,
                                 colors[ImGuiCol_ButtonActive].w);

            auto hovered = ImVec4(colors[ImGuiCol_ButtonHovered].x * 0.5f,
                                  colors[ImGuiCol_ButtonHovered].y * 0.5f,
                                  colors[ImGuiCol_ButtonHovered].z * 0.5f,
                                  colors[ImGuiCol_ButtonHovered].w);

            return {active, hovered, hovered};
        }

        color_set GetTrackColorSet() {
            auto *colors = ImGui::GetStyle().Colors;

            return {colors[ImGuiCol_Button], colors[ImGuiCol_Button], colors[ImGuiCol_Button]};
        }
    }// namespace detail

    template<typename DataType>
    bool BaseKnob(
            const char *label,
            ImGuiDataType data_type,
            DataType *p_value,
            DataType v_min,
            DataType v_max,
            float speed,
            const char *format,
            ImGuiKnobVariant variant,
            float size,
            ImGuiKnobFlags flags,
            int steps,
            float angle_min,
            float angle_max) {
        auto knob = detail::knob_with_drag(
                label,
                data_type,
                p_value,
                v_min,
                v_max,
                speed,
                format,
                size,
                flags,
                angle_min,
                angle_max);

        switch (variant) {
            case ImGuiKnobVariant_Tick: {
                knob.draw_circle(0.85f, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_tick(0.5f, 0.85f, 0.08f, knob.angle, detail::GetPrimaryColorSet());
                break;
            }
            case ImGuiKnobVariant_Dot: {
                knob.draw_circle(0.85f, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_dot(0.12f, 0.6f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }

            case ImGuiKnobVariant_Wiper: {
                knob.draw_circle(0.7f, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_arc(0.8f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet());

                if (knob.t > 0.01f) {
                    knob.draw_arc(0.8f, 0.43f, knob.angle_min, knob.angle, detail::GetPrimaryColorSet());
                }
                break;
            }
            case ImGuiKnobVariant_WiperOnly: {
                knob.draw_arc(0.8f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet());

                if (knob.t > 0.01) {
                    knob.draw_arc(0.8f, 0.43f, knob.angle_min, knob.angle, detail::GetPrimaryColorSet());
                }
                break;
            }
            case ImGuiKnobVariant_WiperDot: {
                knob.draw_circle(0.6f, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_arc(0.85f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet());
                knob.draw_dot(0.1f, 0.85f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }
            case ImGuiKnobVariant_Stepped: {
                for (auto n = 0.f; n < steps; n++) {
                    auto a = n / (steps - 1);
                    auto angle = knob.angle_min + (knob.angle_max - knob.angle_min) * a;
                    knob.draw_tick(0.7f, 0.9f, 0.04f, angle, detail::GetPrimaryColorSet());
                }

                knob.draw_circle(0.6f, detail::GetSecondaryColorSet(), true, 32);
                knob.draw_dot(0.12f, 0.4f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
                break;
            }
            case ImGuiKnobVariant_Space: {
                knob.draw_circle(0.3f - knob.t * 0.1f, detail::GetSecondaryColorSet(), true, 16);

                if (knob.t > 0.01f) {
                    knob.draw_arc(0.4f, 0.15f, knob.angle_min - 1.0f, knob.angle - 1.0f, detail::GetPrimaryColorSet());
                    knob.draw_arc(0.6f, 0.15f, knob.angle_min + 1.0f, knob.angle + 1.0f, detail::GetPrimaryColorSet());
                    knob.draw_arc(0.8f, 0.15f, knob.angle_min + 3.0f, knob.angle + 3.0f, detail::GetPrimaryColorSet());
                }
                break;
            }
        }

        return knob.value_changed;
    }

    bool Knob(
            const char *label,
            float *p_value,
            float v_min,
            float v_max,
            float speed,
            const char *format,
            ImGuiKnobVariant variant,
            float size,
            ImGuiKnobFlags flags,
            int steps,
            float angle_min,
            float angle_max) {
        return BaseKnob(
                label,
                ImGuiDataType_Float,
                p_value,
                v_min,
                v_max,
                speed,
                format,
                variant,
                size,
                flags,
                steps,
                angle_min,
                angle_max);
    }

    bool KnobInt(
            const char *label,
            int *p_value,
            int v_min,
            int v_max,
            float speed,
            const char *format,
            ImGuiKnobVariant variant,
            float size,
            ImGuiKnobFlags flags,
            int steps,
            float angle_min,
            float angle_max) {
        return BaseKnob(
                label,
                ImGuiDataType_S32,
                p_value,
                v_min,
                v_max,
                speed,
                format,
                variant,
                size,
                flags,
                steps,
                angle_min,
                angle_max);
    }
}// namespace ImGuiKnobs
