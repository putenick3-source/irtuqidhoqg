#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "menu_bridge.h"
#include "ui/amphetaminehaxx/ui.hpp"
#include "amphetaminehaxx/vendor/imgui/backends/imgui_impl_dx11.h"
#include "amphetaminehaxx/vendor/imgui/backends/imgui_impl_win32.h"
#include "amphetaminehaxx/vendor/imgui/misc/freetype/imgui_freetype.h"

using Microsoft::WRL::ComPtr;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static constexpr wchar_t menu_class[] = L"RRToolAmphetaminehaxx";
static constexpr int menu_width = 522;
static constexpr int menu_height = 560;
static HANDLE menu_thread;
static HANDLE menu_wake;
static volatile LONG menu_stopping;
static volatile LONG menu_visible;
static volatile LONG menu_hotkey = BR_DEFAULT_MENU_KEY;

BR_Config* br_get(void) {
    static BR_Config config = [] {
        BR_Config defaults{};
        defaults.magic = BR_MAGIC;
        defaults.version = BR_VERSION;
        defaults.hud_accent_rgb = 0x5b7db5;
        defaults.hud_cursor_rgb = 0x141414;
        defaults.hud_outline_rgb = 0x000000;
        defaults.hud_panel_rgb = 0x2b2b2b;
        defaults.hud_inl_rgb = 0x383838;
        defaults.hud_dark_rgb = 0x1e1e1e;
        defaults.hud_text_rgb = 0xffffff;
        defaults.hud_muted_rgb = 0xafafaf;
#define BR_DEFAULT(field, initial, minimum, maximum) defaults.field = initial;
        BR_SETTINGS(BR_DEFAULT)
#undef BR_DEFAULT
        constexpr unsigned char colors[BR_COLOR_COUNT][3] = {
            {43,124,255}, {255,86,86}, {120,255,140}, {86,255,246},
            {158,158,158}, {255,255,255}, {255,255,255}, {255,255,255},
            {255,255,255}, {255,255,255}, {255,255,255}, {120,255,140},
            {255,176,66}, {139,92,246}, {220,220,220},
            {255,255,255}, {255,255,255}, {177,239,255}, {51,179,255},
            {10,77,184}, {255,86,86}, {90,170,255}, {120,255,140},
            {90,170,255}, {190,110,255}, {255,0,200},
            {255,140,60}, {255,90,90}, {110,155,205}, {152,165,180},
            {255,0,200}, {43,124,255}, {255,255,255}, {255,255,255},
            {255,255,255}, {255,255,255}, {255,255,255},
        };
        for (int i = 0; i < BR_COLOR_COUNT; ++i)
            std::copy_n(colors[i], 3, defaults.color[i].rgb);
        return defaults;
    }();
    return &config;
}

void br_set_menu_visible(int visible) {
    br_get()->menu_open = visible != 0;
    InterlockedExchange(&menu_visible, visible != 0);
    if (menu_wake) SetEvent(menu_wake);
}

void br_request_toggle(void) {
    br_set_menu_visible(!InterlockedCompareExchange(&menu_visible, 0, 0));
}

int br_menu_hotkey(void) {
    return InterlockedCompareExchange(&menu_hotkey, 0, 0);
}

void br_set_menu_hotkey(int key) {
    InterlockedExchange(&menu_hotkey, key);
}

static LRESULT CALLBACK menu_window_proc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCALCSIZE && wp) return 0;
    if (msg == WM_CLOSE) {
        br_set_menu_visible(0);
        return 0;
    }
    if (msg == WM_NCHITTEST) {
        POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        RECT client{};
        ScreenToClient(window, &point);
        GetClientRect(window, &client);
        if (point.x >= client.right - 10 && point.y >= client.bottom - 10) return HTBOTTOMRIGHT;
        if (point.y < 26 && point.x < client.right - 27) return HTCAPTION;
    }
    if (msg == WM_GETMINMAXINFO) {
        reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {489, 596};
        return 0;
    }
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(window, msg, wp, lp))
        return 1;
    return DefWindowProcW(window, msg, wp, lp);
}

static void check(HRESULT result, const char* operation) {
    if (FAILED(result)) throw std::runtime_error(operation);
}

static std::filesystem::path executable_directory() {
    wchar_t filename[32768];
    DWORD length = GetModuleFileNameW(nullptr, filename, static_cast<DWORD>(std::size(filename)));
    if (!length || length == std::size(filename)) throw std::runtime_error("Cannot locate executable");
    return std::filesystem::path(filename).parent_path();
}

static void run_menu(HWND window, bool& win32_ready, bool& dx11_ready, const std::filesystem::path& directory) {
    /* The menu is a composition surface, not a swap chain in a window: a plain
     * DXGI swap chain cannot carry per-pixel alpha, so the panel came with an
     * opaque rectangle behind it. A premultiplied flip-model chain handed to
     * DirectComposition clears to fully transparent and only the panel draws. */
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = menu_width;
    desc.Height = menu_height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SampleDesc.Count = 1;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    auto create_device = [&](D3D_DRIVER_TYPE type) {
        return D3D11CreateDevice(nullptr, type, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                 nullptr, 0, D3D11_SDK_VERSION,
                                 &device, nullptr, &context);
    };
    if (FAILED(create_device(D3D_DRIVER_TYPE_HARDWARE)))
        check(create_device(D3D_DRIVER_TYPE_WARP), "Cannot create Amphetaminehaxx D3D11 device");

    ComPtr<IDXGIDevice> dxgi_device;
    check(device.As(&dxgi_device), "Cannot query the Amphetaminehaxx DXGI device");
    ComPtr<IDXGIAdapter> adapter;
    check(dxgi_device->GetAdapter(&adapter), "Cannot query the Amphetaminehaxx adapter");
    ComPtr<IDXGIFactory2> factory;
    check(adapter->GetParent(IID_PPV_ARGS(&factory)), "Cannot query the DXGI factory");
    ComPtr<IDXGISwapChain1> swap_chain;
    check(factory->CreateSwapChainForComposition(device.Get(), &desc, nullptr, &swap_chain),
          "Cannot create the Amphetaminehaxx composition swap chain");
    ComPtr<IDCompositionDevice> compositor;
    check(DCompositionCreateDevice(dxgi_device.Get(), IID_PPV_ARGS(&compositor)),
          "Cannot create the Amphetaminehaxx compositor");
    ComPtr<IDCompositionTarget> comp_target;
    check(compositor->CreateTargetForHwnd(window, TRUE, &comp_target),
          "Cannot create the Amphetaminehaxx composition target");
    ComPtr<IDCompositionVisual> comp_visual;
    check(compositor->CreateVisual(&comp_visual), "Cannot create the Amphetaminehaxx visual");
    check(comp_visual->SetContent(swap_chain.Get()), "Cannot bind the Amphetaminehaxx surface");
    check(comp_target->SetRoot(comp_visual.Get()), "Cannot bind the Amphetaminehaxx visual");
    check(compositor->Commit(), "Cannot commit the Amphetaminehaxx visual");

    ComPtr<ID3D11RenderTargetView> target;
    auto bind_backbuffer = [&](int width, int height) {
        target.Reset();
        ComPtr<ID3D11Texture2D> backbuffer;
        check(swap_chain->GetBuffer(0, IID_PPV_ARGS(backbuffer.GetAddressOf())),
              "Cannot get Amphetaminehaxx back buffer");
        check(device->CreateRenderTargetView(backbuffer.Get(), nullptr, target.GetAddressOf()),
              "Cannot create Amphetaminehaxx render target");
        amphetaminehaxx::menu_size = {static_cast<float>(width), static_cast<float>(height)};
    };
    bind_backbuffer(menu_width, menu_height);

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    win32_ready = ImGui_ImplWin32_Init(window);
    if (!win32_ready) throw std::runtime_error("Cannot initialize Amphetaminehaxx Win32 backend");
    dx11_ready = ImGui_ImplDX11_Init(device.Get(), context.Get());
    if (!dx11_ready) throw std::runtime_error("Cannot initialize Amphetaminehaxx D3D11 backend");
    amphetaminehaxx::initialize(directory / "daydream-data");
    /* The panel fills the window 1:1 and the host moves the window itself, so
     * the panel must not be drawn at an inset inside it. */
    amphetaminehaxx::menu_pos = {0, 0};
    amphetaminehaxx::drag_moves_window = true;

    LONG width = menu_width, height = menu_height;
    bool shown = false;
    bool capture_excluded = false;
    while (!InterlockedCompareExchange(&menu_stopping, 0, 0)) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        const bool visible = InterlockedCompareExchange(&menu_visible, 0, 0) != 0;
        if (visible != shown) {
            ShowWindow(window, visible ? SW_SHOW : SW_HIDE);
            if (visible) SetForegroundWindow(window);
            shown = visible;
            amphetaminehaxx::set_visible(visible);
        }
        if (!shown) {
            MsgWaitForMultipleObjects(1, &menu_wake, FALSE, INFINITE, QS_ALLINPUT);
            continue;
        }

        RECT client{};
        GetClientRect(window, &client);
        if (client.right != width || client.bottom != height) {
            width = client.right;
            height = client.bottom;
            context->OMSetRenderTargets(0, nullptr, nullptr);
            target.Reset();
            check(swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
                  "Cannot resize Amphetaminehaxx swap chain");
            check(compositor->Commit(), "Cannot commit the Amphetaminehaxx resize");
            bind_backbuffer(width, height);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        amphetaminehaxx::render();
        ImGui::Render();
        // A close button can change visibility while building this frame.
        if (!InterlockedCompareExchange(&menu_visible, 0, 0)) continue;
        if (capture_excluded != amphetaminehaxx::excludes_capture()) {
            capture_excluded = amphetaminehaxx::excludes_capture();
            SetWindowDisplayAffinity(window, capture_excluded ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
        }
        const float clear[] = {0, 0, 0, 0};
        context->ClearRenderTargetView(target.Get(), clear);
        context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (!InterlockedCompareExchange(&menu_visible, 0, 0)) continue;
        check(swap_chain->Present(amphetaminehaxx::vsync ? 1 : 0, 0), "Cannot present Amphetaminehaxx");
        if (!amphetaminehaxx::vsync) MsgWaitForMultipleObjects(1, &menu_wake, FALSE, 8, QS_ALLINPUT);
    }
}

static DWORD WINAPI menu_main(LPVOID) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = menu_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = menu_class;
    HWND window = nullptr;
    bool win32_ready = false, dx11_ready = false;
    std::filesystem::path directory;
    try {
        directory = executable_directory();
        if (!RegisterClassW(&window_class)) throw std::runtime_error("Cannot register Amphetaminehaxx window");
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP, menu_class, L"Amphetaminehaxx",
            WS_POPUP | WS_THICKFRAME, work.left + (work.right - work.left - menu_width) / 2,
            work.top + (work.bottom - work.top - menu_height) / 2, menu_width, menu_height,
            nullptr, nullptr, instance, nullptr);
        if (!window) throw std::runtime_error("Cannot create Amphetaminehaxx window");
        run_menu(window, win32_ready, dx11_ready, directory);
    } catch (const std::exception& error) {
        std::ofstream(directory / "daydream-error.log", std::ios::app) << error.what() << '\n';
    }
    if (dx11_ready) ImGui_ImplDX11_Shutdown();
    if (win32_ready) ImGui_ImplWin32_Shutdown();
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
    if (window) DestroyWindow(window);
    UnregisterClassW(menu_class, instance);
    return 0;
}

int br_start_menu(void) {
    if (menu_thread) return 0;
    menu_wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!menu_wake) return 1;
    InterlockedExchange(&menu_stopping, 0);
    menu_thread = CreateThread(nullptr, 0, menu_main, nullptr, 0, nullptr);
    if (menu_thread) return 0;
    CloseHandle(menu_wake);
    menu_wake = nullptr;
    return 1;
}

void br_stop_menu(void) {
    if (!menu_thread) return;
    InterlockedExchange(&menu_stopping, 1);
    SetEvent(menu_wake);
    WaitForSingleObject(menu_thread, INFINITE);
    CloseHandle(menu_thread);
    CloseHandle(menu_wake);
    menu_thread = menu_wake = nullptr;
}
