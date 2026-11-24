#include "overlay/font/load-font/load-font.hpp"
#include "overlay/font/font-res.h"
#include "menu/settings/font/font.hpp"
#include "imgui.h"
#include "imgui_freetype.h"
#include <Windows.h>

#include "otc_fonts.inc"
static ImFont* add_mem(ImGuiIO& io, void* data, int size, float px, const ImFontConfig& cfg)
{
    if (!data || size <= 0)
        return nullptr;
    ImFontConfig c = cfg;
    c.FontDataOwnedByAtlas = false;
    c.SizePixels = px;
    return io.Fonts->AddFontFromMemoryTTF(data, size, px, &c);
}

bool load_font()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    unsigned flags = ImGuiFreeTypeLoaderFlags_LightHinting;
    io.Fonts->FontLoaderFlags = flags;
    ImFontConfig cfg{};
    cfg.FontLoaderFlags = flags;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.RasterizerDensity = 1.f;
    cfg.FontDataOwnedByAtlas = false;

    void* px = nullptr;
    void* flag = nullptr;
    void* tahoma = nullptr;
    void* verdana = nullptr;
    void* verdana_r = nullptr;
    int px_n = 0;
    int flag_n = 0;
    int tahoma_n = 0;
    int verdana_n = 0;
    int verdana_r_n = 0;
    px = otc_px; px_n = sizeof otc_px;
    flag = otc_flag; flag_n = sizeof otc_flag;
    tahoma = otc_tahoma; tahoma_n = sizeof otc_tahoma;
    verdana = otc_verdana_bold; verdana_n = sizeof otc_verdana_bold;
    verdana_r = otc_verdana; verdana_r_n = sizeof otc_verdana;

    if (!add_mem(io, px, px_n, 16.f, cfg))
        io.Fonts->AddFontDefault(&cfg);
    ImFontConfig big = cfg;
    big.RasterizerDensity = 3.f;
    menu::dist_font = add_mem(io, px, px_n, 13.f, big);
    menu::brand_font = add_mem(io, px, px_n, 15.f, big);
    menu::flag_font = add_mem(io, flag, flag_n, 10.f, cfg);
    if (!menu::flag_font)
        menu::flag_font = menu::dist_font;

    ImFontConfig nb{};
    nb.FontLoaderFlags = flags;
    nb.PixelSnapH = true;
    nb.OversampleH = 1;
    nb.OversampleV = 1;
    nb.RasterizerDensity = 1.f;
    nb.FontDataOwnedByAtlas = false;
    menu::nick_fonts[0] = add_mem(io, tahoma, tahoma_n, 13.f, nb);
    menu::nick_fonts[1] = add_mem(io, verdana, verdana_n, 13.f, nb);
    if (!menu::nick_fonts[0])
        menu::nick_fonts[0] = menu::nick_fonts[1];
    if (!menu::nick_fonts[1])
        menu::nick_fonts[1] = menu::nick_fonts[0];
    menu::nick_font = menu::nick_fonts[0];
    menu::info_fonts[0] = menu::dist_font;
    menu::info_fonts[1] = add_mem(io, verdana_r, verdana_r_n, 13.f, nb);
    if (!menu::info_fonts[1])
        menu::info_fonts[1] = menu::info_fonts[0];
    if (!menu::info_fonts[0])
        menu::info_fonts[0] = menu::info_fonts[1];
    menu::info_font = menu::info_fonts[0];

    ImFontConfig mf{};
    mf.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
    mf.RasterizerDensity = 3.f;
    mf.OversampleH = 2;
    mf.OversampleV = 2;
    mf.PixelSnapH = false;
    mf.FontDataOwnedByAtlas = false;
    ImFontConfig sf{};
    sf.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
    sf.RasterizerDensity = 1.f;
    sf.OversampleH = 1;
    sf.OversampleV = 1;
    menu::ui_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.f, &sf);
    menu::ui_small = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 13.f, &sf);
    menu::ui_bold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 20.f, &sf);
    return menu::nick_font != nullptr;
}







