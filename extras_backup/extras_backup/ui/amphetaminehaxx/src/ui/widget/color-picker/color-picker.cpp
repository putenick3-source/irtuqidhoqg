#include "ui/types/canvas/canvas.hpp"
#include "ui/color/pack-color/pack-color.hpp"
#include "ui/frame/draw-window-layers/draw-window-layers.hpp"
#include "ui/state/theme/theme.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "imgui.h"

namespace ui
{
    void Canvas::ColorPicker(const char* id, float color[4])
    {
        struct PickerState
        {
            bool open = false;
            bool held = false;
            bool held_alpha = false;
            float color[3]{};
            ImVec2 pos{};
            ImVec2 size{179.0f, 179.0f};
            ImVec2 marker{-1.0f, -1.0f};
        };
        static std::unordered_map<std::string, PickerState> pickers;
        PickerState& picker = pickers[id];
        constexpr float bar_h = 8.0f;
        constexpr float pad = 10.0f;
        const float sx = x + 150.0f;
        const float sy = y - 15.0f;
        ImGui::SetCursorScreenPos(ImVec2(sx, sy));
        ImGui::PushID(id);
        if (ImGui::InvisibleButton("swatch", ImVec2(20.0f, 8.0f)))
        {
            picker.open = !picker.open;
            picker.pos = ImVec2(sx, sy + 10.0f);
            picker.color[0] = color[0];
            picker.color[1] = color[1];
            picker.color[2] = color[2];
            if (picker.marker.x < 0.0f)
            {
                float h, s, v;
                ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], h, s, v);
                picker.marker = ImVec2(h * picker.size.x, picker.size.y * 0.5f * (v < 1.0f ? 2.0f - v : s));
            }
        }
        Rect(sx, sy, sx + 20.0f, sy + 8.0f, IM_COL32(0, 0, 0, static_cast<int>(alpha * 255.0f)));
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 2; ++j)
                Rect(sx + 1.0f + i * 3.0f, sy + 1.0f + j * 3.0f, sx + 4.0f + i * 3.0f, sy + 4.0f + j * 3.0f, (i + j) & 1 ? C(153, 153, 153) : C(255, 255, 255));
        Rect(sx + 1.0f, sy + 1.0f, sx + 19.0f, sy + 7.0f, IM_COL32(static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f), static_cast<int>(color[2] * 255.0f), static_cast<int>(alpha * color[3] * 255.0f)));
        if (picker.open)
        {
            ImGui::SetNextWindowPos(picker.pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(picker.size.x + 2.0f + pad * 2.0f, picker.size.y + bar_h + 3.0f + pad * 2.0f), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, C(16, 16, 16));
            ImGui::Begin("picker_window", &picker.open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
            ImDrawList* picker_draw = ImGui::GetForegroundDrawList();
            const ImVec2 picker_min = ImGui::GetWindowPos();
            const ImVec2 picker_max(picker_min.x + picker.size.x + 2.0f + pad * 2.0f, picker_min.y + picker.size.y + bar_h + 3.0f + pad * 2.0f);
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const bool mouse_in_picker = mouse.x >= picker_min.x && mouse.x <= picker_max.x && mouse.y >= picker_min.y && mouse.y <= picker_max.y;
            const bool mouse_in_swatch = mouse.x >= sx && mouse.x <= sx + 20.0f && mouse.y >= sy && mouse.y <= sy + 8.0f;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mouse_in_picker && !mouse_in_swatch)
                picker.open = false;
            if (!picker.open)
            {
                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(3);
                ImGui::PopID();
                return;
            }
            picker_draw->AddRectFilled(picker_min, picker_max, PackColor(background_color, 1.0f));
            DrawWindowLayers(picker_draw, picker_min, picker_max, theme, 1.0f);
            picker_draw->AddRectFilled(ImVec2(picker_min.x + pad, picker_min.y + pad), ImVec2(picker_max.x - pad, picker_max.y - pad), IM_COL32(0, 0, 0, 255));
            const ImVec2 palette_min(picker_min.x + pad + 1.0f, picker_min.y + pad + 1.0f);
            const ImVec2 palette_max(palette_min.x + picker.size.x, palette_min.y + picker.size.y);
            ImGui::SetCursorScreenPos(palette_min);
            ImGui::InvisibleButton("palette_area", picker.size);
            const bool palette_clicked = ImGui::IsItemClicked();
            for (int i = 0; i < static_cast<int>(picker.size.x); ++i)
            {
                ImVec4 hue_color;
                ImGui::ColorConvertHSVtoRGB(static_cast<float>(i) / picker.size.x, 1.0f, 1.0f, hue_color.x, hue_color.y, hue_color.z);
                const ImU32 top = IM_COL32(255, 255, 255, 255);
                const ImU32 middle = IM_COL32(static_cast<int>(hue_color.x * 255.0f), static_cast<int>(hue_color.y * 255.0f), static_cast<int>(hue_color.z * 255.0f), 255);
                const float px = palette_min.x + static_cast<float>(i);
                picker_draw->AddRectFilledMultiColor(ImVec2(px, palette_min.y), ImVec2(px + 1.0f, palette_min.y + picker.size.y * 0.5f), top, top, middle, middle);
                picker_draw->AddRectFilledMultiColor(ImVec2(px, palette_min.y + picker.size.y * 0.5f), ImVec2(px + 1.0f, palette_max.y), middle, middle, IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
            }
            if (palette_clicked)
                picker.held = true;
            if (picker.held && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                const float sample_x = std::clamp(mouse.x, palette_min.x, palette_max.x);
                const float sample_y = std::clamp(mouse.y, palette_min.y, palette_max.y);
                picker.marker = ImVec2(sample_x - palette_min.x, sample_y - palette_min.y);
                const float hue = std::clamp((sample_x - palette_min.x) / picker.size.x, 0.0f, 1.0f);
                const float half = picker.size.y * 0.5f;
                const float sat = sample_y - palette_min.y > half ? 1.0f : std::clamp((sample_y - palette_min.y) / half, 0.0f, 1.0f);
                const float val = sample_y - palette_min.y > half ? 1.0f - std::clamp((sample_y - palette_min.y - half) / half, 0.0f, 1.0f) : 1.0f;
                ImGui::ColorConvertHSVtoRGB(hue, sat, val, color[0], color[1], color[2]);
                picker.color[0] = color[0];
                picker.color[1] = color[1];
                picker.color[2] = color[2];
            }
            if (picker.held && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                color[0] = picker.color[0];
                color[1] = picker.color[1];
                color[2] = picker.color[2];
                picker.held = false;
            }
            const ImVec2 mark(palette_min.x + picker.marker.x, palette_min.y + picker.marker.y);
            picker_draw->PushClipRect(palette_min, palette_max, true);
            picker_draw->AddCircle(mark, 4.5f, IM_COL32(0, 0, 0, 255), 0, 3.0f);
            picker_draw->AddCircle(mark, 4.5f, IM_COL32(255, 255, 255, 255), 0, 1.0f);
            picker_draw->PopClipRect();
            const ImVec2 bar_min(palette_min.x, palette_max.y + 1.0f);
            const ImVec2 bar_max(palette_max.x, bar_min.y + bar_h);
            ImGui::SetCursorScreenPos(bar_min);
            ImGui::InvisibleButton("alpha_area", ImVec2(picker.size.x, bar_h));
            if (ImGui::IsItemClicked())
                picker.held_alpha = true;
            if (picker.held_alpha)
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    color[3] = std::clamp((mouse.x - bar_min.x - 1.5f) / (picker.size.x - 3.0f), 0.0f, 1.0f);
                else
                    picker.held_alpha = false;
            }
            for (int i = 0; i * 4 < static_cast<int>(picker.size.x); ++i)
                for (int j = 0; j < 2; ++j)
                {
                    const ImVec2 cell_min(bar_min.x + i * 4.0f, bar_min.y + j * 4.0f);
                    picker_draw->AddRectFilled(cell_min, ImVec2(std::min(cell_min.x + 4.0f, bar_max.x), cell_min.y + 4.0f), (i + j) & 1 ? IM_COL32(153, 153, 153, 255) : IM_COL32(255, 255, 255, 255));
                }
            const ImU32 opaque = ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], 1.0f));
            const ImU32 transparent = opaque & ~IM_COL32_A_MASK;
            picker_draw->AddRectFilledMultiColor(bar_min, bar_max, transparent, opaque, opaque, transparent);
            const float handle_x = std::round(bar_min.x + color[3] * (picker.size.x - 3.0f));
            picker_draw->AddRectFilled(ImVec2(handle_x, bar_min.y), ImVec2(handle_x + 3.0f, bar_max.y), IM_COL32(0, 0, 0, 255));
            picker_draw->AddRectFilled(ImVec2(handle_x + 1.0f, bar_min.y), ImVec2(handle_x + 2.0f, bar_max.y), IM_COL32(255, 255, 255, 255));
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);
        }
        ImGui::PopID();
    }
}
