// ============================================================================
//  ETERNAL - Dear ImGui menu template (UI only, nothing is wired)
//
//  Requires Dear ImGui 1.89 or newer (tested on 1.89.9, 1.91.0, 1.93.1 WIP).
//
//  Usage (every frame, between ImGui::NewFrame() and ImGui::Render()):
//      static eternal::Settings settings;
//      eternal::Fonts fonts = { ... };          // your already-loaded fonts
//      if (menu_open) eternal::Render(settings, fonts, &menu_open);
//
//  Render() pushes every style var / color / font it needs and pops all of
//  them before returning: the global ImGui style is never modified.
//  It does not read any key to open or close itself: the host decides.
//
//  Buttons only raise flags (save_requested / load_requested): read them
//  after Render() and reset them to false yourself.
// ============================================================================
#pragma once
#include "imgui.h"

namespace eternal
{
    enum ColorSlot
    {
        C_BOX = 0, C_DOWN = 5, C_NAME, C_WEAPON, C_DISTANCE, C_ARMOR, C_FOV = 14,
        C_SKELETON, C_HP_HIGH = 17, C_HP_MID, C_HP_LOW, C_CRIMINAL, C_POLICE, C_CIVILIAN,
        C_FOV_FILL, C_FOV_GRADIENT, C_CHAM, C_TRACER, C_OOF, C_SKY, C_FOG,
        C_LOCAL_CHAM, C_LOCAL_BOX, C_LOCAL_SKELETON, C_LOCAL_NAME, C_LOCAL_WEAPON,
        C_LOCAL_DISTANCE, C_LOCAL_ARMOR,
        C_OVERLAY_CHAM, C_LOCAL_NATIVE_CHAM, C_VEHICLE_CHAM,
        C_COUNT
    };

    struct Settings
    {
        // --- Aim ------------------------------------------------------------
        bool     aim_enabled = false;
        ImGuiKey aim_key = ImGuiKey_MouseRight;
        int      aim_mode = 0;          // 0 mouse, 1 camlock
        int      aim_bind = 2;          // 0 hold, 1 toggle, 2 always
        int      aim_bone = 0;          // 0-9
        int      aim_dist = 0;          // 0-1000 m
        int      aim_smooth = 5;        // 0-20
        int      aim_cal = 100;         // 10-2000 %
        bool     aim_sticky = false;
        bool     aim_closest = false;
        bool     aim_los = false;
        bool     aim_team = true;
        bool     aim_clan = false;
        bool     aim_police = false;
        bool     aim_fov_circle = true;
        bool     aim_fov_fill = false;
        bool     aim_fov_gradient = false;
        int      aim_fov_opacity = 18;  // 0-100
        bool     aim_fov_spin = false;
        int      aim_fov_speed = 90;    // 15-360
        int      aim_fov = 150;         // 30-600 px
        int      cam_fov = 0;           // 0-160

        // --- ESP ------------------------------------------------------------
        bool     esp_enabled = true;
        bool     esp_box = true;
        bool     esp_skeleton = true;
        bool     esp_name = true;
        bool     esp_team = true;
        bool     esp_armor = true;
        bool     esp_distance = true;
        bool     esp_weapon = true;
        bool     esp_self = false;
        bool     esp_clan = true;
        bool     esp_police = true;
        bool     esp_health = false;
        bool     esp_health_gradient = true;
        int      esp_skeleton_width = 1; // 1-4
        int      esp_thickness = 1;      // 1-4
        int      esp_max_dist = 300;     // 0-5000 m

        // --- Chams ----------------------------------------------------------
        bool     cham_on = false;
        bool     cham_tint = true;
        int      cham_opacity = 60;      // 0-100
        int      cham_style = 0;         // 0-8
        bool     cham_materials = false;
        bool     cham_vis_check = false;
        bool     vehicle_chams = false;
        int      vehicle_chams_style = 0;
        int      vehicle_chams_opacity = 60;
        bool     local_chams = false;
        bool     local_chams_tint = true;
        int      local_chams_style = 0;
        int      local_chams_opacity = 60;
        bool     local_chams_materials = false;
        bool     tung_model = true;

        // --- Loot -----------------------------------------------------------
        bool     loot_on = false;
        bool     loot_nodes = false;
        bool     loot_t_none = true;
        bool     loot_t_common = true;
        bool     loot_t_uncommon = true;
        bool     loot_t_rare = true;
        bool     loot_t_epic = true;
        bool     loot_t_legendary = true;
        bool     loot_t_exotic = true;
        bool     loot_containers = false;
        bool     loot_doors = false;
        bool     loot_dropped = false;
        bool     loot_vehicle = false;

        // --- Radar ----------------------------------------------------------
        bool     radar_on = true;
        int      radar_range = 150;      // 25-500
        bool     radar_world = false;
        bool     vehicle_markers = false;
        bool     vehicle_occupants = false;

        // --- Tracers / offscreen --------------------------------------------
        bool     tracers = false;
        int      tracer_ms = 900;        // 100-4000
        int      tracer_width = 15;      // 5-40
        int      tracer_glow = 10;       // 0-20
        bool     oof_on = false;
        int      oof_size = 7;           // 3-25
        int      oof_radius = 30;        // 30-500
        int      oof_style = 0;          // 0-2

        // --- World ----------------------------------------------------------
        bool     world_sun = false;
        int      world_sun_pct = 250;    // 100-800
        bool     world_fog_off = false;
        bool     world_fog_custom = false;
        int      world_fog_density = 25; // 1-200
        int      world_fog_distance = 30;// 0-1000
        bool     world_sky_off = false;
        bool     world_sky_color = false;
        int      world_sky_glow = 0;     // 0-300
        int      world_rain = 0;         // 0-100

        // --- Local player ---------------------------------------------------
        bool     local_box = false;
        bool     local_skeleton = false;
        bool     local_name = false;
        bool     local_weapon = false;
        bool     local_distance = false;
        bool     local_armor = false;

        // --- HUD / menu -----------------------------------------------------
        bool     misc_crosshair = false;
        bool     misc_watermark = true;
        ImGuiKey menu_key = ImGuiKey_Insert;
        int      opacity = 100;          // 40-100

        ImVec4   color[C_COUNT];

        bool     save_requested = false;
        bool     load_requested = false;
    };
    // Fonts are owned and loaded by the host. Any pointer may be null:
    // the current font is then used at the given size.
    struct Fonts
    {
        ImFont* body = nullptr;  float body_px = 14.0f;   // labels, values
        ImFont* semi = nullptr;  float semi_px = 14.0f;   // buttons, profile name
        ImFont* logo = nullptr;  float logo_px = 18.0f;   // "ETERNAL" wordmark
        ImFont* h1   = nullptr;  float h1_px   = 22.0f;   // tab title
        ImFont* mono = nullptr;  float mono_px = 11.0f;   // badges, keycaps, footer
    };

    // Draws the menu. `open` is set to false when the close/minimize button is clicked.
    void Render(Settings& s, const Fonts& fonts, bool* open);

    // Reusable widgets. Only valid inside Render(), between BeginCard / EndCard.
    namespace ui
    {
        void BeginCard(const char* title, float width);
        void EndCard();

        bool Toggle    (const char* label, bool* v);
        bool Slider    (const char* label, int* v, int v_min, int v_max, const char* format = "%d");
        bool Dropdown  (const char* label, int* current, const char* const items[], int count);
        bool Keybind   (const char* label, ImGuiKey* key);
        void KeyDisplay(const char* label, ImGuiKey key);      // read-only keycap
        bool Color     (const char* label, ImVec4* color);
        bool Button    (const char* label, bool primary = false);
    }
}


