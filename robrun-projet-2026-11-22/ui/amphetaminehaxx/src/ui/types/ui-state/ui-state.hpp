#pragma once
#include "ui/types/tab/tab.hpp"
#include "imgui.h"

namespace ui
{
    struct UiState
    {
        ImFont* brand_font = nullptr;
        Tab tab = Tab::Visual;
        bool menu_open = false;
        bool aim_key_capture = false;
        bool trigger_key_capture = false;
        bool fly_key_capture = false;
        int fade_speed = 5;
        int aim_key = 0;
        int trigger_key = 0;
        float accent[3] = { 181.0f / 255.0f, 0.0f, 41.0f / 255.0f };
        ImVec2 window_pos = ImVec2(100.0f, 100.0f);
        float content_scroll = 0.0f;
        float content_height = 0.0f;
        ImVec2 drag_offset = ImVec2(0.0f, 0.0f);
        bool dragging = false;
    };
}
