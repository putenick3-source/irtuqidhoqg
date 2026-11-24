#include "ui/tab/draw-world/draw-world.hpp"
#include "ui/eternal/eternal_state.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"
#include <vector>

static const std::vector<const char*> g_world_cham_styles =
{ "flat", "ice", "plasma", "ember", "obsidian", "abyss", "pulse", "invisible", "marble" };

namespace ui {
void DrawWorld(Canvas& c, UiState& state) {
    (void)state;
    eternal::Settings& s = eternal_ui::settings();
    using amphetaminehaxx::check;
    using amphetaminehaxx::color;
    auto tg = [&](const char* label, bool& value, int slot = -1) {
        int v = value ? 1 : 0;
        check(c, label, v, slot);
        value = v != 0;
    };
    c.BeginColumn(false);
    c.BeginGroup("light and sky");
    tg("boost sun", s.world_sun);
    c.Slider("sun power", s.world_sun_pct, 100, 800, "%");
    tg("disable sky", s.world_sky_off);
    tg("custom sky color", s.world_sky_color, BR_COLOR_SKY);
    c.Slider("sky glow", s.world_sky_glow, 0, 300, "%");
    c.EndGroup();
    c.BeginGroup("fog and weather");
    tg("disable fog", s.world_fog_off);
    tg("custom fog", s.world_fog_custom, BR_COLOR_FOG);
    c.Slider("fog density", s.world_fog_density, 1, 200);
    c.Slider("fog distance", s.world_fog_distance, 0, 1000, " m");
    c.Slider("rain", s.world_rain, 0, 100, "%");
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("your player");
    tg("box", s.local_box, BR_COLOR_LOCAL_BOX);
    tg("skeleton", s.local_skeleton, BR_COLOR_LOCAL_SKELETON);
    tg("name", s.local_name, BR_COLOR_LOCAL_NAME);
    tg("weapon", s.local_weapon, BR_COLOR_LOCAL_WEAPON);
    tg("distance", s.local_distance, BR_COLOR_LOCAL_DISTANCE);
    tg("armor", s.local_armor, BR_COLOR_LOCAL_ARMOR);
    c.EndGroup();
    c.BeginGroup("local chams");
#ifdef RR_INTERNAL
    tg("tung tung model", s.tung_model);
#endif
    tg("local chams", s.local_chams, BR_COLOR_LOCAL_CHAM);
    tg("tint", s.local_chams_tint);
    c.Dropdown("style", s.local_chams_style, g_world_cham_styles);
    c.Slider("opacity", s.local_chams_opacity, 0, 100, "%");
    c.EndGroup();
}
}
