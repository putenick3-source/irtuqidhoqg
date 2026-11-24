#pragma once
#include "ui/types/ui-state/ui-state.hpp"
#include <vector>
#include <string>
#include "imgui.h"

namespace ui
{
    struct Canvas
    {
        ImDrawList* draw;
        UiState& state;
        float alpha;
        float x;
        float y;
        float group_x;
        float group_y;
        float title_width;
        ImVec2 state_pos;
        float scale_x;
        float scale_y;
        std::string group_title;
        float content_bottom = 0.0f;

        ImU32 C(int r, int g, int b, int a = 255) const;
        ImU32 Accent(int a = 255) const;
        void Rect(float x0, float y0, float x1, float y1, ImU32 col);
        void Text(float x0, float y0, const char* text, ImU32 col, bool = false);
        bool Hit(float x0, float y0, float w, float h) const;
        bool Click(float x0, float y0, float w, float h, const char* id, const void* salt = nullptr);
        void BeginColumn(bool right);
        void BeginGroup(const char* title);
        void EndGroup();
        void Label(const char* text);
        void Check(const char* label, bool& value, bool compact = false);
        void Slider(const char* label, int& value, int min, int max, const char* suffix = "");
        void Button(const char* label, bool& pressed);
        void ColorPicker(const char* id, float color[4]);
        void Dropdown(const char* label, int& value, const std::vector<const char*>& items, bool multi = false, unsigned int* selection_mask = nullptr);
        void Key(const char* label, int& key);
    };
}
