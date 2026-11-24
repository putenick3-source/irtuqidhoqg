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
    void DrawTextShadow(ImDrawList* draw, ImVec2 pos, ImU32 col, const char* text)
    {
        draw->AddText(ImVec2(std::round(pos.x + 1.0f), std::round(pos.y + 1.0f)), IM_COL32(0, 0, 0, 220), text);
        draw->AddText(ImVec2(std::round(pos.x), std::round(pos.y)), col, text);
    }
}
