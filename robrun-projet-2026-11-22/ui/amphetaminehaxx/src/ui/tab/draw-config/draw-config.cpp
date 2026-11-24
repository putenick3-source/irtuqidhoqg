#include "ui/tab/draw-config/draw-config.hpp"
#include "ui/eternal/eternal_state.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"

namespace ui {
void DrawConfig(Canvas& c, UiState& state) {
    eternal::Settings& s = eternal_ui::settings();
    c.BeginColumn(false);
    c.BeginGroup("theme");
    amphetaminehaxx::theme_controls(c, state);
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("profiles");
    amphetaminehaxx::profiles(c);
    c.EndGroup();
    c.BeginGroup("settings file");
    c.Button("save", s.save_requested);
    c.Button("load", s.load_requested);
    c.EndGroup();
}
}
