#include "ui/tab/draw-loot/draw-loot.hpp"
#include "ui/eternal/eternal_state.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"
#include <vector>

static const std::vector<const char*> g_loot_cham_styles =
{ "flat", "ice", "plasma", "ember", "obsidian", "abyss", "pulse", "invisible", "marble" };

namespace ui {
void DrawLoot(Canvas& c, UiState& state) {
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
    c.BeginGroup("items on the ground");
    tg("loot", s.loot_on);
    tg("loot nodes", s.loot_nodes);
    tg("containers", s.loot_containers);
    tg("doors", s.loot_doors);
    tg("dropped items", s.loot_dropped);
    tg("vehicle loot", s.loot_vehicle);
    c.EndGroup();
    c.BeginGroup("rarities");
    tg("no rarity", s.loot_t_none);
    tg("common", s.loot_t_common);
    tg("uncommon", s.loot_t_uncommon);
    tg("rare", s.loot_t_rare);
    tg("epic", s.loot_t_epic);
    tg("legendary", s.loot_t_legendary);
    tg("exotic", s.loot_t_exotic);
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("vehicles");
    tg("vehicle markers", s.vehicle_markers);
    tg("occupants", s.vehicle_occupants);
    c.EndGroup();
    c.BeginGroup("vehicle chams");
    tg("vehicle chams", s.vehicle_chams);
    c.Dropdown("style", s.vehicle_chams_style, g_loot_cham_styles);
    c.Slider("opacity", s.vehicle_chams_opacity, 0, 100, "%");
    c.Label("cham color"); color(c, "cham color", BR_COLOR_VEHICLE_CHAM);
    c.EndGroup();
}
}
