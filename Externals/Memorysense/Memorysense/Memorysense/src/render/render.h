#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <memory>
#include <d3d11.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>

struct detail_t {
	HWND window = nullptr;
	WNDCLASSEX window_class = {};
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* device_context = nullptr;
	ID3D11RenderTargetView* render_target_view = nullptr;
	IDXGISwapChain* swap_chain = nullptr;
};

class render_t {
public:
	render_t();
	~render_t();
	bool running = true;
	void start_render();
	void render_menu();
	void render_visuals();
	void end_render();
	bool create_device();
	bool create_window();
	bool create_imgui();
	bool limit_fps = true;
	std::unique_ptr<detail_t> detail;
private:
	void destroy_device();
	void destroy_window();
	void destroy_imgui();
};