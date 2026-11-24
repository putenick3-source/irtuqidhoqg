#include "ui/types/canvas/canvas.hpp"
#include "menu/settings/aim/aim.hpp"
#include "menu/settings/move/move.hpp"
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
    void Canvas::Key(const char* label, int& key)
    {
        Text(x + 20, y - 3, label, C(206, 206, 206), true);
        const float bx = x + 20;
        const float by = y + 12;
        bool& capturing = &key == &menu::aim_key ? state.aim_key_capture : &key == &menu::fly_key ? state.fly_key_capture : state.trigger_key_capture;
        ImGui::SetCursorScreenPos(ImVec2(bx, by));
        ImGui::PushID(label);
        ImGui::PushID(static_cast<int>(bx));
        ImGui::PushID(static_cast<int>(by));
        ImGui::InvisibleButton("key", ImVec2(150, 20));
        const bool started_capture = ImGui::IsItemClicked();
        if (started_capture)
            capturing = true;
        if (&key == &menu::aim_key && ImGui::IsItemClicked(ImGuiMouseButton_Right) && !capturing)
            menu::aim_mode = (menu::aim_mode + 1) % 3;
        if (&key == &menu::fly_key && ImGui::IsItemClicked(ImGuiMouseButton_Right) && !capturing)
            menu::fly_mode = (menu::fly_mode + 1) % 2;
        Rect(bx - 1, by - 1, bx + 151, by + 21, PackColor(control_border_color, alpha));
        Rect(bx, by, bx + 150, by + 20, IM_COL32(static_cast<int>(control_color.x * 255), static_cast<int>(control_color.y * 255), static_cast<int>(control_color.z * 255), static_cast<int>(control_color.w * 255 * alpha)));
        const char* key_name = key == 0 ? "none" : ImGui::GetKeyName(static_cast<ImGuiKey>(key));
        if (!key_name || !key_name[0])
            key_name = "none";
        char key_buf[64];
        const char* shown = "...";
        if (!capturing)
        {
            if (&key == &menu::aim_key)
            {
                const char* mode = menu::aim_mode == 1 ? "always on" : menu::aim_mode == 2 ? "toggle" : "hold";
                std::snprintf(key_buf, sizeof(key_buf), "%s [%s]", key_name, mode);
                shown = key_buf;
            }
            else if (&key == &menu::fly_key)
            {
                std::snprintf(key_buf, sizeof(key_buf), "%s [%s]", key_name, menu::fly_mode ? "toggle" : "hold");
                shown = key_buf;
            }
            else
                shown = key_name;
        }
        DrawTextShadow(draw, ImVec2(bx + 10, by + 3), PackColor(control_text_color, alpha), shown);
        if (capturing && !started_capture)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                key = 0;
                capturing = false;
            }
            else
            {
                for (int i = 0; i < 5; ++i)
                {
                    if (!ImGui::IsMouseClicked(i))
                        continue;
                    key = ImGuiKey_MouseLeft + i;
                    capturing = false;
                    break;
                }
                for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i)
                {
                    if (!capturing)
                        break;
                    const ImGuiKey candidate = static_cast<ImGuiKey>(i);
                    if (candidate >= ImGuiKey_GamepadStart && candidate <= ImGuiKey_GamepadRStickDown)
                        continue;
                    if (ImGui::IsKeyPressed(candidate))
                    {
                        key = i;
                        capturing = false;
                        break;
                    }
                }
            }
        }
        ImGui::PopID();
        ImGui::PopID();
        ImGui::PopID();
        y += 40;
    }
}
