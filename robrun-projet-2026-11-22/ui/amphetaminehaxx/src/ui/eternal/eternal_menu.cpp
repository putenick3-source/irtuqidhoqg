// ============================================================================
//  ETERNAL - Dear ImGui menu template (UI only, nothing is wired)
//  See eternal_menu.h for usage. Plain ASCII source (safe with /utf-8).
// ============================================================================
#include "eternal_menu.h"

#include <cfloat>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace eternal
{
// ----------------------------------------------------------------------------
// Palette (same as the design)
// ----------------------------------------------------------------------------
static const ImU32 ACCENT      = IM_COL32(225,  29,  46, 255);   // brand red
static const ImU32 ACCENT_HI   = IM_COL32(240,  60,  75, 255);
static const ImU32 ACCENT_TEXT = IM_COL32(255, 107, 110, 255);
static const ImU32 WIN         = IM_COL32( 18,  14,  15, 255);
static const ImU32 BAR         = IM_COL32( 14,  10,  11, 255);
static const ImU32 CARD        = IM_COL32( 24,  18,  20, 255);
static const ImU32 POPUP       = IM_COL32( 28,  21,  23, 255);
static const ImU32 LINE        = IM_COL32( 36,  26,  29, 255);
static const ImU32 LINE2       = IM_COL32( 42,  31,  34, 255);
static const ImU32 LINE3       = IM_COL32( 58,  42,  46, 255);
static const ImU32 LINE4       = IM_COL32( 90,  66,  72, 255);
static const ImU32 TEXT_MAIN        = IM_COL32(233, 227, 228, 255);
static const ImU32 TEXT_HI     = IM_COL32(245, 240, 241, 255);
static const ImU32 MUTED       = IM_COL32(156, 138, 143, 255);
static const ImU32 LABEL_OFF   = IM_COL32(181, 166, 170, 255);
static const ImU32 HOVER       = IM_COL32( 28,  21,  23, 255);
static const ImU32 HOVER_2     = IM_COL32( 42,  31,  34, 255);
static const ImU32 TRACK_OFF   = IM_COL32( 35,  26,  29, 255);
static const ImU32 KNOB_OFF    = IM_COL32(142, 124, 129, 255);
static const ImU32 SLIDER_BG   = IM_COL32( 46,  34,  38, 255);
static const ImU32 PILL_BG     = IM_COL32( 17,  12,  14, 255);
static const ImU32 PILL_BORDER = IM_COL32( 51,  38,  42, 255);
static const ImU32 KEY_BG      = IM_COL32( 31,  23,  26, 255);
static const ImU32 RING        = IM_COL32( 74,  54,  59, 255);
static const ImU32 PROFILE_BG  = IM_COL32( 20,  15,  17, 255);
static const ImU32 CLOSE_HOVER = IM_COL32( 58,  18,  22, 255);
static const ImU32 WHITE       = IM_COL32(255, 255, 255, 255);
static const ImU32 BLACK       = IM_COL32(  0,   0,   0, 255);

static const ImVec4 COLOR_PRESETS[] = {
    ImVec4(0.882f, 0.114f, 0.180f, 1.0f),   // red
    ImVec4(1.000f, 1.000f, 1.000f, 1.0f),   // white
    ImVec4(0.984f, 0.749f, 0.141f, 1.0f),   // amber
    ImVec4(0.204f, 0.827f, 0.600f, 1.0f),   // green
    ImVec4(0.376f, 0.647f, 0.980f, 1.0f),   // blue
    ImVec4(0.753f, 0.518f, 0.988f, 1.0f),   // purple
};

static const float PI = 3.14159265358979f;

// ----------------------------------------------------------------------------
// Small helpers
// ----------------------------------------------------------------------------
static inline float Min(float a, float b) { return a < b ? a : b; }
static inline float Max(float a, float b) { return a > b ? a : b; }
static inline float Saturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static inline ImVec2 Add(const ImVec2& a, float x, float y) { return ImVec2(a.x + x, a.y + y); }

// Blend two colors.
static ImU32 Mix(ImU32 a, ImU32 b, float t)
{
    t = Saturate(t);
    int r  = (int)(((a >> IM_COL32_R_SHIFT) & 0xFF) * (1 - t) + ((b >> IM_COL32_R_SHIFT) & 0xFF) * t);
    int g  = (int)(((a >> IM_COL32_G_SHIFT) & 0xFF) * (1 - t) + ((b >> IM_COL32_G_SHIFT) & 0xFF) * t);
    int bl = (int)(((a >> IM_COL32_B_SHIFT) & 0xFF) * (1 - t) + ((b >> IM_COL32_B_SHIFT) & 0xFF) * t);
    int al = (int)(((a >> IM_COL32_A_SHIFT) & 0xFF) * (1 - t) + ((b >> IM_COL32_A_SHIFT) & 0xFF) * t);
    return IM_COL32(r, g, bl, al);
}

// Apply an alpha multiplier and the global style alpha (menu opacity).
static ImU32 Px(ImU32 c, float alpha = 1.0f)
{
    float a = ((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f * alpha * ImGui::GetStyle().Alpha;
    return (c & ~IM_COL32_A_MASK) | ((ImU32)(Saturate(a) * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
}

// Smoothly moves a stored value toward `target` (per widget, kept in ImGui storage).
static float Anim(ImGuiID id, float target, float speed = 14.0f)
{
    float* v = ImGui::GetStateStorage()->GetFloatRef(id, target);
    float k = Saturate(ImGui::GetIO().DeltaTime * speed);
    *v += (target - *v) * k;
    if (*v - target < 0.001f && target - *v < 0.001f) *v = target;
    return *v;
}

// ----------------------------------------------------------------------------
// Fonts (provided by the host through Render(); every push is popped right away)
// ----------------------------------------------------------------------------
enum FontKind { F_BODY, F_SEMI, F_LOGO, F_H1, F_MONO };
static const Fonts* g_fonts = nullptr;

static void PushF(FontKind k)
{
    static const Fonts defaults;
    const Fonts& f = g_fonts ? *g_fonts : defaults;
    ImFont* font = nullptr;
    float px = 14.0f;
    switch (k)
    {
    case F_BODY: font = f.body; px = f.body_px; break;
    case F_SEMI: font = f.semi; px = f.semi_px; break;
    case F_LOGO: font = f.logo; px = f.logo_px; break;
    case F_H1:   font = f.h1;   px = f.h1_px;   break;
    case F_MONO: font = f.mono; px = f.mono_px; break;
    }
#if IMGUI_VERSION_NUM >= 19200
    ImGui::PushFont(font, px);                 // null font = keep current font, change size
#else
    (void)px;
    ImGui::PushFont(font ? font : ImGui::GetFont());
#endif
}

static ImVec2 TextSize(FontKind k, const char* text)
{
    PushF(k);
    ImVec2 s = ImGui::CalcTextSize(text);
    ImGui::PopFont();
    return s;
}

static void DrawLabel(ImDrawList* dl, FontKind k, ImVec2 pos, ImU32 col, const char* text)
{
    PushF(k);
    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2((float)(int)pos.x, (float)(int)pos.y), Px(col), text);
    ImGui::PopFont();
}

// Text with extra letter spacing (ASCII). Returns the drawn width.
static float DrawLabelSpaced(ImDrawList* dl, FontKind k, ImVec2 pos, ImU32 col, const char* text, float spacing, bool draw = true)
{
    PushF(k);
    float x = pos.x;
    for (const char* c = text; *c; ++c)
    {
        if (draw) dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2((float)(int)x, (float)(int)pos.y), Px(col), c, c + 1);
        x += ImGui::CalcTextSize(c, c + 1).x + (c[1] ? spacing : 0.0f);
    }
    ImGui::PopFont();
    return x - pos.x;
}

// ----------------------------------------------------------------------------
// Style: pushed at the start of Render(), popped before it returns.
// The global ImGui style is never modified.
// ----------------------------------------------------------------------------
static int g_pushed_vars = 0;
static int g_pushed_cols = 0;

static void Var(ImGuiStyleVar v, float x)            { ImGui::PushStyleVar(v, x); ++g_pushed_vars; }
static void Var(ImGuiStyleVar v, ImVec2 x)           { ImGui::PushStyleVar(v, x); ++g_pushed_vars; }
static void Col(ImGuiCol c, ImU32 x)                 { ImGui::PushStyleColor(c, x); ++g_pushed_cols; }
static void Col(ImGuiCol c, const ImVec4& x)         { ImGui::PushStyleColor(c, x); ++g_pushed_cols; }

static void PushMenuStyle(float alpha)
{
    g_pushed_vars = 0;
    g_pushed_cols = 0;

    Var(ImGuiStyleVar_Alpha,             alpha);
    Var(ImGuiStyleVar_WindowPadding,     ImVec2(0.0f, 0.0f));
    Var(ImGuiStyleVar_ItemSpacing,       ImVec2(0.0f, 0.0f));
    Var(ImGuiStyleVar_WindowBorderSize,  0.0f);
    Var(ImGuiStyleVar_WindowRounding,    14.0f);
    Var(ImGuiStyleVar_ChildRounding,     12.0f);
    Var(ImGuiStyleVar_ChildBorderSize,   0.0f);
    Var(ImGuiStyleVar_PopupRounding,     10.0f);
    Var(ImGuiStyleVar_PopupBorderSize,   1.0f);
    Var(ImGuiStyleVar_FrameRounding,     8.0f);
    Var(ImGuiStyleVar_FrameBorderSize,   0.0f);
    Var(ImGuiStyleVar_GrabRounding,      8.0f);
    Var(ImGuiStyleVar_ScrollbarSize,     8.0f);
    Var(ImGuiStyleVar_ScrollbarRounding, 8.0f);

    Col(ImGuiCol_Text,                 TEXT_MAIN);
    Col(ImGuiCol_TextDisabled,         MUTED);
    Col(ImGuiCol_WindowBg,             WIN);
    Col(ImGuiCol_ChildBg,              IM_COL32(0, 0, 0, 0));
    Col(ImGuiCol_PopupBg,              POPUP);
    Col(ImGuiCol_Border,               LINE3);
    Col(ImGuiCol_FrameBg,              PILL_BG);
    Col(ImGuiCol_FrameBgHovered,       HOVER);
    Col(ImGuiCol_FrameBgActive,        HOVER_2);
    Col(ImGuiCol_ScrollbarBg,          IM_COL32(0, 0, 0, 0));
    Col(ImGuiCol_ScrollbarGrab,        LINE4);
    Col(ImGuiCol_ScrollbarGrabHovered, LINE4);
    Col(ImGuiCol_ScrollbarGrabActive,  ACCENT);
    Col(ImGuiCol_CheckMark,            ACCENT);
    Col(ImGuiCol_SliderGrab,           ACCENT);
    Col(ImGuiCol_SliderGrabActive,     ACCENT_HI);
    Col(ImGuiCol_Button,               HOVER);
    Col(ImGuiCol_ButtonHovered,        HOVER_2);
    Col(ImGuiCol_ButtonActive,         ACCENT);
    Col(ImGuiCol_Header,               ImVec4(0.882f, 0.114f, 0.180f, 0.12f));
    Col(ImGuiCol_HeaderHovered,        HOVER_2);
    Col(ImGuiCol_HeaderActive,         ImVec4(0.882f, 0.114f, 0.180f, 0.25f));
    Col(ImGuiCol_Separator,            LINE);
    Col(ImGuiCol_TextSelectedBg,       ImVec4(0.882f, 0.114f, 0.180f, 0.35f));
}

static void PopMenuStyle()
{
    ImGui::PopStyleColor(g_pushed_cols);
    ImGui::PopStyleVar(g_pushed_vars);
    g_pushed_cols = 0;
    g_pushed_vars = 0;
}

// ----------------------------------------------------------------------------
// Icons (drawn in a 24x24 grid, like the SVGs of the design)
// ----------------------------------------------------------------------------
enum Icon { ICON_CROSSHAIR, ICON_EYE, ICON_SLIDERS, ICON_LAYERS };

static void DrawIcon(ImDrawList* dl, int icon, ImVec2 tl, float size, ImU32 col)
{
    const float s = size / 24.0f;
    const float th = 2.0f * s + 0.1f;
    auto P = [&](float x, float y) { return ImVec2(tl.x + x * s, tl.y + y * s); };
    switch (icon)
    {
    case ICON_CROSSHAIR:
        dl->AddCircle(P(12, 12), 8 * s, col, 32, th);
        dl->AddLine(P(12, 2),  P(12, 7),  col, th);
        dl->AddLine(P(12, 17), P(12, 22), col, th);
        dl->AddLine(P(2, 12),  P(7, 12),  col, th);
        dl->AddLine(P(17, 12), P(22, 12), col, th);
        break;
    case ICON_EYE:
        dl->PathLineTo(P(2, 12));
        dl->PathBezierCubicCurveTo(P(6, 4),  P(18, 4),  P(22, 12));
        dl->PathBezierCubicCurveTo(P(18, 20), P(6, 20), P(2, 12));
        dl->PathStroke(col, ImDrawFlags_Closed, th);
        dl->AddCircle(P(12, 12), 3 * s, col, 20, th);
        break;
    case ICON_SLIDERS:
        dl->AddLine(P(4, 7),  P(14, 7),  col, th);
        dl->AddLine(P(18, 7), P(20, 7),  col, th);
        dl->AddLine(P(4, 17), P(8, 17),  col, th);
        dl->AddLine(P(12, 17), P(20, 17), col, th);
        dl->AddCircle(P(16, 7),  2 * s, col, 16, th);
        dl->AddCircle(P(10, 17), 2 * s, col, 16, th);
        break;
    case ICON_LAYERS:
    {
        ImVec2 top[] = { P(12, 3), P(21, 8), P(12, 13), P(3, 8) };
        dl->AddPolyline(top, 4, col, ImDrawFlags_Closed, th);
        ImVec2 bottom[] = { P(3, 13), P(12, 18), P(21, 13) };
        dl->AddPolyline(bottom, 3, col, 0, th);
        break;
    }
    }
}

// The infinity mark of the logo.
static void DrawInfinity(ImDrawList* dl, ImVec2 tl, float size, ImU32 col)
{
    const float s = size / 24.0f;
    auto P = [&](float x, float y) { return ImVec2(tl.x + x * s, tl.y + y * s); };
    dl->PathLineTo(P(12, 12));
    dl->PathBezierCubicCurveTo(P(10, 9.5f), P(8.5f, 8), P(6, 8));
    dl->PathArcTo(P(6, 12), 4 * s, -PI * 0.5f, -PI * 1.5f, 20);
    dl->PathBezierCubicCurveTo(P(8.5f, 16), P(10, 14.5f), P(12, 12));
    dl->PathBezierCubicCurveTo(P(14, 14.5f), P(15.5f, 16), P(18, 16));
    dl->PathArcTo(P(18, 12), 4 * s, PI * 0.5f, -PI * 0.5f, 20);
    dl->PathBezierCubicCurveTo(P(15.5f, 8), P(14, 9.5f), P(12, 12));
    dl->PathStroke(col, 0, 2.2f * s);
}

static void DrawChevron(ImDrawList* dl, ImVec2 c, ImU32 col, bool up)
{
    const float d = up ? -1.0f : 1.0f;
    ImVec2 pts[] = { ImVec2(c.x - 4, c.y - 2 * d), ImVec2(c.x, c.y + 2 * d), ImVec2(c.x + 4, c.y - 2 * d) };
    dl->AddPolyline(pts, 3, col, 0, 2.0f);
}

// Soft glow: a few expanding, transparent rounded rects.
static void DrawGlow(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, ImU32 col, float strength, float offset_y = 0.0f)
{
    if (strength <= 0.01f) return;
    for (int i = 3; i >= 1; --i)
    {
        float e = i * 2.5f;
        dl->AddRectFilled(ImVec2(a.x - e, a.y - e + offset_y), ImVec2(b.x + e, b.y + e + offset_y), Px(col, 0.05f * strength), rounding + e);
    }
}

// Key names: MOUSE1..5 for mouse buttons, uppercase ImGui names otherwise.
static const char* KeyLabel(ImGuiKey k, char* buf, size_t n)
{
    switch (k)
    {
    case ImGuiKey_None:        return "NONE";
    case ImGuiKey_MouseLeft:   return "MOUSE1";
    case ImGuiKey_MouseRight:  return "MOUSE2";
    case ImGuiKey_MouseMiddle: return "MOUSE3";
    case ImGuiKey_MouseX1:     return "MOUSE4";
    case ImGuiKey_MouseX2:     return "MOUSE5";
    default: break;
    }
    const char* name = ImGui::GetKeyName(k);
    size_t i = 0;
    for (; name[i] && i + 1 < n; ++i) buf[i] = (char)toupper((unsigned char)name[i]);
    buf[i] = 0;
    return buf;
}

// ----------------------------------------------------------------------------
// Widgets
// ----------------------------------------------------------------------------
static float  g_row_w = 300.0f;      // width of a row inside the current card
static ImVec2 g_card_min;
static float  g_card_w = 0.0f;
static ImGuiID g_listening = 0;
static int     g_serial = 0;      // keybind waiting for a key
static int     g_listen_frame = 0;

namespace ui
{
void BeginCard(const char* title, float width)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);

    g_card_min = ImGui::GetCursorScreenPos();
    g_card_w   = width;
    g_row_w    = width - 32.0f;

    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(width, 14.0f));                                  // top padding
    ImGui::SetCursorScreenPos(ImVec2(g_card_min.x + 16.0f, ImGui::GetCursorScreenPos().y));
    ImGui::BeginGroup();

    // Title: small red square + uppercase mono label
    char up[64];
    size_t i = 0;
    for (; title[i] && i + 1 < sizeof(up); ++i) up[i] = (char)toupper((unsigned char)title[i]);
    up[i] = 0;

    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(Add(p, 0, 6), Add(p, 4, 10), Px(ACCENT), 1.0f);
    ImVec2 ts = TextSize(F_MONO, up);
    DrawLabelSpaced(dl, F_MONO, Add(p, 12, (16 - ts.y) * 0.5f), MUTED, up, 1.3f);
    ImGui::Dummy(ImVec2(g_row_w, 22.0f));
}

void EndCard()
{
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(1.0f, 8.0f));                                    // bottom padding
    ImGui::EndGroup();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 max(g_card_min.x + g_card_w, ImGui::GetItemRectMax().y);
    dl->ChannelsSetCurrent(0);
    dl->AddRectFilled(g_card_min, max, Px(CARD), 12.0f);
    dl->AddRect(g_card_min, max, Px(LINE2), 12.0f, 0, 1.0f);
    dl->ChannelsMerge();
}

bool Toggle(const char* label, bool* v)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 44.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##row", ImVec2(g_row_w, h));
    bool hovered = ImGui::IsItemHovered();
    if (pressed) *v = !*v;

    float t  = Anim(ImGui::GetID("##t"), *v ? 1.0f : 0.0f, 16.0f);
    float hv = Anim(ImGui::GetID("##h"), hovered ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 0, (h - ts.y) * 0.5f), Mix(LABEL_OFF, TEXT_HI, Max(t, hv * 0.6f)), label);

    ImVec2 a(p.x + g_row_w - 46.0f, p.y + (h - 26.0f) * 0.5f);
    ImVec2 b(a.x + 46.0f, a.y + 26.0f);
    DrawGlow(dl, a, b, 13.0f, ACCENT, t);
    ImU32 track = Mix(Mix(TRACK_OFF, ACCENT, t), WHITE, 0.06f * hv);
    dl->AddRectFilled(a, b, Px(track), 13.0f);
    dl->AddRect(a, b, Px(Mix(LINE3, ACCENT, t)), 13.0f, 0, 1.0f);

    ImVec2 knob(a.x + 13.0f + t * 20.0f, a.y + 13.0f);
    dl->AddCircleFilled(Add(knob, 0, 1), 9.0f, Px(BLACK, 0.45f), 24);
    dl->AddCircleFilled(knob, 9.0f, Px(Mix(KNOB_OFF, WHITE, t)), 24);

    ImGui::PopID();
    ImGui::PopID();
    return pressed;
}

bool Slider(const char* label, int* v, int v_min, int v_max, const char* format)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 60.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##row", ImVec2(g_row_w, h));
    bool hovered = ImGui::IsItemHovered();
    bool active  = ImGui::IsItemActive();

    const float x0 = p.x + 8.0f, x1 = p.x + g_row_w - 8.0f;
    bool changed = false;
    if (active && v_max > v_min)
    {
        float f = Saturate((ImGui::GetIO().MousePos.x - x0) / (x1 - x0));
        int nv = v_min + (int)(f * (v_max - v_min) + 0.5f);
        if (nv != *v) { *v = nv; changed = true; }
    }

    float frac  = v_max > v_min ? (float)(*v - v_min) / (float)(v_max - v_min) : 0.0f;
    float shown = Anim(ImGui::GetID("##f"), frac, 22.0f);
    float hv    = Anim(ImGui::GetID("##h"), (hovered || active) ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Label + value badge
    const float cy = p.y + 18.0f;
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, ImVec2(p.x, cy - ts.y * 0.5f), TEXT_MAIN, label);

    char buf[32];
    snprintf(buf, sizeof(buf), format, *v);
    ImVec2 vs = TextSize(F_MONO, buf);
    ImVec2 ba(p.x + g_row_w - vs.x - 16.0f, cy - 10.0f), bb(p.x + g_row_w, cy + 10.0f);
    dl->AddRectFilled(ba, bb, Px(ACCENT, 0.12f + 0.06f * hv), 6.0f);
    DrawLabel(dl, F_MONO, ImVec2(ba.x + 8.0f, cy - vs.y * 0.5f), ACCENT_TEXT, buf);

    // Track
    const float ty = p.y + 46.0f;
    const float tx = x0 + shown * (x1 - x0);
    dl->AddRectFilled(ImVec2(p.x, ty - 3.0f), ImVec2(p.x + g_row_w, ty + 3.0f), Px(SLIDER_BG), 3.0f);
    dl->AddRectFilled(ImVec2(p.x, ty - 3.0f), ImVec2(Max(tx, p.x + 6.0f), ty + 3.0f), Px(ACCENT), 3.0f);

    // Thumb
    const float r = 8.0f * (1.0f + 0.15f * hv);
    dl->AddCircleFilled(ImVec2(tx, ty), r + 4.0f + 2.0f * hv, Px(ACCENT, 0.18f + 0.07f * hv), 32);
    dl->AddCircleFilled(ImVec2(tx, ty), r, Px(ACCENT), 32);
    dl->AddCircleFilled(ImVec2(tx, ty), r - 3.0f, Px(WHITE), 32);

    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

bool Dropdown(const char* label, int* current, const char* const items[], int count)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 44.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##row", ImVec2(g_row_w, h));
    bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##popup");
    bool open = ImGui::IsPopupOpen("##popup");
    float hv = Anim(ImGui::GetID("##h"), (hovered || open) ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 0, (h - ts.y) * 0.5f), TEXT_MAIN, label);

    // Pill with the current value
    const char* cur = items[*current];
    ImVec2 cs = TextSize(F_BODY, cur);
    float pw = Max(96.0f, cs.x + 46.0f);
    ImVec2 a(p.x + g_row_w - pw, p.y + 6.0f), b(p.x + g_row_w, p.y + 38.0f);
    dl->AddRectFilled(a, b, Px(Mix(PILL_BG, HOVER, hv)), 16.0f);
    dl->AddRect(a, b, Px(open ? ACCENT : Mix(PILL_BORDER, LINE4, hv)), 16.0f, 0, 1.0f);
    DrawLabel(dl, F_BODY, ImVec2(a.x + 12.0f, p.y + (h - cs.y) * 0.5f), TEXT_MAIN, cur);
    DrawChevron(dl, ImVec2(b.x - 16.0f, p.y + h * 0.5f), Px(ACCENT_TEXT), open);

    // List
    bool changed = false;
    float item_w = pw;
    for (int i = 0; i < count; ++i) item_w = Max(item_w, TextSize(F_BODY, items[i]).x + 44.0f);

    ImGui::SetNextWindowPos(ImVec2(b.x, b.y + 6.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
    if (ImGui::BeginPopup("##popup"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        for (int i = 0; i < count; ++i)
        {
            ImGui::PushID(i);
            ImVec2 ip = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##item", ImVec2(item_w, 30.0f)))
            {
                *current = i;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            bool ih = ImGui::IsItemHovered();
            bool sel = (i == *current);
            if (sel)     pdl->AddRectFilled(ip, Add(ip, item_w, 30.0f), Px(ACCENT, 0.14f), 7.0f);
            else if (ih) pdl->AddRectFilled(ip, Add(ip, item_w, 30.0f), Px(HOVER_2), 7.0f);
            ImVec2 is = TextSize(F_BODY, items[i]);
            DrawLabel(pdl, F_BODY, Add(ip, 10.0f, (30.0f - is.y) * 0.5f), sel ? WHITE : TEXT_MAIN, items[i]);
            if (sel)
            {
                ImVec2 c = Add(ip, item_w - 16.0f, 15.0f);
                ImVec2 chk[] = { Add(c, -4, 0), Add(c, -1, 3), Add(c, 4, -3) };
                pdl->AddPolyline(chk, 3, Px(ACCENT_TEXT), 0, 2.0f);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);

    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

bool Keybind(const char* label, ImGuiKey* key)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 44.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGuiID id = ImGui::GetID("##row");
    bool pressed = ImGui::InvisibleButton("##row", ImVec2(g_row_w, h));
    bool hovered = ImGui::IsItemHovered();
    bool changed = false;

    if (pressed) { g_listening = id; g_listen_frame = ImGui::GetFrameCount(); }
    bool listening = (g_listening == id);
    if (listening && ImGui::GetFrameCount() > g_listen_frame)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            g_listening = 0;                                             // cancel
        else
            for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
            {
                ImGuiKey kk = (ImGuiKey)k;
                if (kk == ImGuiKey_Escape || kk == ImGuiKey_MouseWheelX || kk == ImGuiKey_MouseWheelY) continue;
                if (ImGui::IsKeyPressed(kk, false)) { *key = kk; g_listening = 0; changed = true; break; }
            }
        listening = (g_listening == id);
    }
    float hv = Anim(ImGui::GetID("##h"), hovered ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 0, (h - ts.y) * 0.5f), TEXT_MAIN, label);

    // Keycap
    char buf[64];
    const char* name = listening ? "..." : KeyLabel(*key, buf, sizeof(buf));
    ImVec2 ks = TextSize(F_MONO, name);
    float kw = Max(52.0f, ks.x + 20.0f);
    ImVec2 a(p.x + g_row_w - kw, p.y + 7.0f), b(p.x + g_row_w, p.y + 37.0f);
    if (listening) DrawGlow(dl, a, b, 7.0f, ACCENT, 1.0f);
    dl->AddRectFilled(a, b, Px(listening ? ACCENT : Mix(LINE3, LINE4, hv)), 7.0f);   // border + bottom edge
    dl->AddRectFilled(Add(a, 1, 1), Add(b, -1, -3), Px(Mix(KEY_BG, HOVER_2, hv)), 6.0f);
    DrawLabel(dl, F_MONO, ImVec2(a.x + (kw - ks.x) * 0.5f, a.y + (27.0f - ks.y) * 0.5f), listening ? ACCENT_TEXT : TEXT_HI, name);

    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

void KeyDisplay(const char* label, ImGuiKey key)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 44.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(g_row_w, h));                                   // not clickable

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 0, (h - ts.y) * 0.5f), TEXT_MAIN, label);

    char buf[64];
    const char* name = KeyLabel(key, buf, sizeof(buf));
    ImVec2 ks = TextSize(F_MONO, name);
    float kw = Max(52.0f, ks.x + 20.0f);
    ImVec2 a(p.x + g_row_w - kw, p.y + 7.0f), b(p.x + g_row_w, p.y + 37.0f);
    dl->AddRectFilled(a, b, Px(LINE3), 7.0f);
    dl->AddRectFilled(Add(a, 1, 1), Add(b, -1, -3), Px(KEY_BG), 6.0f);
    DrawLabel(dl, F_MONO, ImVec2(a.x + (kw - ks.x) * 0.5f, a.y + (27.0f - ks.y) * 0.5f), MUTED, name);

    ImGui::PopID();
    ImGui::PopID();
}

bool Color(const char* label, ImVec4* color)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const float h = 44.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##row", ImVec2(g_row_w, h));
    bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##popup");
    float hv = Anim(ImGui::GetID("##h"), hovered ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 0, (h - ts.y) * 0.5f), TEXT_MAIN, label);

    ImVec2 c(p.x + g_row_w - 15.0f, p.y + h * 0.5f);
    dl->AddCircleFilled(c, 15.5f, Px(Mix(RING, LINE4, hv)), 32);
    dl->AddCircleFilled(c, 14.5f, Px(CARD), 32);
    dl->AddCircleFilled(c, 11.5f, Px(ImGui::ColorConvertFloat4ToU32(*color)), 32);

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(c.x + 15.0f, c.y + 22.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    if (ImGui::BeginPopup("##popup"))
    {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        const int n = (int)(sizeof(COLOR_PRESETS) / sizeof(COLOR_PRESETS[0]));
        for (int i = 0; i < n; ++i)
        {
            if (i > 0) ImGui::SameLine();
            ImGui::PushID(i);
            ImVec2 sp = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##preset", ImVec2(22.0f, 22.0f))) { *color = COLOR_PRESETS[i]; changed = true; }
            const ImVec4& pc = COLOR_PRESETS[i];
            bool same = pc.x == color->x && pc.y == color->y && pc.z == color->z;
            ImVec2 sc = Add(sp, 11, 11);
            if (same || ImGui::IsItemHovered()) pdl->AddCircle(sc, 11.0f, Px(same ? WHITE : LINE4), 24, 1.5f);
            pdl->AddCircleFilled(sc, 8.0f, Px(ImGui::ColorConvertFloat4ToU32(pc)), 24);
            ImGui::PopID();
        }
        ImGui::SetNextItemWidth(176.0f);
        if (ImGui::ColorPicker4("##picker", &color->x,
                ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueBar))
            changed = true;
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);

    ImGui::PopID();
    ImGui::PopID();
    return changed;
}

bool Button(const char* label, bool primary)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    ImGui::Dummy(ImVec2(g_row_w, 6.0f));
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##btn", ImVec2(g_row_w, 42.0f));
    bool hovered = ImGui::IsItemHovered();
    bool active  = ImGui::IsItemActive();
    ImGui::Dummy(ImVec2(g_row_w, 6.0f));

    float hv = Anim(ImGui::GetID("##h"), hovered ? 1.0f : 0.0f, 18.0f);
    float pr = Anim(ImGui::GetID("##p"), active ? 1.0f : 0.0f, 28.0f);

    // Press: shrink slightly around the center
    const float sx = g_row_w * 0.02f * pr, sy = 42.0f * 0.03f * pr;
    ImVec2 a(p.x + sx, p.y + sy), b(p.x + g_row_w - sx, p.y + 42.0f - sy);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (primary)
    {
        DrawGlow(dl, a, b, 10.0f, ACCENT, 1.0f + hv, 4.0f);
        dl->AddRectFilled(a, b, Px(Mix(ACCENT, ACCENT_HI, hv)), 10.0f);
        dl->AddLine(ImVec2(a.x + 10.0f, a.y + 1.0f), ImVec2(b.x - 10.0f, a.y + 1.0f), Px(WHITE, 0.18f), 1.0f);
    }
    else
    {
        dl->AddRectFilled(a, b, Px(Mix(HOVER, HOVER_2, hv)), 10.0f);
        dl->AddRect(a, b, Px(Mix(LINE3, LINE4, hv)), 10.0f, 0, 1.0f);
    }
    ImVec2 ts = TextSize(F_SEMI, label);
    DrawLabel(dl, F_SEMI, ImVec2((a.x + b.x - ts.x) * 0.5f, (a.y + b.y - ts.y) * 0.5f), WHITE, label);

    ImGui::PopID();
    ImGui::PopID();
    return pressed;
}
} // namespace ui

// ----------------------------------------------------------------------------
// Sidebar tab
// ----------------------------------------------------------------------------
static bool TabButton(const char* label, int icon, bool active)
{
    ImGui::PushID(label);
    ImGui::PushID(g_serial++);
    const ImVec2 size(164.0f, 44.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##tab", size);
    bool hovered = ImGui::IsItemHovered();
    float t  = Anim(ImGui::GetID("##t"), active ? 1.0f : 0.0f, 16.0f);
    float hv = Anim(ImGui::GetID("##h"), hovered ? 1.0f : 0.0f, 18.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 b = Add(p, size.x, size.y);
    if (hv * (1.0f - t) > 0.01f) dl->AddRectFilled(p, b, Px(HOVER, hv * (1.0f - t)), 10.0f);
    if (t > 0.01f)
    {
        dl->AddRectFilled(p, b, Px(ACCENT, 0.12f * t), 10.0f);
        dl->AddRect(p, b, Px(ACCENT, 0.35f * t), 10.0f, 0, 1.0f);
    }

    DrawIcon(dl, icon, Add(p, 12.0f, 13.0f), 18.0f, Px(Mix(MUTED, ACCENT_TEXT, t)));
    ImVec2 ts = TextSize(F_BODY, label);
    DrawLabel(dl, F_BODY, Add(p, 42.0f, (size.y - ts.y) * 0.5f), Mix(MUTED, WHITE, Max(t, hv * 0.7f)), label);

    if (t > 0.01f)
    {
        ImVec2 dot = Add(p, size.x - 15.0f, size.y * 0.5f);
        dl->AddCircleFilled(dot, 6.0f, Px(ACCENT, 0.25f * t), 16);
        dl->AddCircleFilled(dot, 3.0f, Px(ACCENT, t), 16);
    }

    ImGui::PopID();
    ImGui::PopID();
    return pressed;
}

// ----------------------------------------------------------------------------
// Tabs
// ----------------------------------------------------------------------------
static const char* CHAM_STYLES[] = { "Flat", "Ice", "Plasma", "Ember", "Obsidian", "Abyss", "Pulse", "Invisible", "Marble" };
static const char* BONES[] = { "Head", "Neck", "Chest", "Pelvis", "Closest", "Arms", "Legs", "Face", "Hands", "Feet" };
static const char* BINDS[] = { "Hold", "Toggle", "Always" };
static const char* AIM_MODES[] = { "Mouse", "Camlock" };
static const char* OOF_STYLES[] = { "Arrow", "Dot", "Chevron" };


static void TabAimbot(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("General", col);
    ui::Toggle  ("Enable", &s.aim_enabled);
    ui::Keybind ("Aim key", &s.aim_key);
    ui::Dropdown("Mode", &s.aim_mode, AIM_MODES, 2);
    ui::Dropdown("Activation", &s.aim_bind, BINDS, 3);
    ui::Dropdown("Target bone", &s.aim_bone, BONES, 10);
    ui::Slider  ("Max distance", &s.aim_dist, 0, 1000, "%d m");
    ui::Slider  ("Smoothing", &s.aim_smooth, 0, 20);
    ui::Slider  ("Mouse calibration", &s.aim_cal, 10, 2000, "%d %%");
    ui::Toggle  ("Sticky target", &s.aim_sticky);
    ui::Toggle  ("Closest first", &s.aim_closest);
    ui::Toggle  ("Visibility check", &s.aim_los);
    ui::Toggle  ("Skip party", &s.aim_team);
    ui::Toggle  ("Target clan", &s.aim_clan);
    ui::Toggle  ("Target police", &s.aim_police);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Field of view", col);
    ui::Toggle("Draw circle", &s.aim_fov_circle);
    ui::Toggle("Fill", &s.aim_fov_fill);
    ui::Toggle("Gradient fill", &s.aim_fov_gradient);
    ui::Toggle("Spin", &s.aim_fov_spin);
    ui::Slider("Radius", &s.aim_fov, 30, 600, "%d px");
    ui::Slider("Opacity", &s.aim_fov_opacity, 0, 100, "%d %%");
    ui::Slider("Spin speed", &s.aim_fov_speed, 15, 360);
    ui::Slider("Camera FOV", &s.cam_fov, 0, 160);
    ui::Color ("Circle color", &s.color[C_FOV]);
    ui::Color ("Fill color", &s.color[C_FOV_FILL]);
    ui::Color ("Gradient color", &s.color[C_FOV_GRADIENT]);
    ui::EndCard();
}

static void TabVisual(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("Players", col);
    ui::Toggle("Enable", &s.esp_enabled);
    ui::Toggle("Boxes", &s.esp_box);
    ui::Toggle("Skeleton", &s.esp_skeleton);
    ui::Toggle("Names", &s.esp_name);
    ui::Toggle("Health bar", &s.esp_health);
    ui::Toggle("Armor", &s.esp_armor);
    ui::Toggle("Distance", &s.esp_distance);
    ui::Toggle("Weapon", &s.esp_weapon);
    ui::Toggle("Team tag", &s.esp_team);
    ui::Toggle("Clan tag", &s.esp_clan);
    ui::Toggle("Police tag", &s.esp_police);
    ui::Toggle("Show self", &s.esp_self);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Player style", col);
    ui::Slider("Max distance", &s.esp_max_dist, 0, 5000, "%d m");
    ui::Slider("Box thickness", &s.esp_thickness, 1, 4, "%d px");
    ui::Slider("Skeleton width", &s.esp_skeleton_width, 1, 4, "%d px");
    ui::Toggle("Health gradient", &s.esp_health_gradient);
    ui::Color ("Box color", &s.color[C_BOX]);
    ui::Color ("Name color", &s.color[C_NAME]);
    ui::Color ("Weapon color", &s.color[C_WEAPON]);
    ui::Color ("Distance color", &s.color[C_DISTANCE]);
    ui::Color ("Skeleton color", &s.color[C_SKELETON]);
    ui::Color ("HP high", &s.color[C_HP_HIGH]);
    ui::Color ("HP mid", &s.color[C_HP_MID]);
    ui::Color ("HP low", &s.color[C_HP_LOW]);
    ui::EndCard();

    ui::BeginCard("Chams", col);
    ui::Toggle  ("Enable", &s.cham_on);
    ui::Toggle  ("Tint", &s.cham_tint);
    ui::Dropdown("Style", &s.cham_style, CHAM_STYLES, 9);
    ui::Slider  ("Opacity", &s.cham_opacity, 0, 100, "%d %%");
    ui::Toggle  ("Materials", &s.cham_materials);
    ui::Toggle  ("Visibility check", &s.cham_vis_check);
    ui::Color   ("Cham color", &s.color[C_CHAM]);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Local and vehicles", col);
    ui::Toggle  ("Local chams", &s.local_chams);
    ui::Toggle  ("Local tint", &s.local_chams_tint);
    ui::Dropdown("Local style", &s.local_chams_style, CHAM_STYLES, 9);
    ui::Slider  ("Local opacity", &s.local_chams_opacity, 0, 100, "%d %%");
    ui::Color   ("Local cham color", &s.color[C_LOCAL_CHAM]);
    ui::Toggle  ("Vehicle chams", &s.vehicle_chams);
    ui::Dropdown("Vehicle style", &s.vehicle_chams_style, CHAM_STYLES, 9);
    ui::Slider  ("Vehicle opacity", &s.vehicle_chams_opacity, 0, 100, "%d %%");
    ui::Color   ("Vehicle color", &s.color[C_VEHICLE_CHAM]);
    ui::EndCard();

    ui::BeginCard("Radar", col);
    ui::Toggle("Enable", &s.radar_on);
    ui::Slider("Range", &s.radar_range, 25, 500, "%d m");
    ui::Toggle("Rotate with world", &s.radar_world);
    ui::Toggle("Vehicle markers", &s.vehicle_markers);
    ui::Toggle("Vehicle occupants", &s.vehicle_occupants);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Your player", col);
    ui::Toggle("Box", &s.local_box);
    ui::Toggle("Skeleton", &s.local_skeleton);
    ui::Toggle("Name", &s.local_name);
    ui::Toggle("Weapon", &s.local_weapon);
    ui::Toggle("Distance", &s.local_distance);
    ui::Toggle("Armor", &s.local_armor);
    ui::Color ("Box color", &s.color[C_LOCAL_BOX]);
    ui::Color ("Name color", &s.color[C_LOCAL_NAME]);
    ui::Color ("Skeleton color", &s.color[C_LOCAL_SKELETON]);
    ui::Color ("Armor color", &s.color[C_LOCAL_ARMOR]);
    ui::EndCard();
}

static void TabLoot(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("Loot", col);
    ui::Toggle("Enable", &s.loot_on);
    ui::Toggle("Loot nodes", &s.loot_nodes);
    ui::Toggle("Containers", &s.loot_containers);
    ui::Toggle("Doors", &s.loot_doors);
    ui::Toggle("Dropped items", &s.loot_dropped);
    ui::Toggle("Vehicle loot", &s.loot_vehicle);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Tiers", col);
    ui::Toggle("No rarity", &s.loot_t_none);
    ui::Toggle("Common", &s.loot_t_common);
    ui::Toggle("Uncommon", &s.loot_t_uncommon);
    ui::Toggle("Rare", &s.loot_t_rare);
    ui::Toggle("Epic", &s.loot_t_epic);
    ui::Toggle("Legendary", &s.loot_t_legendary);
    ui::Toggle("Exotic", &s.loot_t_exotic);
    ui::EndCard();
}

static void TabWorld(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("Light and sky", col);
    ui::Toggle("Boost sun", &s.world_sun);
    ui::Slider("Sun strength", &s.world_sun_pct, 100, 800, "%d %%");
    ui::Toggle("Disable sky", &s.world_sky_off);
    ui::Toggle("Custom sky color", &s.world_sky_color);
    ui::Color ("Sky color", &s.color[C_SKY]);
    ui::Slider("Sky glow", &s.world_sky_glow, 0, 300);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Fog and weather", col);
    ui::Toggle("Disable fog", &s.world_fog_off);
    ui::Toggle("Custom fog", &s.world_fog_custom);
    ui::Slider("Fog density", &s.world_fog_density, 1, 200);
    ui::Slider("Fog distance", &s.world_fog_distance, 0, 1000, "%d m");
    ui::Color ("Fog color", &s.color[C_FOG]);
    ui::Slider("Rain", &s.world_rain, 0, 100, "%d %%");
    ui::EndCard();
}

static void TabMisc(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("Screen", col);
    ui::Toggle("Custom crosshair", &s.misc_crosshair);
    ui::Toggle("Watermark", &s.misc_watermark);
    ui::Toggle("Tracers", &s.tracers);
    ui::Slider("Tracer duration", &s.tracer_ms, 100, 4000, "%d ms");
    ui::Slider("Tracer width", &s.tracer_width, 5, 40);
    ui::Slider("Tracer glow", &s.tracer_glow, 0, 20);
    ui::Color ("Tracer color", &s.color[C_TRACER]);
    ui::EndCard();

    ImGui::SameLine(0.0f, 16.0f);

    ui::BeginCard("Offscreen and menu", col);
    ui::Toggle  ("Offscreen arrows", &s.oof_on);
    ui::Dropdown("Arrow style", &s.oof_style, OOF_STYLES, 3);
    ui::Slider  ("Arrow size", &s.oof_size, 3, 25);
    ui::Slider  ("Arrow radius", &s.oof_radius, 30, 500);
    ui::Color   ("Arrow color", &s.color[C_OOF]);
    ui::KeyDisplay("Open / close", s.menu_key);
    ui::Slider  ("Opacity", &s.opacity, 40, 100, "%d %%");
    ui::EndCard();
}

static void TabConfig(Settings& s, float w)
{
    const float col = (w - 16.0f) * 0.5f;

    ui::BeginCard("Profile", col);
    if (ui::Button("Save", true)) s.save_requested = true;
    if (ui::Button("Load"))       s.load_requested = true;
    ui::EndCard();
}


struct TabDef
{
    const char* label;
    const char* desc;
    int icon;
    void (*draw)(Settings&, float);
};

static const TabDef TABS[] = {
    { "Aimbot", "Aim, precision and target",   ICON_CROSSHAIR, TabAimbot },
    { "Visual", "Everything drawn on screen",  ICON_EYE,       TabVisual },
    { "Loot",   "Items, tiers and containers", ICON_EYE,       TabLoot   },
    { "World",  "Light, sky, fog and rain",    ICON_LAYERS,    TabWorld  },
    { "Misc",   "Extra options and interface", ICON_SLIDERS,   TabMisc   },
    { "Config", "Save and load your settings", ICON_LAYERS,    TabConfig },
};

// ----------------------------------------------------------------------------
// Window
// ----------------------------------------------------------------------------
void Render(Settings& s, const Fonts& fonts, bool* open)
{
    const ImVec2 size(880.0f, 640.0f);
    const float title_h = 54.0f;     // 2 px brand line + 52 px bar
    const float footer_y = 608.0f;
    const float side_w = 188.0f;
    static int tab = 0;

    g_fonts = &fonts;
    float alpha = s.opacity / 100.0f;
    g_serial = 0;
    PushMenuStyle(alpha < 0.2f ? 0.2f : (alpha > 1.0f ? 1.0f : alpha));

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
    if (!ImGui::Begin("##eternal", nullptr, flags))
    {
        ImGui::End();
        PopMenuStyle();
        g_fonts = nullptr;
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 wp = ImGui::GetWindowPos();

    dl->AddRectFilled(wp, Add(wp, size.x, size.y), Px(WIN), 14.0f);
    dl->AddRectFilled(wp, Add(wp, size.x, title_h), Px(ACCENT), 14.0f, ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled(Add(wp, 0, 2), Add(wp, size.x, title_h), Px(BAR), 14.0f, ImDrawFlags_RoundCornersTop);
    dl->AddLine(Add(wp, 0, title_h + 0.5f), Add(wp, size.x, title_h + 0.5f), Px(LINE));
    dl->AddRectFilled(Add(wp, 0, title_h + 1), Add(wp, side_w, footer_y), Px(BAR));
    dl->AddLine(Add(wp, side_w + 0.5f, title_h + 1), Add(wp, side_w + 0.5f, footer_y), Px(LINE));
    dl->AddRectFilled(Add(wp, 0, footer_y), Add(wp, size.x, size.y), Px(BAR), 14.0f, ImDrawFlags_RoundCornersBottom);
    dl->AddLine(Add(wp, 0, footer_y + 0.5f), Add(wp, size.x, footer_y + 0.5f), Px(LINE));
    dl->AddRect(wp, Add(wp, size.x, size.y), Px(LINE2), 14.0f, 0, 1.0f);

    ImVec2 logo = Add(wp, 20.0f, 2.0f + 11.0f);
    DrawGlow(dl, logo, Add(logo, 30, 30), 8.0f, ACCENT, 1.0f, 3.0f);
    dl->AddRectFilled(logo, Add(logo, 30, 30), Px(ACCENT), 8.0f);
    DrawInfinity(dl, Add(logo, 5, 5), 20.0f, Px(WHITE));

    ImVec2 ns = TextSize(F_LOGO, "ETERNAL SERVICES");
    float name_x = logo.x + 42.0f;
    float name_w = DrawLabelSpaced(dl, F_LOGO, ImVec2(name_x, logo.y + (30.0f - ns.y) * 0.5f), TEXT_HI, "ETERNAL SERVICES", 2.2f);

    ImVec2 vs = TextSize(F_MONO, "v1.0");
    ImVec2 va(name_x + name_w + 12.0f, logo.y + 5.0f), vb(va.x + vs.x + 16.0f, logo.y + 25.0f);
    dl->AddRectFilled(va, vb, Px(ACCENT, 0.12f), 10.0f);
    dl->AddRect(va, vb, Px(ACCENT, 0.35f), 10.0f, 0, 1.0f);
    DrawLabel(dl, F_MONO, ImVec2(va.x + 8.0f, va.y + (20.0f - vs.y) * 0.5f), ACCENT_TEXT, "v1.0");

    for (int i = 0; i < 2; ++i)
    {
        const bool is_close = (i == 1);
        ImVec2 bp = Add(wp, size.x - 14.0f - 34.0f - (is_close ? 0.0f : 38.0f), 2.0f + 9.0f);
        ImGui::SetCursorScreenPos(bp);
        bool pressed = ImGui::InvisibleButton(is_close ? "##close" : "##min", ImVec2(34, 34));
        float hv = Anim(ImGui::GetID(is_close ? "##close_h" : "##min_h"), ImGui::IsItemHovered() ? 1.0f : 0.0f, 18.0f);
        if (hv > 0.01f) dl->AddRectFilled(bp, Add(bp, 34, 34), Px(is_close ? CLOSE_HOVER : HOVER, hv), 8.0f);
        ImU32 ic = Px(Mix(MUTED, is_close ? ACCENT_TEXT : TEXT_HI, hv));
        ImVec2 c = Add(bp, 17, 17);
        if (is_close)
        {
            dl->AddLine(Add(c, -5, -5), Add(c, 5, 5), ic, 1.6f);
            dl->AddLine(Add(c, 5, -5), Add(c, -5, 5), ic, 1.6f);
        }
        else
            dl->AddLine(Add(c, -6, 0), Add(c, 6, 0), ic, 1.6f);
        if (pressed && open) *open = false;
    }

    const int tab_count = (int)(sizeof(TABS) / sizeof(TABS[0]));
    for (int i = 0; i < tab_count; ++i)
    {
        ImGui::SetCursorScreenPos(Add(wp, 12.0f, title_h + 8.0f + i * 40.0f));
        if (TabButton(TABS[i].label, TABS[i].icon, tab == i)) tab = i;
    }

    ImGui::SetCursorScreenPos(Add(wp, side_w + 1.0f, title_h + 1.0f));
    ImGui::BeginChild("##content", ImVec2(size.x - side_w - 1.0f, footer_y - title_h - 1.0f), 0, 0);
    {
        ImGui::SetCursorPos(ImVec2(24.0f, 20.0f));
        const float w = ImGui::GetContentRegionAvail().x - 24.0f;
        ImGui::BeginGroup();

        ImDrawList* cdl = ImGui::GetWindowDrawList();
        ImVec2 hp = ImGui::GetCursorScreenPos();
        ImVec2 hs = TextSize(F_H1, TABS[tab].label);
        ImVec2 ds = TextSize(F_BODY, TABS[tab].desc);
        DrawLabel(cdl, F_H1, hp, TEXT_HI, TABS[tab].label);
        DrawLabel(cdl, F_BODY, ImVec2(hp.x + w - ds.x, hp.y + hs.y - ds.y - 2.0f), MUTED, TABS[tab].desc);
        ImGui::Dummy(ImVec2(w, hs.y + 16.0f));

        TABS[tab].draw(s, w);

        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(1.0f, 20.0f));
    }
    ImGui::EndChild();

    char kb[64], footer[96];
    snprintf(footer, sizeof(footer), "%s to open / close", KeyLabel(s.menu_key, kb, sizeof(kb)));
    ImVec2 fs = TextSize(F_MONO, footer);
    DrawLabel(dl, F_MONO, ImVec2(wp.x + 20.0f, wp.y + footer_y + (32.0f - fs.y) * 0.5f), MUTED, footer);

    ImGui::End();
    PopMenuStyle();
    g_fonts = nullptr;
}

} // namespace eternal

