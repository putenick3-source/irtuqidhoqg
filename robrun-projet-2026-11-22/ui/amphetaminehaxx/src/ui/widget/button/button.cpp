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
    void Canvas::Button(const char* label, bool& pressed)
    {
        const float bx = x + 20;
        const float by = y;
        if (Click(bx, by, 150, 20, label))
            pressed = true;
        Rect(bx - 1, by - 1, bx + 151, by + 21, PackColor(control_border_color, alpha));
        Rect(bx, by, bx + 150, by + 20, IM_COL32(static_cast<int>(control_color.x * 255), static_cast<int>(control_color.y * 255), static_cast<int>(control_color.z * 255), static_cast<int>(control_color.w * 255 * alpha)));
        const ImVec2 size = ImGui::CalcTextSize(label);
        Text(bx + (150.0f - size.x) * 0.5f, by + (20.0f - size.y) * 0.5f, label, PackColor(button_text_color, alpha));
        y += 25;
    }
}
