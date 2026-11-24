#include "ui/tab/draw-style/draw-style.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"

namespace ui {
void DrawStyle(Canvas& c, UiState& state) {
    auto& b = *br_get();
    using amphetaminehaxx::check;
    using amphetaminehaxx::color;
    c.BeginColumn(false);
    c.BeginGroup("hud");
    check(c,"watermark",b.hud_watermark);
    check(c,"custom cursor",b.hud_cursor);
    amphetaminehaxx::theme_controls(c,state);
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("profiles");
    amphetaminehaxx::profiles(c);
    c.EndGroup();
}
}
