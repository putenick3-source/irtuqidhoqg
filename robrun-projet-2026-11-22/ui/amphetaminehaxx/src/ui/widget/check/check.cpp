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
    void Canvas::Check(const char* label, bool& value, bool compact)
    {
        const float sx = x;
        const float sy = y;
        const float tw = label ? ImGui::CalcTextSize(label).x : 0;
        const float hitw = compact ? 120.0f : 25.0f + tw;
        if (Click(sx, sy - 3, hitw, 14, label ? label : "small", &value))
            value = !value;
        Rect(sx, sy, sx + 8, sy + 8, PackColor(checkbox_outline_color, alpha));
        Rect(sx + 1, sy + 1, sx + 7, sy + 7, PackColor(checkbox_off_color, alpha));
        if (value)
        {
            draw->AddRectFilledMultiColor(ImVec2(sx + 1, sy + 1), ImVec2(sx + 7, sy + 7), PackColor(checkbox_on_color, alpha), PackColor(checkbox_on_color, alpha), PackColor(checkbox_on_shade_color, alpha), PackColor(checkbox_on_shade_color, alpha));
        }
        if (label && !compact)
            Text(std::round(sx + 20), std::round(sy - 4), label, PackColor(checkbox_text_color, alpha), true);
        if (!(compact && !label))
            y += 15.0f;
    }
}
