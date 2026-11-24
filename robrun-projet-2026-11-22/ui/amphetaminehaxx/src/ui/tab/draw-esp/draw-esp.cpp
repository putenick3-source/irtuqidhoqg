#include "ui/tab/draw-esp/draw-esp.hpp"
#include "ui/eternal/eternal_state.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"
#include <vector>

static const std::vector<const char*> g_esp_cham_styles =
{ "flat", "ice", "plasma", "ember", "obsidian", "abyss", "pulse", "invisible", "marble" };
static const std::vector<const char*> g_esp_oof_styles = { "arrow", "chevron", "triangle" };

namespace ui {
void DrawEsp(Canvas& c, UiState& state) {
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
    c.BeginGroup("players");
    tg("enabled", s.esp_enabled);
    tg("box", s.esp_box, BR_COLOR_BOX);
    tg("skeleton", s.esp_skeleton, BR_COLOR_SKELETON);
    tg("name", s.esp_name, BR_COLOR_NAME);
    tg("weapon", s.esp_weapon, BR_COLOR_WEAPON);
    tg("distance", s.esp_distance, BR_COLOR_DISTANCE);
    tg("health bar", s.esp_health);
    tg("armor", s.esp_armor, BR_COLOR_ARMOR);
    tg("team tag", s.esp_team);
    tg("clan tag", s.esp_clan);
    tg("police tag", s.esp_police);
    tg("show self", s.esp_self);
    c.EndGroup();
    c.BeginGroup("player style");
    c.Slider("box thickness", s.esp_thickness, 1, 4, " px");
    c.Slider("skeleton width", s.esp_skeleton_width, 1, 4, " px");
    c.Slider("max distance", s.esp_max_dist, 0, 5000, " m");
    tg("health gradient", s.esp_health_gradient);
    c.EndGroup();
    c.BeginGroup("colors");
    c.Label("box"); color(c, "box", BR_COLOR_BOX);
    c.Label("name"); color(c, "name", BR_COLOR_NAME);
    c.Label("weapon"); color(c, "weapon", BR_COLOR_WEAPON);
    c.Label("distance"); color(c, "distance", BR_COLOR_DISTANCE);
    c.Label("skeleton"); color(c, "skeleton", BR_COLOR_SKELETON);
    c.Label("armor"); color(c, "armor", BR_COLOR_ARMOR);
    c.Label("hp high"); color(c, "hp high", BR_COLOR_HP_HIGH);
    c.Label("hp mid"); color(c, "hp mid", BR_COLOR_HP_MID);
    c.Label("hp low"); color(c, "hp low", BR_COLOR_HP_LOW);
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("chams");
#ifdef RR_INTERNAL
    tg("overlay chams", s.cham_on, BR_COLOR_OVERLAY_CHAM);
    tg("visibility check", s.cham_vis_check);
#else
    tg("overlay chams", s.cham_on);
#endif
    tg("tint", s.cham_tint);
    c.Dropdown("style", s.cham_style, g_esp_cham_styles);
    c.Slider("opacity", s.cham_opacity, 0, 100, "%");
    tg("materials", s.cham_materials);
    c.EndGroup();
    c.BeginGroup("team colors");
    c.Label("criminal"); color(c, "criminal", BR_COLOR_CRIMINAL);
    c.Label("police"); color(c, "police", BR_COLOR_POLICE);
    c.Label("civilian"); color(c, "civilian", BR_COLOR_CIVILIAN);
    c.EndGroup();
    c.BeginGroup("radar and arrows");
    tg("radar", s.radar_on);
    tg("rotate with world", s.radar_world);
    c.Slider("radar range", s.radar_range, 25, 500, " m");
    tg("vehicle markers", s.vehicle_markers);
    tg("vehicle occupants", s.vehicle_occupants);
    tg("edge arrows", s.oof_on, BR_COLOR_OOF);
    c.Dropdown("arrow style", s.oof_style, g_esp_oof_styles);
    c.Slider("arrow size", s.oof_size, 3, 25, " px");
    c.Slider("arrow radius", s.aim_fov, 30, 600, " px");
    s.oof_radius = s.aim_fov;
    c.EndGroup();
}
}
