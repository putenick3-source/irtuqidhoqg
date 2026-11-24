#pragma once
#include "ui/state/theme/theme.hpp"
#include "ui/types/ui-layer-alpha/ui-layer-alpha.hpp"
#include "imgui.h"

namespace ui
{
    struct UiTheme
    {
        UiLayerAlpha layer_alpha{};
        ImVec4 background_color{28.0f / 255.0f, 33.0f / 255.0f, 36.0f / 255.0f, 1.0f};
        ImVec4 group_color{12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 72.0f / 255.0f};
        ImVec4 border_color{40.0f / 255.0f, 55.0f / 255.0f, 60.0f / 255.0f, 180.0f / 255.0f};
        ImVec4 text_color{206.0f / 255.0f, 206.0f / 255.0f, 206.0f / 255.0f, 1.0f};
        ImVec4 control_color{36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 190.0f / 255.0f};
        ImVec4 track_color{69.0f / 255.0f, 69.0f / 255.0f, 69.0f / 255.0f, 190.0f / 255.0f};
        ImVec4 checkbox_outline_color{0, 0, 0, 1};
        ImVec4 checkbox_off_color{69.0f / 255.0f, 69.0f / 255.0f, 69.0f / 255.0f, 1};
        ImVec4 checkbox_on_color{0, 181.0f / 255.0f, 200.0f / 255.0f, 1};
        ImVec4 checkbox_on_shade_color{0, 156.0f / 255.0f, 176.0f / 255.0f, 1};
        ImVec4 checkbox_text_color{206.0f / 255.0f, 206.0f / 255.0f, 206.0f / 255.0f, 1};
        ImVec4 slider_border_color{0, 0, 0, 1};
        ImVec4 slider_track_bottom_color{44.0f / 255.0f, 44.0f / 255.0f, 44.0f / 255.0f, 1};
        ImVec4 slider_fill_shade_color{0, 156.0f / 255.0f, 176.0f / 255.0f, 1};
        ImVec4 slider_label_color{206.0f / 255.0f, 206.0f / 255.0f, 206.0f / 255.0f, 1};
        ImVec4 slider_value_color{1, 1, 1, 1};
        ImVec4 slider_button_text_color{210.0f / 255.0f, 210.0f / 255.0f, 210.0f / 255.0f, 1};
        ImVec4 control_border_color{0, 0, 0, 1};
        ImVec4 control_text_color{153.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f, 1};
        ImVec4 control_arrow_color{153.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f, 1};
        ImVec4 button_text_color{206.0f / 255.0f, 206.0f / 255.0f, 206.0f / 255.0f, 1};
        ImVec4 group_black_line_color{0, 0, 0, 1};
        ImVec4 tab_text_color{205.0f / 255.0f, 205.0f / 255.0f, 205.0f / 255.0f, 1};
        ImVec4 tab_idle_line_color{95.0f / 255.0f, 140.0f / 255.0f, 150.0f / 255.0f, 1};
        ImVec4 tab_idle_line_bottom_color{50.0f / 255.0f, 50.0f / 255.0f, 55.0f / 255.0f, 1};
        ImVec4 tab_line_shadow_color{18.0f / 255.0f, 18.0f / 255.0f, 18.0f / 255.0f, 1};
        ImVec4 tab_line_black_color{0, 0, 0, 1};
        ImVec4 outer_shadow_color{0.0f, 0.0f, 0.0f, 150.0f / 255.0f};
        ImVec4 outer_outer_line_color{50.0f / 255.0f, 72.0f / 255.0f, 80.0f / 255.0f, 210.0f / 255.0f};
        ImVec4 outer_between_color{35.0f / 255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 150.0f / 255.0f};
        ImVec4 outer_inner_line_color{50.0f / 255.0f, 72.0f / 255.0f, 80.0f / 255.0f, 190.0f / 255.0f};
        ImVec4 outer_face_color{4.0f / 255.0f, 4.0f / 255.0f, 4.0f / 255.0f, 175.0f / 255.0f};
        ImVec4 panel_outer_line_color{0.0f, 0.0f, 0.0f, 145.0f / 255.0f};
        ImVec4 panel_between_color{44.0f / 255.0f, 44.0f / 255.0f, 44.0f / 255.0f, 145.0f / 255.0f};
        ImVec4 panel_inner_line_color{9.0f / 255.0f, 9.0f / 255.0f, 9.0f / 255.0f, 120.0f / 255.0f};
        ImVec4 panel_inside_color{9.0f / 255.0f, 9.0f / 255.0f, 9.0f / 255.0f, 82.0f / 255.0f};
    };
}
