#include "ui/types/canvas/canvas.hpp"
#include "ui/color/pack-color/pack-color.hpp"
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
    void Canvas::Slider(const char* label, int& value, int min, int max, const char* suffix)
    {
        const bool has_label = label && std::strlen(label) > 1;
        if (has_label)
        {
            Text(x + 20, y - 3, label, PackColor(slider_label_color, alpha), true);
            y += 15;
        }
        const float sx = x + 20;
        const float sy = y;
        const float width = 150;
        value = std::clamp(value, min, max);
        ImGui::PushID(label ? label : "slider");
         const bool minus = Click(sx - 22, sy - 4, 16, 15, "minus");
         const bool plus = Click(sx + width + 6, sy - 4, 16, 15, "plus");
        if (minus && value > min)
            --value;
        if (plus && value < max)
            ++value;
        Click(sx - 1, sy - 1, width + 2, 8, "slider-track");
        if (ImGui::IsItemActive() && max > min)
            value = min + static_cast<int>((max - min) * std::clamp((ImGui::GetIO().MousePos.x - sx) / width, 0.0f, 1.0f));
        const float pos = max == min ? 0 : width * static_cast<float>(value - min) / static_cast<float>(max - min);
        Text(sx - 18, sy - 4, "-", PackColor(slider_button_text_color, alpha));
        Text(sx + width + 10, sy - 4, "+", PackColor(slider_button_text_color, alpha));
        Rect(sx - 1, sy - 1, sx + width + 1, sy + 7, PackColor(slider_border_color, alpha));
        const ImU32 track = IM_COL32(static_cast<int>(track_color.x * 255), static_cast<int>(track_color.y * 255), static_cast<int>(track_color.z * 255), static_cast<int>(track_color.w * 255 * alpha));
        draw->AddRectFilledMultiColor(ImVec2(sx, sy), ImVec2(sx + width, sy + 6), track, track, PackColor(slider_track_bottom_color, alpha), PackColor(slider_track_bottom_color, alpha));
        if (pos > 0.0f)
            draw->AddRectFilledMultiColor(ImVec2(sx, sy), ImVec2(sx + pos, sy + 6), Accent(), Accent(), PackColor(slider_fill_shade_color, alpha), PackColor(slider_fill_shade_color, alpha));
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%d%s", value, suffix);
        const float text_w = ImGui::CalcTextSize(buf).x;
        const float plus_x = sx + width + 10.0f;
        const float center_x = sx + pos;
        const float left_x = std::min(center_x - text_w * 0.5f, plus_x - text_w - 1.0f);
        Text(std::max(sx + 1.0f, left_x), sy, buf, PackColor(slider_value_color, alpha));
        y += 15;
        ImGui::PopID();
    }
}
