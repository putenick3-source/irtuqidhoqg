/* menu_bridge.h - Shared integer settings, limits, colors and menu telemetry.
 * BR_SETTINGS is the schema used for declarations, defaults and validation.
 */
#ifndef MENU_BRIDGE_H
#define MENU_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#define BR_MAGIC 0x57444D31u
#ifdef RR_INTERNAL
#define BR_VERSION 23u
#else
#define BR_VERSION 22u
#endif
#define BR_DEFAULT_MENU_KEY 0x2D  /* VK_INSERT */
#define BR_ESP_TOGGLE_KEY 0x24    /* VK_HOME */
#define BR_MAX_DISTANCE_METRES 5000
enum {
    BR_COLOR_BOX = 0, BR_COLOR_DOWN = 5, BR_COLOR_NAME, BR_COLOR_WEAPON,
    BR_COLOR_DISTANCE, BR_COLOR_ARMOR, BR_COLOR_FOV = 14,
    BR_COLOR_SKELETON, BR_COLOR_HP_HIGH = 17, BR_COLOR_HP_MID,
    BR_COLOR_HP_LOW, BR_COLOR_CRIMINAL, BR_COLOR_POLICE, BR_COLOR_CIVILIAN,
    BR_COLOR_FOV_FILL, BR_COLOR_FOV_GRADIENT, BR_COLOR_CHAM,
    BR_COLOR_TRACER, BR_COLOR_OOF, BR_COLOR_SKY, BR_COLOR_FOG,
    BR_COLOR_LOCAL_CHAM, BR_COLOR_LOCAL_BOX, BR_COLOR_LOCAL_SKELETON,
    BR_COLOR_LOCAL_NAME, BR_COLOR_LOCAL_WEAPON, BR_COLOR_LOCAL_DISTANCE, BR_COLOR_LOCAL_ARMOR,
#ifdef RR_INTERNAL
    BR_COLOR_OVERLAY_CHAM, BR_COLOR_LOCAL_NATIVE_CHAM, BR_COLOR_VEHICLE_CHAM,
#endif
    BR_COLOR_COUNT
};

/* Chams fill materials, in menu order. rr_draw.c shades per vertex from the
 * projected hull; the enum is shared so the combo index and the shader agree. */
#define BR_CHAM_FLAT 0
#define BR_CHAM_ICE 1
#define BR_CHAM_PLASMA 2
#define BR_CHAM_EMBER 3
#define BR_CHAM_OBSIDIAN 4
#define BR_CHAM_ABYSS 5
#define BR_CHAM_PULSE 6
#define BR_CHAM_INVISIBLE 7
#define BR_CHAM_MARBLE 8
#define BR_CHAM_STYLE_COUNT 9

#ifdef RR_INTERNAL
#define BR_INTERNAL_SETTINGS(X) \
    X(local_mat_chams, 0, 0, 1) \
    X(tung_tung_model, 1, 0, 1) \
    X(cham_vis_check, 0, 0, 1) \
    X(vehicle_chams_style, BR_CHAM_FLAT, 0, BR_CHAM_STYLE_COUNT - 1) \
    X(vehicle_chams_opacity, 60, 0, 100)
#else
#define BR_INTERNAL_SETTINGS(X)
#endif

/* Shared schema for Amphetaminehaxx, defaults and on-disk settings.
 * Each entry is a field, default, minimum and maximum. */
#define BR_SETTINGS(X) \
    X(enabled, 1, 0, 1) \
    X(show_box, 1, 0, 1) \
    X(show_skeleton, 1, 0, 1) \
    X(show_name, 1, 0, 1) \
    X(show_team, 1, 0, 1) \
    X(show_armor, 1, 0, 1) \
    X(show_distance, 1, 0, 1) \
    X(show_weapon, 1, 0, 1) \
    X(show_self, 0, 0, 1) \
    X(show_clan, 1, 0, 1) \
    X(show_police, 1, 0, 1) \
    X(cham_on, 0, 0, 1) \
    X(cham_tint, 1, 0, 1) \
    X(cham_opacity, 60, 0, 100) \
    X(cham_style, BR_CHAM_FLAT, 0, BR_CHAM_STYLE_COUNT - 1) \
    X(mat_chams, 0, 0, 1) \
    X(max_distance, 30000, 0, BR_MAX_DISTANCE_METRES * 100) \
    X(hud_watermark, 1, 0, 1) \
    X(hud_cursor, 0, 0, 1) \
    X(line_w, 1, 1, 4) \
    X(radar_on, 1, 0, 1) \
    X(radar_range, 150, 25, 500) \
    X(aa_on, 0, 0, 1) \
    X(aa_mode, 0, 0, 2) \
    X(aa_show_fov, 1, 0, 1) \
    X(aa_fov_fill, 0, 0, 1) \
    X(aa_fov_gradient, 0, 0, 1) \
    X(aa_fov_opacity, 18, 0, 100) \
    X(aa_fov_spin, 0, 0, 1) \
    X(aa_fov_spin_speed, 90, 15, 360) \
    X(aa_fov_px, 150, 30, 600) \
    X(aa_key, 0, 0, 255) \
    X(aa_bind_mode, 2, 0, 2) \
    X(aa_smooth, 5, 0, 20) \
    X(aa_aim_cal, 100, 10, 2000) \
    X(aa_dist, 0, 0, 1000) \
    X(aa_target_clan, 0, 0, 1) \
    X(aa_target_police, 0, 0, 1) \
    X(aa_team_check, 1, 0, 1) \
    X(aa_bone, 0, 0, 9) \
    X(aa_los_on, 0, 0, 1) \
    X(aa_closest_first, 0, 0, 1) \
    X(aa_sticky, 0, 0, 1) \
    X(cam_fov, 0, 0, 160) \
    X(show_loot, 0, 0, 1) \
    X(show_loot_nodes, 0, 0, 1) \
    X(loot_tier_none, 1, 0, 1) \
    X(loot_tier_common, 1, 0, 1) \
    X(loot_tier_uncommon, 1, 0, 1) \
    X(loot_tier_rare, 1, 0, 1) \
    X(loot_tier_epic, 1, 0, 1) \
    X(loot_tier_legendary, 1, 0, 1) \
    X(loot_tier_exotic, 1, 0, 1) \
    X(vehicle_chams, 0, 0, 1) \
    X(show_containers, 0, 0, 1) \
    X(show_doors, 0, 0, 1) \
    X(show_dropped_loot, 0, 0, 1) \
    X(show_vehicle_loot, 0, 0, 1) \
    X(radar_world, 0, 0, 1) \
    X(show_health, 0, 0, 1) \
    X(health_gradient, 1, 0, 1) \
    X(show_vehicle_markers, 0, 0, 1) \
    X(show_vehicle_occupants, 0, 0, 1) \
    X(skeleton_width, 1, 1, 4) \
    X(show_tracers, 0, 0, 1) \
    X(tracer_ms, 900, 100, 4000) \
    X(tracer_width, 15, 5, 40) \
    X(tracer_glow, 10, 0, 20) \
    X(oof_on, 0, 0, 1) \
    X(oof_size, 7, 3, 25) \
    X(oof_radius, 30, 30, 500) \
    X(oof_style, 0, 0, 2) \
    X(world_sun, 0, 0, 1) \
    X(world_sun_pct, 250, 100, 800) \
    X(world_fog_off, 0, 0, 1) \
    X(world_fog_custom, 0, 0, 1) \
    X(world_fog_density, 25, 1, 200) \
    X(world_fog_distance, 30, 0, 1000) \
    X(world_sky_off, 0, 0, 1) \
    X(world_sky_color, 0, 0, 1) \
    X(world_sky_glow, 0, 0, 300) \
    X(world_rain, 0, 0, 100) \
    X(local_box, 0, 0, 1) \
    X(local_skeleton, 0, 0, 1) \
    X(local_name, 0, 0, 1) \
    X(local_weapon, 0, 0, 1) \
    X(local_distance, 0, 0, 1) \
    X(local_armor, 0, 0, 1) \
    X(local_chams, 0, 0, 1) \
    X(local_chams_tint, 1, 0, 1) \
    X(local_chams_style, BR_CHAM_FLAT, 0, BR_CHAM_STYLE_COUNT - 1) \
    X(local_chams_opacity, 60, 0, 100) \
    BR_INTERNAL_SETTINGS(X)

typedef struct BR_Config {
    unsigned int magic, version;
#define BR_DECLARE(field, initial, minimum, maximum) int field;
    BR_SETTINGS(BR_DECLARE)
#undef BR_DECLARE
    struct { unsigned char rgb[3]; } color[BR_COLOR_COUNT];
    unsigned int hud_accent_rgb, hud_cursor_rgb;
    /* Theme colors the Amphetaminehaxx menu publishes for the overlay watermark and
     * keybind chrome; not persisted, refreshed by the menu thread. */
    unsigned int hud_outline_rgb, hud_panel_rgb, hud_inl_rgb, hud_dark_rgb,
                 hud_text_rgb, hud_muted_rgb;

    int menu_open;
    int telem_targets;
    int telem_game_pid;
    int telem_cham_n;
    int telem_native_chams;
    int telem_los_state; /* 0 off, 1 loading/stale, 2 ready */
    /* Last game-side status line, mirrored into the menu so an unresolved
     * world reads as "scanning" instead of a dead cheat. */
    char telem_status[224];

#ifdef RR_PORT_DIAGNOSTICS
    int show_vehicles;
#endif
} BR_Config;

static inline int br_local_esp_enabled(const BR_Config* config) {
    return config->local_box || config->local_skeleton || config->local_name ||
           config->local_weapon || config->local_distance || config->local_armor || config->local_chams;
}

/* Unrecognized values use the same no-rarity bucket as the loot renderer. */
static int br_loot_rarity_visible(const BR_Config* config, int rarity) {
    switch (rarity) {
        case 1: return config->loot_tier_common;
        case 2: return config->loot_tier_uncommon;
        case 3: return config->loot_tier_rare;
        case 4: return config->loot_tier_epic;
        case 5: return config->loot_tier_legendary;
        case 6: return config->loot_tier_exotic;
        default: return config->loot_tier_none;
    }
}

BR_Config* br_get(void);
int br_config_set(BR_Config* config, const char* key, int value);
int br_config_save(void);
int br_config_load(void);

int br_start_menu(void);
void br_stop_menu(void);
void br_set_menu_visible(int visible);
void br_request_toggle(void);
int br_menu_hotkey(void);
void br_set_menu_hotkey(int key);

#ifdef __cplusplus
}
#endif
#endif
