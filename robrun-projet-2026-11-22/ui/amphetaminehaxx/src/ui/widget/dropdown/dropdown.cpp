#include "ui/types/canvas/canvas.hpp"
#include "ui/color/pack-color/pack-color.hpp"
#include "ui/state/theme/theme.hpp"
#include "ui/text/draw-text-shadow/draw-text-shadow.hpp"
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
    void Canvas::Dropdown(const char* label, int& value, const std::vector<const char*>& items, bool multi, unsigned int* selection_mask)
    {
        const bool has_label = label && std::strlen(label) > 1;
        const float bx = x + 20;
        float by = y;
        y += 25;
        if (has_label)
        {
            Text(bx, by - 3, label, PackColor(slider_label_color, alpha), true);
            by += 15;
            y += 15;
        }
        if (!multi)
            value = std::clamp(value, 0, static_cast<int>(items.size()) - 1);
        ImGui::SetCursorScreenPos(ImVec2(bx, by));
        ImGui::PushID(label);
        ImGui::PushID(static_cast<int>(bx));
        ImGui::PushID(static_cast<int>(by));
        ImGui::InvisibleButton("combo", ImVec2(150, 20));
        const bool open = ImGui::IsItemClicked();
        ImGui::PopID();
        ImGui::PopID();
        ImGui::PopID();
        Rect(bx - 1, by - 1, bx + 151, by + 21, PackColor(control_border_color, alpha));
        Rect(bx, by, bx + 150, by + 20, IM_COL32(static_cast<int>(control_color.x * 255), static_cast<int>(control_color.y * 255), static_cast<int>(control_color.z * 255), static_cast<int>(control_color.w * 255 * alpha)));
        std::string display;
        if (multi && selection_mask)
        {
            for (int i = 0; i < static_cast<int>(items.size()); ++i)
            {
                if ((*selection_mask & (1u << i)) == 0)
                    continue;
                if (!display.empty())
                    display += ", ";
                display += items[i];
            }
            if (display.empty())
                display = "off";
            if (ImGui::CalcTextSize(display.c_str()).x > 105.0f)
            {
                while (!display.empty() && ImGui::CalcTextSize((display + "...").c_str()).x > 105.0f)
                    display.pop_back();
                display += "...";
            }
        }
        else
            display = items[value];
        DrawTextShadow(draw, ImVec2(bx + 10, by + 3), PackColor(control_text_color, alpha), display.c_str());
        const float arrow_x = bx + 139.0f;
        const float arrow_mid_y = by + 10.0f;
        draw->AddTriangleFilled(ImVec2(arrow_x, arrow_mid_y - 2.0f), ImVec2(arrow_x + 6.0f, arrow_mid_y - 2.0f), ImVec2(arrow_x + 3.0f, arrow_mid_y + 2.0f), PackColor(control_arrow_color, alpha));
        ImGui::PushID(label);
        ImGui::PushID(static_cast<int>(bx));
        ImGui::PushID(static_cast<int>(by));
        if (open)
            ImGui::OpenPopup("list");
        if (ImGui::IsPopupOpen("list"))
        {
            ImGui::SetNextWindowPos(ImVec2(bx - 1.0f, by + 21), ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(ImVec2(151, 0), ImGuiCond_Appearing);
            ImGui::SetNextWindowSizeConstraints(ImVec2(151, 0), ImVec2(151, ImGui::GetIO().DisplaySize.y * 0.65f));
        }
         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(151, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, C(36, 36, 36));
        ImGui::PushStyleColor(ImGuiCol_Border, C(0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, C(30, 30, 30));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, C(80, 80, 80));
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));
        ImGui::SetNextWindowSizeConstraints(ImVec2(151, 0), ImVec2(151, ImGui::GetIO().DisplaySize.y * 0.65f));
        if (ImGui::BeginPopup("list", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImDrawList* popup_draw = ImGui::GetWindowDrawList();
            for (int i = 0; i < static_cast<int>(items.size()); ++i)
            {
                const ImVec2 item_pos = ImGui::GetCursorScreenPos();
                ImGui::PushID(i);
                 ImGui::InvisibleButton("row", ImVec2(143, 18));
                const bool hovered = ImGui::IsItemHovered();
                const bool selected = multi && selection_mask ? ((*selection_mask & (1u << i)) != 0) : value == i;
                const bool clicked = ImGui::IsItemClicked();
                ImGui::PopID();
                 popup_draw->AddText(ImVec2(item_pos.x + 6, item_pos.y + 2), selected ? Accent() : hovered ? Accent(140) : C(153, 153, 153), items[i]);
                if (clicked)
                {
                    if (multi && selection_mask)
                        *selection_mask ^= 1u << i;
                    else
                    {
                        value = i;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar(8);
        ImGui::PopID();
        ImGui::PopID();
        ImGui::PopID();
    }
}
