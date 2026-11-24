// Two-way bridge between the Eternal menu template and BR_Config.
// The template owns the widgets; this file owns the mapping. BR_Config stores
// raw virtual-key codes and centimetres, so both need translating.
#include "ui/eternal/eternal_menu.h"

#include "ui/menu_bridge.h"
#include "menu/settings/font/font.hpp"
#include "menu/settings/window/window.hpp"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace eternal_ui
{
bool g_active = true;

namespace
{
eternal::Settings s;
int s_opacity = 100;
}

eternal::Settings& settings() { return s; }

namespace
{

int vk_of(ImGuiKey k)
{
    if (k >= ImGuiKey_A && k <= ImGuiKey_Z) return 'A' + int(k - ImGuiKey_A);
    if (k >= ImGuiKey_0 && k <= ImGuiKey_9) return '0' + int(k - ImGuiKey_0);
    if (k >= ImGuiKey_F1 && k <= ImGuiKey_F12) return VK_F1 + int(k - ImGuiKey_F1);
    if (k >= ImGuiKey_Keypad0 && k <= ImGuiKey_Keypad9) return VK_NUMPAD0 + int(k - ImGuiKey_Keypad0);
    switch (k)
    {
    case ImGuiKey_MouseLeft:    return VK_LBUTTON;
    case ImGuiKey_MouseRight:   return VK_RBUTTON;
    case ImGuiKey_MouseMiddle:  return VK_MBUTTON;
    case ImGuiKey_MouseX1:      return VK_XBUTTON1;
    case ImGuiKey_MouseX2:      return VK_XBUTTON2;
    case ImGuiKey_Space:        return VK_SPACE;
    case ImGuiKey_Tab:          return VK_TAB;
    case ImGuiKey_Enter:        return VK_RETURN;
    case ImGuiKey_Backspace:    return VK_BACK;
    case ImGuiKey_Insert:       return VK_INSERT;
    case ImGuiKey_Delete:       return VK_DELETE;
    case ImGuiKey_Home:         return VK_HOME;
    case ImGuiKey_End:          return VK_END;
    case ImGuiKey_PageUp:       return VK_PRIOR;
    case ImGuiKey_PageDown:     return VK_NEXT;
    case ImGuiKey_LeftArrow:    return VK_LEFT;
    case ImGuiKey_RightArrow:   return VK_RIGHT;
    case ImGuiKey_UpArrow:      return VK_UP;
    case ImGuiKey_DownArrow:    return VK_DOWN;
    case ImGuiKey_LeftShift:    return VK_LSHIFT;
    case ImGuiKey_RightShift:   return VK_RSHIFT;
    case ImGuiKey_LeftCtrl:     return VK_LCONTROL;
    case ImGuiKey_RightCtrl:    return VK_RCONTROL;
    case ImGuiKey_LeftAlt:      return VK_LMENU;
    case ImGuiKey_RightAlt:     return VK_RMENU;
    case ImGuiKey_CapsLock:     return VK_CAPITAL;
    case ImGuiKey_NumLock:      return VK_NUMLOCK;
    case ImGuiKey_ScrollLock:   return VK_SCROLL;
    case ImGuiKey_Pause:        return VK_PAUSE;
    case ImGuiKey_Minus:        return VK_OEM_MINUS;
    case ImGuiKey_Equal:        return VK_OEM_PLUS;
    case ImGuiKey_LeftBracket:  return VK_OEM_4;
    case ImGuiKey_RightBracket: return VK_OEM_6;
    case ImGuiKey_Backslash:    return VK_OEM_5;
    case ImGuiKey_Semicolon:    return VK_OEM_1;
    case ImGuiKey_Apostrophe:   return VK_OEM_7;
    case ImGuiKey_Comma:        return VK_OEM_COMMA;
    case ImGuiKey_Period:       return VK_OEM_PERIOD;
    case ImGuiKey_Slash:        return VK_OEM_2;
    case ImGuiKey_GraveAccent:  return VK_OEM_3;
    default: break;
    }
    return 0;
}

ImGuiKey key_of(int vk)
{
    if (vk >= 'A' && vk <= 'Z') return ImGuiKey(ImGuiKey_A + (vk - 'A'));
    if (vk >= '0' && vk <= '9') return ImGuiKey(ImGuiKey_0 + (vk - '0'));
    if (vk >= VK_F1 && vk <= VK_F12) return ImGuiKey(ImGuiKey_F1 + (vk - VK_F1));
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return ImGuiKey(ImGuiKey_Keypad0 + (vk - VK_NUMPAD0));
    switch (vk)
    {
    case VK_LBUTTON:  return ImGuiKey_MouseLeft;
    case VK_RBUTTON:  return ImGuiKey_MouseRight;
    case VK_MBUTTON:  return ImGuiKey_MouseMiddle;
    case VK_XBUTTON1: return ImGuiKey_MouseX1;
    case VK_XBUTTON2: return ImGuiKey_MouseX2;
    case VK_SPACE:    return ImGuiKey_Space;
    case VK_TAB:      return ImGuiKey_Tab;
    case VK_RETURN:   return ImGuiKey_Enter;
    case VK_BACK:     return ImGuiKey_Backspace;
    case VK_INSERT:   return ImGuiKey_Insert;
    case VK_DELETE:   return ImGuiKey_Delete;
    case VK_HOME:     return ImGuiKey_Home;
    case VK_END:      return ImGuiKey_End;
    case VK_PRIOR:    return ImGuiKey_PageUp;
    case VK_NEXT:     return ImGuiKey_PageDown;
    case VK_LEFT:     return ImGuiKey_LeftArrow;
    case VK_RIGHT:    return ImGuiKey_RightArrow;
    case VK_UP:       return ImGuiKey_UpArrow;
    case VK_DOWN:     return ImGuiKey_DownArrow;
    case VK_LSHIFT:   return ImGuiKey_LeftShift;
    case VK_RSHIFT:   return ImGuiKey_RightShift;
    case VK_LCONTROL: return ImGuiKey_LeftCtrl;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_LMENU:    return ImGuiKey_LeftAlt;
    case VK_RMENU:    return ImGuiKey_RightAlt;
    case VK_CAPITAL:  return ImGuiKey_CapsLock;
    case VK_NUMLOCK:  return ImGuiKey_NumLock;
    case VK_SCROLL:   return ImGuiKey_ScrollLock;
    case VK_PAUSE:    return ImGuiKey_Pause;
    case VK_OEM_MINUS:  return ImGuiKey_Minus;
    case VK_OEM_PLUS:   return ImGuiKey_Equal;
    case VK_OEM_4:      return ImGuiKey_LeftBracket;
    case VK_OEM_6:      return ImGuiKey_RightBracket;
    case VK_OEM_5:      return ImGuiKey_Backslash;
    case VK_OEM_1:      return ImGuiKey_Semicolon;
    case VK_OEM_7:      return ImGuiKey_Apostrophe;
    case VK_OEM_COMMA:  return ImGuiKey_Comma;
    case VK_OEM_PERIOD: return ImGuiKey_Period;
    case VK_OEM_2:      return ImGuiKey_Slash;
    case VK_OEM_3:      return ImGuiKey_GraveAccent;
    default: break;
    }
    return ImGuiKey_None;
}

unsigned char to_byte(float v)
{
    return static_cast<unsigned char>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

ImVec4 color_of(int slot)
{
    const auto& rgb = br_get()->color[slot].rgb;
    return ImVec4(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f);
}

bool same_color(int slot, const ImVec4& c)
{
    const auto& rgb = br_get()->color[slot].rgb;
    return rgb[0] == to_byte(c.x) && rgb[1] == to_byte(c.y) && rgb[2] == to_byte(c.z);
}

void set_color(int slot, const ImVec4& c)
{
    auto& rgb = br_get()->color[slot].rgb;
    rgb[0] = to_byte(c.x);
    rgb[1] = to_byte(c.y);
    rgb[2] = to_byte(c.z);
}

void pull()
{
    BR_Config* b = br_get();

    const ImGuiKey k = key_of(b->aa_key);
    s.aim_key = (k == ImGuiKey_None) ? ImGuiKey_MouseRight : k;
    s.aim_enabled    = b->aa_on != 0;
    s.aim_mode       = b->aa_mode == 2 ? 1 : 0;
    s.aim_bind       = std::clamp(b->aa_bind_mode, 0, 2);
    s.aim_bone       = std::clamp(b->aa_bone, 0, 9);
    s.aim_dist       = std::clamp(b->aa_dist, 0, 1000);
    s.aim_smooth     = std::clamp(b->aa_smooth, 0, 20);
    s.aim_cal        = std::clamp(b->aa_aim_cal, 10, 2000);
    s.aim_sticky     = b->aa_sticky != 0;
    s.aim_closest    = b->aa_closest_first != 0;
    s.aim_los        = b->aa_los_on != 0;
    s.aim_team       = b->aa_team_check != 0;
    s.aim_clan       = b->aa_target_clan != 0;
    s.aim_police     = b->aa_target_police != 0;
    s.aim_fov_circle = b->aa_show_fov != 0;
    s.aim_fov_fill   = b->aa_fov_fill != 0;
    s.aim_fov_gradient = b->aa_fov_gradient != 0;
    s.aim_fov_opacity = std::clamp(b->aa_fov_opacity, 0, 100);
    s.aim_fov_spin   = b->aa_fov_spin != 0;
    s.aim_fov_speed  = std::clamp(b->aa_fov_spin_speed, 15, 360);
    s.aim_fov        = std::clamp(b->aa_fov_px, 30, 600);
    s.cam_fov        = std::clamp(b->cam_fov, 0, 160);

    s.esp_enabled    = b->enabled != 0;
    s.esp_box        = b->show_box != 0;
    s.esp_skeleton   = b->show_skeleton != 0;
    s.esp_name       = b->show_name != 0;
    s.esp_team       = b->show_team != 0;
    s.esp_armor      = b->show_armor != 0;
    s.esp_distance   = b->show_distance != 0;
    s.esp_weapon     = b->show_weapon != 0;
    s.esp_self       = b->show_self != 0;
    s.esp_clan       = b->show_clan != 0;
    s.esp_police     = b->show_police != 0;
    s.esp_health     = b->show_health != 0;
    s.esp_health_gradient = b->health_gradient != 0;
    s.esp_skeleton_width = std::clamp(b->skeleton_width, 1, 4);
    s.esp_thickness  = std::clamp(b->line_w, 1, 4);
    s.esp_max_dist   = std::clamp((b->max_distance + 50) / 100, 0, 5000);

    s.cham_on        = b->cham_on != 0;
    s.cham_tint      = b->cham_tint != 0;
    s.cham_opacity   = std::clamp(b->cham_opacity, 0, 100);
    s.cham_style     = std::clamp(b->cham_style, 0, 8);
    s.cham_materials = b->mat_chams != 0;
    s.cham_vis_check = b->cham_vis_check != 0;
    s.vehicle_chams  = b->vehicle_chams != 0;
    s.vehicle_chams_style = std::clamp(b->vehicle_chams_style, 0, 8);
    s.vehicle_chams_opacity = std::clamp(b->vehicle_chams_opacity, 0, 100);
    s.local_chams    = b->local_chams != 0;
    s.local_chams_tint = b->local_chams_tint != 0;
    s.local_chams_style = std::clamp(b->local_chams_style, 0, 8);
    s.local_chams_opacity = std::clamp(b->local_chams_opacity, 0, 100);
    s.local_chams_materials = b->local_mat_chams != 0;
    s.tung_model     = b->tung_tung_model != 0;

    s.loot_on        = b->show_loot != 0;
    s.loot_nodes     = b->show_loot_nodes != 0;
    s.loot_t_none    = b->loot_tier_none != 0;
    s.loot_t_common  = b->loot_tier_common != 0;
    s.loot_t_uncommon = b->loot_tier_uncommon != 0;
    s.loot_t_rare    = b->loot_tier_rare != 0;
    s.loot_t_epic    = b->loot_tier_epic != 0;
    s.loot_t_legendary = b->loot_tier_legendary != 0;
    s.loot_t_exotic  = b->loot_tier_exotic != 0;
    s.loot_containers = b->show_containers != 0;
    s.loot_doors     = b->show_doors != 0;
    s.loot_dropped   = b->show_dropped_loot != 0;
    s.loot_vehicle   = b->show_vehicle_loot != 0;

    s.radar_on       = b->radar_on != 0;
    s.radar_range    = std::clamp(b->radar_range, 25, 500);
    s.radar_world    = b->radar_world != 0;
    s.vehicle_markers = b->show_vehicle_markers != 0;
    s.vehicle_occupants = b->show_vehicle_occupants != 0;

    s.tracers        = b->show_tracers != 0;
    s.tracer_ms      = std::clamp(b->tracer_ms, 100, 4000);
    s.tracer_width   = std::clamp(b->tracer_width, 5, 40);
    s.tracer_glow    = std::clamp(b->tracer_glow, 0, 20);
    s.oof_on         = b->oof_on != 0;
    s.oof_size       = std::clamp(b->oof_size, 3, 25);
    s.oof_radius     = std::clamp(b->oof_radius, 30, 500);
    s.oof_style      = std::clamp(b->oof_style, 0, 2);

    s.world_sun      = b->world_sun != 0;
    s.world_sun_pct  = std::clamp(b->world_sun_pct, 100, 800);
    s.world_fog_off  = b->world_fog_off != 0;
    s.world_fog_custom = b->world_fog_custom != 0;
    s.world_fog_density = std::clamp(b->world_fog_density, 1, 200);
    s.world_fog_distance = std::clamp(b->world_fog_distance, 0, 1000);
    s.world_sky_off  = b->world_sky_off != 0;
    s.world_sky_color = b->world_sky_color != 0;
    s.world_sky_glow = std::clamp(b->world_sky_glow, 0, 300);
    s.world_rain     = std::clamp(b->world_rain, 0, 100);

    s.local_box      = b->local_box != 0;
    s.local_skeleton = b->local_skeleton != 0;
    s.local_name     = b->local_name != 0;
    s.local_weapon   = b->local_weapon != 0;
    s.local_distance = b->local_distance != 0;
    s.local_armor    = b->local_armor != 0;

    s.misc_crosshair = b->hud_cursor != 0;
    s.misc_watermark = b->hud_watermark != 0;
    s.menu_key       = key_of(br_menu_hotkey());
    s.opacity        = s_opacity;

    for (int i = 0; i < BR_COLOR_COUNT && i < eternal::C_COUNT; ++i)
        s.color[i] = color_of(i);
}

#define SYNC(field, cfg, low, high) if ((cfg) != s.field) cfg = std::clamp(s.field, low, high);

void push()
{
    BR_Config* b = br_get();

    const int k = vk_of(s.aim_key);
    if (k && k != b->aa_key) b->aa_key = k;
    if ((b->aa_on != 0) != s.aim_enabled) b->aa_on = s.aim_enabled ? 1 : 0;
    { const int v = s.aim_mode == 1 ? 2 : 0; if (b->aa_mode != v) b->aa_mode = v; }
    SYNC(aim_bind, b->aa_bind_mode, 0, 2)
    SYNC(aim_bone, b->aa_bone, 0, 9)
    SYNC(aim_dist, b->aa_dist, 0, 1000)
    SYNC(aim_smooth, b->aa_smooth, 0, 20)
    SYNC(aim_cal, b->aa_aim_cal, 10, 2000)
    if ((b->aa_sticky != 0) != s.aim_sticky) b->aa_sticky = s.aim_sticky ? 1 : 0;
    if ((b->aa_closest_first != 0) != s.aim_closest) b->aa_closest_first = s.aim_closest ? 1 : 0;
    if ((b->aa_los_on != 0) != s.aim_los) b->aa_los_on = s.aim_los ? 1 : 0;
    if ((b->aa_team_check != 0) != s.aim_team) b->aa_team_check = s.aim_team ? 1 : 0;
    if ((b->aa_target_clan != 0) != s.aim_clan) b->aa_target_clan = s.aim_clan ? 1 : 0;
    if ((b->aa_target_police != 0) != s.aim_police) b->aa_target_police = s.aim_police ? 1 : 0;
    if ((b->aa_show_fov != 0) != s.aim_fov_circle) b->aa_show_fov = s.aim_fov_circle ? 1 : 0;
    if ((b->aa_fov_fill != 0) != s.aim_fov_fill) b->aa_fov_fill = s.aim_fov_fill ? 1 : 0;
    if ((b->aa_fov_gradient != 0) != s.aim_fov_gradient) b->aa_fov_gradient = s.aim_fov_gradient ? 1 : 0;
    SYNC(aim_fov_opacity, b->aa_fov_opacity, 0, 100)
    if ((b->aa_fov_spin != 0) != s.aim_fov_spin) b->aa_fov_spin = s.aim_fov_spin ? 1 : 0;
    SYNC(aim_fov_speed, b->aa_fov_spin_speed, 15, 360)
    SYNC(aim_fov, b->aa_fov_px, 30, 600)
    SYNC(cam_fov, b->cam_fov, 0, 160)

    if ((b->enabled != 0) != s.esp_enabled) b->enabled = s.esp_enabled ? 1 : 0;
    if ((b->show_box != 0) != s.esp_box) b->show_box = s.esp_box ? 1 : 0;
    if ((b->show_skeleton != 0) != s.esp_skeleton) b->show_skeleton = s.esp_skeleton ? 1 : 0;
    if ((b->show_name != 0) != s.esp_name) b->show_name = s.esp_name ? 1 : 0;
    if ((b->show_team != 0) != s.esp_team) b->show_team = s.esp_team ? 1 : 0;
    if ((b->show_armor != 0) != s.esp_armor) b->show_armor = s.esp_armor ? 1 : 0;
    if ((b->show_distance != 0) != s.esp_distance) b->show_distance = s.esp_distance ? 1 : 0;
    if ((b->show_weapon != 0) != s.esp_weapon) b->show_weapon = s.esp_weapon ? 1 : 0;
    if ((b->show_self != 0) != s.esp_self) b->show_self = s.esp_self ? 1 : 0;
    if ((b->show_clan != 0) != s.esp_clan) b->show_clan = s.esp_clan ? 1 : 0;
    if ((b->show_police != 0) != s.esp_police) b->show_police = s.esp_police ? 1 : 0;
    if ((b->show_health != 0) != s.esp_health) b->show_health = s.esp_health ? 1 : 0;
    if ((b->health_gradient != 0) != s.esp_health_gradient) b->health_gradient = s.esp_health_gradient ? 1 : 0;
    SYNC(esp_skeleton_width, b->skeleton_width, 1, 4)
    SYNC(esp_thickness, b->line_w, 1, 4)
    { const int cm = std::clamp(s.esp_max_dist, 0, 5000) * 100; if (b->max_distance != cm) b->max_distance = cm; }

    if ((b->cham_on != 0) != s.cham_on) b->cham_on = s.cham_on ? 1 : 0;
    if ((b->cham_tint != 0) != s.cham_tint) b->cham_tint = s.cham_tint ? 1 : 0;
    SYNC(cham_opacity, b->cham_opacity, 0, 100)
    SYNC(cham_style, b->cham_style, 0, 8)
    if ((b->mat_chams != 0) != s.cham_materials) b->mat_chams = s.cham_materials ? 1 : 0;
    if ((b->cham_vis_check != 0) != s.cham_vis_check) b->cham_vis_check = s.cham_vis_check ? 1 : 0;
    if ((b->vehicle_chams != 0) != s.vehicle_chams) b->vehicle_chams = s.vehicle_chams ? 1 : 0;
    SYNC(vehicle_chams_style, b->vehicle_chams_style, 0, 8)
    SYNC(vehicle_chams_opacity, b->vehicle_chams_opacity, 0, 100)
    if ((b->local_chams != 0) != s.local_chams) b->local_chams = s.local_chams ? 1 : 0;
    if ((b->local_chams_tint != 0) != s.local_chams_tint) b->local_chams_tint = s.local_chams_tint ? 1 : 0;
    SYNC(local_chams_style, b->local_chams_style, 0, 8)
    SYNC(local_chams_opacity, b->local_chams_opacity, 0, 100)
    if ((b->local_mat_chams != 0) != s.local_chams_materials) b->local_mat_chams = s.local_chams_materials ? 1 : 0;
    if ((b->tung_tung_model != 0) != s.tung_model) b->tung_tung_model = s.tung_model ? 1 : 0;

    if ((b->show_loot != 0) != s.loot_on) b->show_loot = s.loot_on ? 1 : 0;
    if ((b->show_loot_nodes != 0) != s.loot_nodes) b->show_loot_nodes = s.loot_nodes ? 1 : 0;
    if ((b->loot_tier_none != 0) != s.loot_t_none) b->loot_tier_none = s.loot_t_none ? 1 : 0;
    if ((b->loot_tier_common != 0) != s.loot_t_common) b->loot_tier_common = s.loot_t_common ? 1 : 0;
    if ((b->loot_tier_uncommon != 0) != s.loot_t_uncommon) b->loot_tier_uncommon = s.loot_t_uncommon ? 1 : 0;
    if ((b->loot_tier_rare != 0) != s.loot_t_rare) b->loot_tier_rare = s.loot_t_rare ? 1 : 0;
    if ((b->loot_tier_epic != 0) != s.loot_t_epic) b->loot_tier_epic = s.loot_t_epic ? 1 : 0;
    if ((b->loot_tier_legendary != 0) != s.loot_t_legendary) b->loot_tier_legendary = s.loot_t_legendary ? 1 : 0;
    if ((b->loot_tier_exotic != 0) != s.loot_t_exotic) b->loot_tier_exotic = s.loot_t_exotic ? 1 : 0;
    if ((b->show_containers != 0) != s.loot_containers) b->show_containers = s.loot_containers ? 1 : 0;
    if ((b->show_doors != 0) != s.loot_doors) b->show_doors = s.loot_doors ? 1 : 0;
    if ((b->show_dropped_loot != 0) != s.loot_dropped) b->show_dropped_loot = s.loot_dropped ? 1 : 0;
    if ((b->show_vehicle_loot != 0) != s.loot_vehicle) b->show_vehicle_loot = s.loot_vehicle ? 1 : 0;

    if ((b->radar_on != 0) != s.radar_on) b->radar_on = s.radar_on ? 1 : 0;
    SYNC(radar_range, b->radar_range, 25, 500)
    if ((b->radar_world != 0) != s.radar_world) b->radar_world = s.radar_world ? 1 : 0;
    if ((b->show_vehicle_markers != 0) != s.vehicle_markers) b->show_vehicle_markers = s.vehicle_markers ? 1 : 0;
    if ((b->show_vehicle_occupants != 0) != s.vehicle_occupants) b->show_vehicle_occupants = s.vehicle_occupants ? 1 : 0;

    if ((b->show_tracers != 0) != s.tracers) b->show_tracers = s.tracers ? 1 : 0;
    SYNC(tracer_ms, b->tracer_ms, 100, 4000)
    SYNC(tracer_width, b->tracer_width, 5, 40)
    SYNC(tracer_glow, b->tracer_glow, 0, 20)
    if ((b->oof_on != 0) != s.oof_on) b->oof_on = s.oof_on ? 1 : 0;
    SYNC(oof_size, b->oof_size, 3, 25)
    SYNC(oof_radius, b->oof_radius, 30, 500)
    SYNC(oof_style, b->oof_style, 0, 2)

    if ((b->world_sun != 0) != s.world_sun) b->world_sun = s.world_sun ? 1 : 0;
    SYNC(world_sun_pct, b->world_sun_pct, 100, 800)
    if ((b->world_fog_off != 0) != s.world_fog_off) b->world_fog_off = s.world_fog_off ? 1 : 0;
    if ((b->world_fog_custom != 0) != s.world_fog_custom) b->world_fog_custom = s.world_fog_custom ? 1 : 0;
    SYNC(world_fog_density, b->world_fog_density, 1, 200)
    SYNC(world_fog_distance, b->world_fog_distance, 0, 1000)
    if ((b->world_sky_off != 0) != s.world_sky_off) b->world_sky_off = s.world_sky_off ? 1 : 0;
    if ((b->world_sky_color != 0) != s.world_sky_color) b->world_sky_color = s.world_sky_color ? 1 : 0;
    SYNC(world_sky_glow, b->world_sky_glow, 0, 300)
    SYNC(world_rain, b->world_rain, 0, 100)

    if ((b->local_box != 0) != s.local_box) b->local_box = s.local_box ? 1 : 0;
    if ((b->local_skeleton != 0) != s.local_skeleton) b->local_skeleton = s.local_skeleton ? 1 : 0;
    if ((b->local_name != 0) != s.local_name) b->local_name = s.local_name ? 1 : 0;
    if ((b->local_weapon != 0) != s.local_weapon) b->local_weapon = s.local_weapon ? 1 : 0;
    if ((b->local_distance != 0) != s.local_distance) b->local_distance = s.local_distance ? 1 : 0;
    if ((b->local_armor != 0) != s.local_armor) b->local_armor = s.local_armor ? 1 : 0;

    if ((b->hud_cursor != 0) != s.misc_crosshair) b->hud_cursor = s.misc_crosshair ? 1 : 0;
    if ((b->hud_watermark != 0) != s.misc_watermark) b->hud_watermark = s.misc_watermark ? 1 : 0;

    for (int i = 0; i < BR_COLOR_COUNT && i < eternal::C_COUNT; ++i)
        if (!same_color(i, s.color[i])) set_color(i, s.color[i]);

    s_opacity = s.opacity;
}

#undef SYNC
}

void draw()
{
    if (!menu::open) return;
    pull();

    static bool font_logged = false;
    if (!font_logged)
    {
        font_logged = true;
        std::ofstream pf("C:\\\\rr_fonts.txt");
        pf << "ui=" << (void*)menu::ui_font << " bold=" << (void*)menu::ui_bold << " dist=" << (void*)menu::dist_font << " nick=" << (void*)menu::nick_font;
    }

    eternal::Fonts fonts;
    fonts.body = menu::ui_font;     fonts.body_px = 15.0f;
    fonts.semi = menu::ui_font;     fonts.semi_px = 15.0f;
    fonts.logo = menu::ui_bold;     fonts.logo_px = 20.0f;
    fonts.h1   = menu::ui_bold;     fonts.h1_px   = 20.0f;
    fonts.mono = menu::ui_small;    fonts.mono_px = 13.0f;

    bool open = menu::open;
    eternal::Render(s, fonts, &open);
    if (open != menu::open) br_set_menu_visible(open ? 1 : 0);

    if (s.save_requested) { br_config_save(); s.save_requested = false; }
    if (s.load_requested) { br_config_load(); pull(); s.load_requested = false; }

    push();
}
}





