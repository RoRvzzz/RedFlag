#define IMGUI_DEFINE_MATH_OPERATORS
#include "render.h"
#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <D3DX11.h>
using namespace ImGui;
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#include <d3dx11tex.h>
#include <features/cheats/visuals/visuals.h>
#include <framework/settings/functions.h>
#include <imgui/imgui_freetype.h>
#include <framework/data/fonts.h>
ID3D11ShaderResourceView* background{ };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#define ALPHA    ( ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float      | ImGuiColorEditFlags_NoDragDrop   | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
#define NO_ALPHA ( ImGuiColorEditFlags_NoTooltip    | ImGuiColorEditFlags_NoInputs  | ImGuiColorEditFlags_NoLabel  | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float    | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    render_t* instance = reinterpret_cast<render_t*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_SIZE:
        if (instance && instance->detail && instance->detail->swap_chain && wParam != SIZE_MINIMIZED) {
            if (instance->detail->render_target_view)
                instance->detail->render_target_view->Release();

            instance->detail->swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

            ID3D11Texture2D* back_buffer = nullptr;
            instance->detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

            if (back_buffer) {
                instance->detail->device->CreateRenderTargetView(back_buffer, NULL, &instance->detail->render_target_view);
                back_buffer->Release();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

render_t::render_t() {
    this->detail = std::make_unique<detail_t>();
}

render_t::~render_t() {
    destroy_imgui();
    destroy_window();
    destroy_device();
}

bool render_t::create_window() {
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = wnd_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hIcon = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "MemorySense";

    if (!RegisterClassExA(&wc))
        return false;

    this->detail->window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        "main",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!this->detail->window) {
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }

    SetWindowLongPtrA(this->detail->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    if (!SetLayeredWindowAttributes(this->detail->window, RGB(0, 0, 0), 255, LWA_ALPHA))
        return false;

    MARGINS margins = { -1 };
    if (DwmExtendFrameIntoClientArea(this->detail->window, &margins) != S_OK)
        return false;

    ShowWindow(this->detail->window, SW_SHOW);
    UpdateWindow(this->detail->window);

    return true;
}

bool render_t::create_device() {
    if (!this->detail->window)
        return false;

    DXGI_SWAP_CHAIN_DESC swap_desc{};
    swap_desc.BufferCount = 1;
    swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.OutputWindow = this->detail->window;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.SampleDesc.Quality = 0;
    swap_desc.Windowed = TRUE;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL feature_level;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        levels,
        2,
        D3D11_SDK_VERSION,
        &swap_desc,
        &this->detail->swap_chain,
        &this->detail->device,
        &feature_level,
        &this->detail->device_context
    );

    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            levels,
            2,
            D3D11_SDK_VERSION,
            &swap_desc,
            &this->detail->swap_chain,
            &this->detail->device,
            &feature_level,
            &this->detail->device_context
        );
    }

    if (FAILED(hr) || !this->detail->device || !this->detail->device_context)
        return false;

    ID3D11Texture2D* back_buffer = nullptr;
    hr = this->detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (FAILED(hr) || !back_buffer)
        return false;

    hr = this->detail->device->CreateRenderTargetView(back_buffer, nullptr, &this->detail->render_target_view);
    back_buffer->Release();

    return !FAILED(hr);
}

bool render_t::create_imgui() {
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();
    
    // Disable antialiasing for pixel-perfect bitmap font (ProggyClean)
    ImGuiStyle& style = GetStyle();
    style.AntiAliasedLines = false;
    style.AntiAliasedLinesUseTex = false;
    style.AntiAliasedFill = false;

    if (!ImGui_ImplWin32_Init(this->detail->window))
        return false;

    if (!this->detail->device || !this->detail->device_context)
        return false;

    if (!ImGui_ImplDX11_Init(this->detail->device, this->detail->device_context))
        return false;

    return true;
}

void render_t::destroy_device() {
    if (this->detail->render_target_view) this->detail->render_target_view->Release();
    if (this->detail->swap_chain) this->detail->swap_chain->Release();
    if (this->detail->device_context) this->detail->device_context->Release();
    if (this->detail->device) this->detail->device->Release();
}

void render_t::destroy_window() {
    if (this->detail->window)
        DestroyWindow(this->detail->window);
}

void render_t::destroy_imgui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void render_t::start_render() {
    MSG msg{};
    static bool last_running_state = false;

    const std::chrono::milliseconds frame_time(1000 / 60);
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    // <<<<<<<<<<< ///////////

    // Load Fonts - Pure monochrome monohinted Tahoma
    ImFontConfig font_config;
    font_config.OversampleH = 1;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = true;
    font_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
    font_config.FontDataOwnedByAtlas = false;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    var->font.icons[0] = io.Fonts->AddFontFromMemoryTTF(section_icons_hex, sizeof section_icons_hex, 15.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    var->font.icons[1] = io.Fonts->AddFontFromMemoryTTF(icons_hex, sizeof icons_hex, 5.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());

    var->font.tahoma = io.Fonts->AddFontFromMemoryTTF(tahoma_hex, sizeof tahoma_hex, 13.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());

    // << <<<<<<<<< ///////////
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if (this->limit_fps) {
            /*auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time);
            if (elapsed < frame_time) {
                std::this_thread::sleep_for(frame_time - elapsed);
            }
            last_frame_time = std::chrono::high_resolution_clock::now();*/

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (GetAsyncKeyState(var->gui.menu_key) & 1)
            this->running = !this->running;

        HWND target = FindWindowA(nullptr, "Roblox");
        if (!target || IsIconic(target)) {
            MoveWindow(this->detail->window, 0, 0, 0, 0, true);
        }
        else {
            RECT client_rect;
            if (GetClientRect(target, &client_rect)) {
                POINT client_to_screen_pos = { client_rect.left, client_rect.top };
                ClientToScreen(target, &client_to_screen_pos);
                MoveWindow(this->detail->window, client_to_screen_pos.x, client_to_screen_pos.y, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, true);
            }

        }


        if (last_running_state != this->running) {
            if (this->running) {
                SetWindowLong(this->detail->window, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
            }
            else {
                SetWindowLong(this->detail->window, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
            }
            last_running_state = this->running;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        render_visuals();
        render_menu();

        end_render();
    }
}

void render_t::end_render() {
    using namespace ImGui;
    Render();
    const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    this->detail->device_context->OMSetRenderTargets(1, &this->detail->render_target_view, nullptr);
    this->detail->device_context->ClearRenderTargetView(this->detail->render_target_view, clear_color);
    ImGui_ImplDX11_RenderDrawData(GetDrawData());
    this->detail->swap_chain->Present(0, 0);
}

void render_t::render_menu() {
    if (this->running) {
        gui->render();
    }
}

void render_t::render_visuals() {
    visuals::render();
}