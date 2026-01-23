#define IMGUI_DEFINE_MATH_OPERATORS
#define STB_IMAGE_IMPLEMENTATION
#include "textures/stb_image.h"
#include <filesystem>
namespace fs = std::filesystem;

#include "overlay.hpp"
#include "imgui/TextEditor.h"
#include <dwmapi.h>

#include "../../misc/configs/configs.hpp"

#include "ckey/keybind.hpp"
#include "../overlay/XorStr/xorstr.hpp"
#include "../overlay/XorStr/json.hpp"
#include "fonts/tomaha.h"
#include <filesystem>
#include <thread>
#include <bitset>
#include "auth.hpp"
#include "skStr.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include "fonts/Verdana.h"
#include "fonts/Arial.h"
#ifdef min
#undef min
#endif
#include <stack>
#ifndef DXGI_PRESENT_ALLOW_TEARING
#define DXGI_PRESENT_ALLOW_TEARING 0x00000200

#endif
#ifdef max
#undef max
#endif
#include <Psapi.h>
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "Notifications.h"
#include "fonts/IconsFontAwesome6.h"
#include "fonts/roboto_medium.h"
#include "../../globals/globals.hpp"
#include "../cheat/features/esp/fonts.h"
static std::vector<std::string> ConfigList;
static int SelectedConfigIndex = 0;
#define HEX_TO_FLOAT3(hex) \
    ((float)((((hex) >> 16) & 0xFF)) / 255.f), \
    ((float)((((hex) >> 8) & 0xFF)) / 255.f), \
    ((float)(((hex) & 0xFF)) / 255.f)

ID3D11Device* overlay::d3d11Device = nullptr;
ID3D11DeviceContext* overlay::d3d11DeviceContext = nullptr;
IDXGISwapChain* overlay::dxgiSwapChain = nullptr;
ID3D11RenderTargetView* overlay::d3d11RenderTargetView = nullptr;
void WidgetTip(std::string tip) {

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(tip.c_str());
	}
}
std::string GetConfigFolderPath()
{

	std::string configFolderPath = RBX::appdata_path() + "\\Void\\configs";

	if (!std::filesystem::exists(configFolderPath))
	{
		std::filesystem::create_directory(configFolderPath);
	}

	return configFolderPath;
}
bool overlay::full_screen(HWND windowHandle)
{
	MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
	if (GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
	{
		RECT windowRect;
		if (GetWindowRect(windowHandle, &windowRect))
		{
			return windowRect.left == monitorInfo.rcMonitor.left
				&& windowRect.right == monitorInfo.rcMonitor.right
				&& windowRect.top == monitorInfo.rcMonitor.top
				&& windowRect.bottom == monitorInfo.rcMonitor.bottom;
		}
	}
}

enum ImFonts_ : int
{
	ImFont_Main = 0,
	ImFont_Icons
};

enum MenuPages_ : int
{
	MenuPage_Home,
	MenuPage_Self,
	MenuPage_Players,
	MenuPage_Misc,
	MenuPage_Scripts,
	MenuPage_Appearance,
	MenuPage_Updates,

	MenuPages_COUNT
};
bool m_bDrawPage[MenuPages_COUNT];


namespace ImGui
{
	bool ColoredButtonV1(const char* label, const ImVec2& size, ImU32 text_color, ImU32 bg_color_1, ImU32 bg_color_2);
}

bool ImGui::ColoredButtonV1(const char* label, const ImVec2& size_arg, ImU32 text_color, ImU32 bg_color_1, ImU32 bg_color_2)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	ImGuiButtonFlags flags = ImGuiButtonFlags_None;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	static std::map<ImGuiID, float> hover_alpha;
	if (hover_alpha.find(id) == hover_alpha.end()) hover_alpha[id] = 0.0f;

	float target_alpha = hovered ? 1.0f : 0.0f;
	hover_alpha[id] = ImLerp(hover_alpha[id], target_alpha, ImGui::GetIO().DeltaTime * 10.0f); // Smooth transition

	ImVec4 bg1f = ColorConvertU32ToFloat4(bg_color_1);
	ImVec4 bg2f = ColorConvertU32ToFloat4(bg_color_2);

	float h1, s1, v1;
	ColorConvertRGBtoHSV(bg1f.x, bg1f.y, bg1f.z, h1, s1, v1);
	v1 = ImClamp(v1 + hover_alpha[id] * 0.15f, 0.0f, 1.0f);
	ColorConvertHSVtoRGB(h1, s1, v1, bg1f.x, bg1f.y, bg1f.z);

	float h2, s2, v2;
	ColorConvertRGBtoHSV(bg2f.x, bg2f.y, bg2f.z, h2, s2, v2);
	v2 = ImClamp(v2 + hover_alpha[id] * 0.15f, 0.0f, 1.0f);
	ColorConvertHSVtoRGB(h2, s2, v2, bg2f.x, bg2f.y, bg2f.z);

	bg_color_1 = GetColorU32(bg1f);
	bg_color_2 = GetColorU32(bg2f);

	RenderNavHighlight(bb, id);

	int vert_start_idx = window->DrawList->VtxBuffer.Size;
	window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_color_1, g.Style.FrameRounding);
	int vert_end_idx = window->DrawList->VtxBuffer.Size;
	if (bg_color_1 != bg_color_2)
		ShadeVertsLinearColorGradientKeepAlpha(window->DrawList, vert_start_idx, vert_end_idx, bb.Min, bb.GetBL(), bg_color_1, bg_color_2);

	if (g.Style.FrameBorderSize > 0.0f)
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), g.Style.FrameRounding, 0, g.Style.FrameBorderSize);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");

	PushStyleColor(ImGuiCol_Text, text_color);
	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
	PopStyleColor();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}



ImColor VecToColor(ImVec4 Color)
{
	return ImColor(
		(int)(Color.x * 255.0f),
		(int)(Color.y * 255.0f),
		(int)(Color.z * 255.0f),
		(int)(Color.w * 255.0f)
	);
}
ImVec4 ColorFromFloat(const float color[3], float alpha = 1.0f)
{
	return ImVec4(color[0], color[1], color[2], alpha);
}

bool overlay::init = false;

bool Keybind(CKeybind* bind, float custom_width)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(bind->get_name().c_str());
	char display[64] = "None";
	ImVec2 pos = window->DC.CursorPos + ImVec2(15.0f, 0); // move to the right
	ImVec2 display_size = ImGui::CalcTextSize(display, NULL, true);
	float width = custom_width == 0 ? 80.0f : custom_width; // smaller default width
	float height = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
	ImVec2 size(width, height);
	ImRect frame_bb(pos, pos + size);
	ImRect total_bb(pos, frame_bb.Max + ImVec2(size.x, 0));

	ImGui::ItemSize(total_bb);
	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	bool hovered = ImGui::ItemHoverable(frame_bb, id, false);
	if (hovered)
	{
		ImGui::SetHoveredID(id);
		g.MouseCursor = ImGuiMouseCursor_Hand;
	}

	bool pressed = hovered && io.MouseClicked[0];
	bool right_click = hovered && io.MouseClicked[1];
	bool released_elsewhere = io.MouseClicked[0] && !hovered;

	if (pressed)
	{
		if (g.ActiveId != id)
		{
			memset(io.KeysDown, 0, sizeof(io.KeysDown));
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			bind->key = 0;
		}
		ImGui::SetActiveID(id, window);
		ImGui::FocusWindow(window);
		bind->waiting_for_input = true;
	}
	else if (released_elsewhere && g.ActiveId == id)
	{
		ImGui::ClearActiveID();
		bind->waiting_for_input = false;
	}
	else if (right_click)
		ImGui::OpenPopup(bind->get_name().c_str());

	bool changed = false;
	int key = bind->key;

	if (bind->waiting_for_input && g.ActiveId == id)
	{
		if (io.MouseClicked[0] && !hovered)
		{
			key = VK_LBUTTON;
			bind->waiting_for_input = false;
			ImGui::ClearActiveID();
			changed = true;
		}
		else
		{
			if (bind->set_key())
			{
				key = bind->key;
				bind->waiting_for_input = false;
				ImGui::ClearActiveID();
				changed = true;
			}
		}
	}

	bind->key = key;

	if (bind->waiting_for_input)
		strcpy_s(display, sizeof display, "...");
	else if (bind->key != 0)
		strcpy_s(display, sizeof display, bind->get_key_name().c_str());
	else
		strcpy_s(display, sizeof display, "None");

	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_ChildBg), style.FrameRounding); // match child background
	if (style.FrameBorderSize > 0.0f)
	{
		window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding, 0, style.FrameBorderSize);
	}

	ImGui::RenderNavHighlight(frame_bb, id);

	ImVec2 text_pos = frame_bb.Min + (frame_bb.Max - frame_bb.Min) * 0.5f - ImGui::CalcTextSize(display, NULL, true) * 0.5f;
	window->DrawList->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), display);

	if (ImGui::BeginPopup(bind->get_name().c_str()))
	{
		if (ImGui::Selectable("Hold", bind->type == CKeybind::HOLD))
			bind->type = CKeybind::HOLD;
		if (ImGui::Selectable("Always", bind->type == CKeybind::ALWAYS))
			bind->type = CKeybind::ALWAYS;
		if (ImGui::Selectable("Toggle", bind->type == CKeybind::TOGGLE))
			bind->type = CKeybind::TOGGLE;
		ImGui::EndPopup();
	}
	return changed;
}


ImVec4 HexToColorVec4(unsigned int hex_color, float alpha)
{
	ImVec4 color;

	color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
	color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
	color.z = (hex_color & 0xFF) / 255.0f;
	color.w = alpha;

	return color;

}




#include <atomic>
#include <mutex>
#include <string>
#include "framework/data/fonts.h"
#include "framework/settings/variables.h"
#include "framework/settings/functions.h"
#include "../cheat/features/features.h"
#include "../../misc/output_system/output/output.hpp"
#include "../cheat/features/esp/font3.h"
#include "imgui/misc/freetype/imgui_freetype.h"
#include "addons/imgui_addons.h"




std::string tm_to_readable_timeA(tm ctx) {
	char buffer[80];

	strftime(buffer, sizeof(buffer), "%a %m/%d/%y %H:%M:%S %Z", &ctx);

	return std::string(buffer);
}

static std::time_t string_to_timetA(std::string timestamp) {
	auto cv = strtol(timestamp.c_str(), NULL, 10); // long

	return (time_t)cv;
}

static std::tm timet_to_tmA(time_t timestamp) {
	std::tm context;

	localtime_s(&context, &timestamp);

	return context;
}





ID3D11Device*  overlay::devicleoad() {
	return d3d11Device; 
}
bool overlay::render() {

	static float fadeAlpha = 0.0f;
	static bool isMenuOpen = true;
	static float fadeSpeed = 1.0f;
	static bool priority_set = false;
	if (!priority_set) {
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		priority_set = true;
	}

	static const ImWchar ranges[] = {
		0x0020, 0x00FF,
		0x0400, 0x052F,
		0x2DE0, 0x2DFF,
		0xA640, 0xA69F,
		0xE000, 0xE226,
		0,
	};

	ImGui_ImplWin32_EnableDpiAwareness();

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszMenuName = nullptr;

	wc.lpszClassName = TEXT("X");
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		DWORD error = GetLastError();
		char errMsg[128];
		sprintf_s(errMsg, "RegisterClassEx failed: GetLastError = %lu", error);
		MessageBoxA(nullptr, errMsg, "error", MB_OK | MB_ICONERROR);
		return false;
	}

	const HWND hw = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
		wc.lpszClassName,
		TEXT("X"),
		WS_POPUP,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	if (!hw) {
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	SetLayeredWindowAttributes(hw, 0, 255, LWA_ALPHA);
	const MARGINS margin = { -1 };
	if (FAILED(DwmExtendFrameIntoClientArea(hw, &margin))) {
		DestroyWindow(hw);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	if (!create_device_d3d(hw)) {
		cleanup_device_d3d();
		DestroyWindow(hw);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	ShowWindow(hw, SW_SHOW);
	UpdateWindow(hw);

	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui::GetIO().IniFilename = nullptr;
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplWin32_Init(hw);
	ImGui_ImplDX11_Init(d3d11Device, d3d11DeviceContext);

	const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	init = true;

	bool draw = true;
	static int brightness = 100;
	static int fogStart = 0, fogEnd = 1000;
	static RBX::Vector3 ambientColor = { 1.0f, 1.0f, 1.0f };
	static RBX::Vector3 colorShiftTop = { 1.0f, 1.0f, 1.0f };
	static RBX::Vector3 colorShiftBottom = { 1.0f, 1.0f, 1.0f };
	static int bloomIntensity = 1, bloomSize = 10, bloomThreshold = 5;
	static int sunRayIntensity = 1, sunRaySpread = 1;
	double doubleValue = 60.0f;
	float fpsvalue = static_cast<float>(doubleValue);
	bool done = false;
	bool holderplacelol = false;
	ImGuiIO& i2342432o = ImGui::GetIO();

	ImFontConfig font_config;
	font_config.OversampleH = 3;
	font_config.OversampleV = 3;
	font_config.PixelSnapH = true;
	font_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
	font_config.FontDataOwnedByAtlas = false;



	ImFont* font_main = i2342432o.Fonts->Fonts[0];
	ImFont* font2342342 = i2342432o.Fonts->AddFontFromMemoryTTF(VisitorFont, sizeof(VisitorFont), 11.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* Arialfontboy = i2342432o.Fonts->AddFontFromMemoryTTF(ArialFDont, sizeof(ArialFDont), 11.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* Verdanafont2 = i2342432o.Fonts->AddFontFromMemoryTTF(Verdanacoolfont, sizeof(Verdanacoolfont), 11.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* TomahaFontboy = i2342432o.Fonts->AddFontFromMemoryTTF(Tomahafontsigma, sizeof(Tomahafontsigma), 11.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* font223 = i2342432o.Fonts->AddFontFromMemoryTTF(font_bytes232, sizeof(font_bytes232), 13.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* CrossHairFont = i2342432o.Fonts->AddFontFromMemoryTTF(font_bytes232, sizeof(font_bytes232), 17.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	ImFont* ExplorerFont = i2342432o.Fonts->AddFontFromMemoryTTF(font_bytes232, sizeof(font_bytes232), 13.0f, &font_config, i2342432o.Fonts->GetGlyphRangesJapanese());
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };


	var->font.icons[0] = i2342432o.Fonts->AddFontFromMemoryTTF(section_icons_hex, sizeof section_icons_hex, 15.f, &font_config, i2342432o.Fonts->GetGlyphRangesCyrillic());
	var->font.icons[1] = i2342432o.Fonts->AddFontFromMemoryTTF(icons_hex, sizeof icons_hex, 5.f, &font_config, i2342432o.Fonts->GetGlyphRangesCyrillic());

	var->font.tahoma = i2342432o.Fonts->AddFontFromMemoryTTF(tahoma_hex, sizeof tahoma_hex, 13.0f, &font_config, i2342432o.Fonts->GetGlyphRangesCyrillic());

	i2342432o.Fonts->AddFontFromMemoryCompressedTTF(roboto_medium_compressed_data, roboto_medium_compressed_size, 12.0f, &font_config, i2342432o.Fonts->GetGlyphRangesDefault());
	i2342432o.Fonts->AddFontDefault(&font_config);

	ImFont* font2 = i2342432o.Fonts->AddFontFromMemoryCompressedTTF(fa6_solid_compressed_data, fa6_solid_compressed_size, 14.0f, &font_config, icons_ranges);

	bool explorer = false;
	bool wm2342342 = false;
	bool KeybindList2 = false;
	ImGuiStyle& style = ImGui::GetStyle();

	ImVec4 accent = HexToColorVec4(0x9896d6, 1);
	ImVec4 sumthings = HexToColorVec4(0x57579c, 1);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.8f);
	style.Colors[ImGuiCol_FrameBg] = HexToColorVec4(0x2c2c2c, 1);
	style.Colors[ImGuiCol_FrameBgActive] = HexToColorVec4(0x2c2c2c, 1);
	style.Colors[ImGuiCol_FrameBgHovered] = HexToColorVec4(0x535353, 1);

	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0, 0, 0, 0);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0, 0, 0, 0);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);

	style.WindowBorderSize = 1.5;
	style.ChildBorderSize = 1.5;
	style.Colors[ImGuiCol_Border] = HexToColorVec4(0x353244, 1);

	style.Colors[ImGuiCol_CheckMark] = sumthings;
	style.Colors[ImGuiCol_Header] = sumthings;
	style.Colors[ImGuiCol_HeaderActive] = sumthings;
	style.Colors[ImGuiCol_HeaderHovered] = ImColor(35, 35, 35);
	style.Colors[ImGuiCol_SliderGrab] = HexToColorVec4(0x6d6d6d, 1);
	style.Colors[ImGuiCol_SliderGrabActive] = HexToColorVec4(0x6d6d6d, 1);
	style.Colors[ImGuiCol_Separator] = accent;
	style.Colors[ImGuiCol_SeparatorActive] = accent;
	style.Colors[ImGuiCol_SeparatorHovered] = accent;

	style.Colors[ImGuiCol_Button] = sumthings;
	style.Colors[ImGuiCol_ButtonActive] = sumthings;
	style.Colors[ImGuiCol_ButtonHovered] = sumthings;

	float menuglow = 50.0f;
	style.WindowShadowSize = menuglow;

	style.Colors[ImGuiCol_WindowShadow] = HexToColorVec4(0x000000, 0.15f);

	while (!done) {
		static bool priority_setA = false;
		if (!priority_setA) {
			::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
			priority_setA = true;
		}

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				done = true;
			}
		}

		if (done)
			break;

		move_window(hw);

		if (GetAsyncKeyState(var->gui.menu_key) & 0x1) {
			
			draw = !draw;

		

		}
	
	



		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuiStyle& style = ImGui::GetStyle();

		

		ImGuiIO& io = ImGui::GetIO();

		ImGui::GetIO().LogFilename = NULL;

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	

		if (GetForegroundWindow() == FindWindowA(0, XorStr("Roblox")) || GetForegroundWindow() == hw) {

			ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
			ImGui::SetNextWindowSize({ 0, 0 }, ImGuiCond_Always);

			if (ImGui::Begin(XorStr("ESP"), nullptr,
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoInputs |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoMouseInputs)) {
				
				static bool priority_setBD = false;
				if (!priority_setBD) {
					::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
					priority_setBD = true;
				}

				ImGui::PushFont(CrossHairFont);
		
				RBX::CrosshairLoop();
				ImGui::PopFont();

				if (globals::fonttype == 0) ImGui::PushFont(font2342342);
				else if (globals::fonttype == 1) ImGui::PushFont(font223);
				else if (globals::fonttype == 2) ImGui::PushFont(TomahaFontboy);
				else if (globals::fonttype == 3) ImGui::PushFont(Verdanafont2);
				else if (globals::fonttype == 4) ImGui::PushFont(Arialfontboy);

				RBX::initVisuals();

				ImGui::PopFont();
				ImGui::End();
			}
		
		}// uigyuig

		if (draw) {
	
			gui->render();


		}
	 // 2e2
		if (draw) {
			SetLayeredWindowAttributes(hw, 0, 255, LWA_ALPHA);
			const MARGINS margin = { -1 };
			DwmExtendFrameIntoClientArea(hw, &margin);
			SetWindowLong(hw, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);

			BringWindowToTop(hw);
			SetForegroundWindow(hw);
		}
		else {
			SetWindowLong(hw, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
			SetLayeredWindowAttributes(hw, 0, 255, LWA_ALPHA);
			const MARGINS margin = { -1 };
			DwmExtendFrameIntoClientArea(hw, &margin);
		}

		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		d3d11DeviceContext->OMSetRenderTargets(1, &d3d11RenderTargetView, nullptr);
		d3d11DeviceContext->ClearRenderTargetView(d3d11RenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		if (globals::allow_tearing) {
			dxgiSwapChain->Present(globals::vsync, DXGI_PRESENT_ALLOW_TEARING);
		}

		dxgiSwapChain->Present(globals::vsync, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(!globals::highcpuusageesp)));
	}

	init = false;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanup_device_d3d();
	DestroyWindow(hw);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
}
bool robloxClosed = false;

void overlay::move_window(HWND hw)
{
	HWND target = FindWindowA(nullptr, "roblox");
	HWND foregroundWindow = GetForegroundWindow();

	if (target != foregroundWindow && hw != foregroundWindow)
	{
		MoveWindow(hw, 0, 0, 0, 0, true);
		return;
	}

	RECT rect;

	if (!GetWindowRect(target, &rect)) {
		if (!robloxClosed) {
			utils::output::error("roblox Closed, Or Overlay Failed");
			robloxClosed = true;
			system("pause");

		}
		return;
	}
	else {
		robloxClosed = false;

	}

	int rsize_x = rect.right - rect.left;
	int rsize_y = rect.bottom - rect.top;

	if (full_screen(target))
	{
		rsize_x += 16;
	}
	else
	{
		rsize_y -= 39;
		rect.left += 4;
		rect.top += 31;
	}

	if (!MoveWindow(hw, rect.left, rect.top, rsize_x, rsize_y, TRUE))
	{
		std::cerr << "failed to move window." << std::endl;
	}
}

bool overlay::create_device_d3d(HWND hw) {
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hw;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL selected_level;
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	//HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, feature_levels, _countof(feature_levels), D3D11_SDK_VERSION, &sd, &dxgiSwapChain, &d3d11Device, &selected_level, &d3d11DeviceContext);
	HRESULT hr2 = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createDeviceFlags,
		feature_levels,
		2,
		D3D11_SDK_VERSION,
		&sd,
		&dxgiSwapChain,
		&d3d11Device,
		&selected_level,
		&d3d11DeviceContext);


	if (FAILED(hr2)) {
		if (hr2 == DXGI_ERROR_UNSUPPORTED) {
			hr2 = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, feature_levels, _countof(feature_levels), D3D11_SDK_VERSION, &sd, &dxgiSwapChain, &d3d11Device, &selected_level, &d3d11DeviceContext);
		}
	}

	create_render_target();
	return true;
}
void overlay::cleanup_device_d3d()
{
	cleanup_render_target();

	if (dxgiSwapChain)
	{
		dxgiSwapChain->Release();
		dxgiSwapChain = nullptr;
	}

	if (d3d11DeviceContext)
	{
		d3d11DeviceContext->Release();
		d3d11DeviceContext = nullptr;
	}

	if (d3d11Device)
	{
		d3d11Device->Release();
		d3d11Device = nullptr;
	}
}

void overlay::create_render_target()
{
	ID3D11Texture2D* back_buffer = nullptr;
	if (SUCCEEDED(dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
	{
		d3d11Device->CreateRenderTargetView(back_buffer, nullptr, &d3d11RenderTargetView);
		back_buffer->Release();
	}
	else
	{
		std::cerr << "failed to get back buffer" << std::endl;
	}
}

void overlay::cleanup_render_target()
{
	if (d3d11RenderTargetView)
	{
		d3d11RenderTargetView->Release();
		d3d11RenderTargetView = nullptr;
	}
}

LRESULT __stdcall overlay::window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (d3d11Device != nullptr && wParam != SIZE_MINIMIZED)
		{
			cleanup_render_target();
			dxgiSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			create_render_target();
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}