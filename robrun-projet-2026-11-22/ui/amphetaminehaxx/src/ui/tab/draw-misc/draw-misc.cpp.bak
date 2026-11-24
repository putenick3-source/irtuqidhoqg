#include "ui/tab/draw-misc/draw-misc.hpp"
#include "rr_controls.hpp"
#include "game/rr_game.h"

namespace ui {
void DrawMisc(Canvas& c, UiState& state) {
    auto& b = *br_get();
    using amphetaminehaxx::check;
    using amphetaminehaxx::color;
    (void)state;
    c.BeginColumn(false);
    c.BeginGroup("world markers");
    check(c,"loot",b.show_loot);
    check(c,"loot nodes",b.show_loot_nodes);
    check(c,"containers",b.show_containers);
    check(c,"doors",b.show_doors);
    check(c,"dropped loot",b.show_dropped_loot);
    check(c,"vehicle loot",b.show_vehicle_loot);
    check(c,"vehicle markers",b.show_vehicle_markers);
    check(c,"occupants",b.show_vehicle_occupants);
#ifndef RR_INTERNAL
    check(c,"vehicle chams",b.vehicle_chams);
#endif
    c.EndGroup();
    c.BeginColumn(true);
    c.BeginGroup("loot rarities");
    check(c,"no rarity",b.loot_tier_none);
    check(c,"common",b.loot_tier_common);
    check(c,"uncommon",b.loot_tier_uncommon);
    check(c,"rare",b.loot_tier_rare);
    check(c,"epic",b.loot_tier_epic);
    check(c,"legendary",b.loot_tier_legendary);
    check(c,"exotic",b.loot_tier_exotic);
    c.EndGroup();
}
}
