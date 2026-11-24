#include "ui/tab/draw-misc/draw-misc.hpp"
#include "ui/eternal/eternal_state.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"

namespace ui {
void DrawMisc(Canvas& c, UiState& state) {
    (void)state;
    eternal::Settings& s = eternal_ui::settings();
    using amphetaminehaxx::check;
    using amphetaminehaxx::key;
    auto tg = [&](const char* label, bool& value, int slot = -1) {
        int v = value ? 1 : 0;
        check(c, label, v, slot);
        value = v != 0;
    };
    c.BeginColumn(false);
    c.BeginGroup("screen");
    tg("custom crosshair", s.misc_crosshair);
    tg("watermark", s.misc_watermark);
    tg("tracers", s.tracers, BR_COLOR_TRACER);
    c.Slider("tracer duration", s.tracer_ms, 100, 4000, " ms");
    c.Slider("tracer width", s.tracer_width, 5, 40);
    c.Slider("tracer glow", s.tracer_glow, 0, 20);
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("interface");
    int mk = (int)s.menu_key;
    key(c, "open / close", mk, true);
    s.menu_key = (ImGuiKey)mk;
    c.Slider("opacity", s.opacity, 40, 100, "%");
    c.EndGroup();
}
}
