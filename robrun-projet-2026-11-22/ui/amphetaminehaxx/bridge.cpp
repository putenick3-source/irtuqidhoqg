#include "ui.hpp"
#include "rr_controls.hpp"
#include "menu/draw/draw/draw.hpp"
#include "menu/settings/window/window.hpp"
#include "overlay/font/load-font/load-font.hpp"
#include "ui/state/theme/theme.hpp"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

namespace amphetaminehaxx {
bool vsync = true;
bool drag_moves_window = false;
ImVec2 menu_pos{40,40};
ImVec2 menu_size{522,560};
static bool no_capture;
static std::filesystem::path directory;
static char profile_name[64] = "default";
static const char* profile_status = "";

static bool valid_name(const std::string& name) {
    return !name.empty() && name != "." && name != ".." &&
           name.find_first_of("/\\:*?\"<>|") == std::string::npos;
}
static bool load_profile(const std::string& name) {
    if (!valid_name(name)) return false;
    const auto file = directory/"profiles"/(name+".dream");
    std::error_code error;
    if (std::filesystem::file_size(file,error) > 65536 || error) return false;
    std::ifstream in(file);
    std::string text;
    int version, theme;
    if (!(in >> text >> version >> theme) || text != "DAYDREAM" || version != 2) return false;
    BR_Config config = *br_get();
    int hotkey = br_menu_hotkey();
    bool capture = no_capture, sync = vsync;
    std::getline(in,text);
    while (std::getline(in,text)) {
        std::istringstream line(text);
        char type;
        if (!(line >> type)) continue;
        if (type == 'r') {
            std::string key;
            int value;
            if (!(line >> key >> value)) return false;
#define RR_LOAD(name, initial, low, high) if (key == #name) config.name = std::clamp(value, int(low), int(high));
            BR_SETTINGS(RR_LOAD)
#undef RR_LOAD
        } else if (type == 'c') {
            int slot, r, g, b;
            if (!(line >> slot >> r >> g >> b) || slot < 0 || slot >= BR_COLOR_COUNT ||
                std::min({r,g,b})<0 || std::max({r,g,b})>255) return false;
            config.color[slot] = {static_cast<unsigned char>(r),static_cast<unsigned char>(g),static_cast<unsigned char>(b)};
        } else if (type == 'k') {
            if (!(line >> hotkey) || hotkey < 6 || hotkey > 254 || hotkey == VK_END ||
                hotkey == VK_ESCAPE || hotkey == BR_ESP_TOGGLE_KEY) return false;
        } else if (type == 'b') {
            bool value;
            std::string key;
            if (!(line >> std::quoted(key) >> value)) return false;
            if (key == "No Capture") capture = value;
            if (key == "Vertical Sync") sync = value;
        }
    }
#define RR_COMMIT(name, initial, low, high) br_get()->name = config.name;
    BR_SETTINGS(RR_COMMIT)
#undef RR_COMMIT
    std::copy_n(config.color,BR_COLOR_COUNT,br_get()->color);
    br_set_menu_hotkey(hotkey);
    vsync = sync;
    no_capture = capture;
    return true;
}
static bool save_profile(const std::string& name) {
    if (!valid_name(name)) return false;
    const auto file = directory/"profiles"/(name+".dream");
    auto pending = file;
    pending += ".tmp";
    std::ofstream out(pending);
    out << "DAYDREAM 2\n0\n";
    const auto& config = *br_get();
#define RR_SAVE(name, initial, low, high) out << "r " #name " " << config.name << '\n';
    BR_SETTINGS(RR_SAVE)
#undef RR_SAVE
    for (int i=0; i<BR_COLOR_COUNT; ++i) {
        const auto& rgb = config.color[i].rgb;
        out << "c " << i << ' ' << int(rgb[0]) << ' ' << int(rgb[1]) << ' ' << int(rgb[2]) << '\n';
    }
    out << "k " << br_menu_hotkey() << '\n'
        << "b \"Vertical Sync\" " << vsync << '\n'
        << "b \"No Capture\" " << no_capture << '\n';
    out.close();
    return out && MoveFileExW(pending.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
void initialize(const std::filesystem::path& path) {
    load_font();
    directory = path;
    std::error_code error;
    std::filesystem::create_directories(directory/"profiles",error);
    std::ifstream in(directory/"autoload.txt");
    std::string name;
    if (std::getline(in,name) && load_profile(name)) strcpy_s(profile_name,name.c_str());
    br_get()->hud_accent_rgb=0x00b5c8;
}
void set_visible(bool show) { menu::open=show; }
bool excludes_capture() { return no_capture; }
void render() { menu::draw(); }

void color(ui::Canvas& c, const char* label, int slot) {
    auto& rgb = br_get()->color[slot].rgb;
    float rgba[]{rgb[0]/255.f,rgb[1]/255.f,rgb[2]/255.f,1.f};
    c.ColorPicker(label,rgba);
    for (int i=0; i<3; ++i)
        rgb[i] = static_cast<unsigned char>(std::lround(std::clamp(rgba[i],0.f,1.f)*255.f));
}
void check(ui::Canvas& c, const char* label, int& setting, int slot) {
    bool value = setting != 0;
    c.Check(label,value);
    if (int(value) != setting) setting = value;
    if (slot >= 0) color(c,label,slot);
}

void key(ui::Canvas& c, const char* label, int& setting, bool menu_key) {
    static ImGuiID listening;
    const ImGuiID id = ImGui::GetID(label);
    c.Label(label);
    const bool started = c.Click(c.x+20,c.y,150,19,"bind");
    if (started) { listening = id; for (int vk=1; vk<255; ++vk) (void)GetAsyncKeyState(vk); }
    const auto& color = ui::theme.control_text_color;
    char title[40]{};
    if (listening == id) strcpy_s(title,"press a key");
    else if (setting >= VK_LBUTTON && setting <= VK_XBUTTON2) {
        const char* names[] = {"none","mouse 1","mouse 2","unknown","mouse 3","mouse 4","mouse 5"};
        strcpy_s(title,names[setting]);
    } else if (setting) GetKeyNameTextA(static_cast<LONG>(MapVirtualKeyW(setting,MAPVK_VK_TO_VSC)<<16),title,40);
    if (!*title) strcpy_s(title,setting ? "key" : "none");
    c.Rect(c.x+20,c.y,c.x+170,c.y+19,c.C(16,16,16));
    c.Text(c.x+26,c.y+2,title,c.C(int(color.x*255),int(color.y*255),int(color.z*255)),true);
    c.y += 24;
    if (listening != id || started) return;
    for (int vk=1; vk<255; ++vk) {
        if (!(GetAsyncKeyState(vk)&1)) continue;
        if (vk == VK_ESCAPE) { listening=0; return; }
        if (menu_key && (vk < 7 || vk == VK_END || vk == BR_ESP_TOGGLE_KEY)) continue;
        setting = vk;
        listening=0;
        return;
    }
}
void profiles(ui::Canvas& c) {
    ImGui::SetCursorScreenPos({c.x+20,c.y});
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##profile",profile_name,sizeof profile_name);
    c.y += 24;
    bool pressed=false;
    c.Button("load",pressed);
    if (pressed) profile_status=load_profile(profile_name) ? "loaded" : "load failed";
    pressed=false;
    c.Button("save",pressed);
    if (pressed) profile_status=save_profile(profile_name) ? "saved" : "save failed";
    pressed=false;
    c.Button("autoload",pressed);
    if (pressed && valid_name(profile_name)) {
        std::ofstream file(directory/"autoload.txt");
        file << profile_name << '\n';
        profile_status=file ? "autoload set" : "autoload failed";
    }
    if (*profile_status) c.Label(profile_status);
}
void theme_controls(ui::Canvas& c, ui::UiState& state) {
    c.Check("vertical sync",vsync);
    c.Check("exclude capture",no_capture);
    c.Slider("fade speed",state.fade_speed,1,255);
    int menu_key=br_menu_hotkey();
    key(c,"menu key",menu_key,true);
    if (menu_key != br_menu_hotkey()) br_set_menu_hotkey(menu_key);
    float accent[]{state.accent[0],state.accent[1],state.accent[2],1.f};
    c.Label("accent");
    c.ColorPicker("accent",accent);
    std::copy_n(accent,3,state.accent);
    ui::theme.checkbox_on_color={accent[0],accent[1],accent[2],1.f};
    const auto byte=[](float value){return unsigned(std::lround(std::clamp(value,0.f,1.f)*255.f));};
    br_get()->hud_accent_rgb=(byte(accent[0])<<16)|(byte(accent[1])<<8)|byte(accent[2]);
}
}
