#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"
#include "colors.h"
#include "variables.h"
#include "elements.h"
#include <vector>
#include <sstream>
#include <string>

using namespace ImGui;

class c_gui
{
public:

    template <typename T>
    T* anim_container(T** state_ptr, ImGuiID id)
    {
        T* state = static_cast<T*>(GetStateStorage()->GetVoidPtr(id));
        if (!state)
            GetStateStorage()->SetVoidPtr(id, state = new T());

        *state_ptr = state;
        return state;
    }

    float fixed_speed(float speed) { return speed / ImGui::GetIO().Framerate; };

    bool begin(std::string_view name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

    void end();

    void push_style_color(ImGuiCol idx, ImU32 col);

    void pop_style_color(int count = 1);

    void push_style_var(ImGuiStyleVar idx, float val);

    void push_style_var(ImGuiStyleVar idx, const ImVec2& val);

    void pop_style_var(int count = 1);

    void push_font(ImFont* font);

    void pop_font();

    void set_cursor_pos(const ImVec2& local_pos);

    void begin_group();

    void end_group();

    void begin_content();

    void end_content();

    void sameline();

    bool begin_def_child(std::string_view name, const ImVec2& size_arg = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);

    void end_def_child();

    void set_next_window_pos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));

    void set_next_window_size(const ImVec2& size, ImGuiCond cond = 0);

    void set_next_window_size_constraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback = NULL, void* custom_callback_data = NULL);

    void begin_child(std::string_view name, int x = 1, int y = 1, const ImVec2& size = ImVec2(0, 0));

    void end_child();

    bool section(std::string_view icon, bool* callback);

    bool sub_section(std::string_view label, int section_id, int& section_variable, float count);

    bool checkbox(std::string_view label, bool* callback);

    bool checkbox(std::string_view label, bool* callback, int* key, int* mode);

    bool checkbox(std::string_view label, bool* callback, float col[4], bool active_alpha = true);

    bool slider_float(std::string_view label, float* v, float v_min, float v_max, bool wo_label = false, const char* format = "%.1f");

    bool slider_int(std::string_view label, int* v, int v_min, int v_max, bool wo_label = false, const char* format = "%d");

    bool dropdown(std::string_view label, int* current_item, const char* const items[], int items_count, bool wo_label = false, int height_in_items = -1);
    
    void multi_dropdown(std::string_view label, std::vector<int>& variable, const char* labels[], int count, bool wo_label = false);

    bool text_field(std::string_view label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0);

    bool button(std::string_view label, int val = 1);

    bool color_edit(std::string_view label, float col[4], bool active_alpha = true);

    bool label_color_edit(std::string_view label, float col[4], bool active_alpha = true);

    bool keybind(const char* label, int* key, int* mode);

    bool label_keybind(const char* label, int* key, int* mode);

    bool begin_table(const char* str_id, int columns_count, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0);

    void end_table();

    void table_next_row(ImGuiTableRowFlags row_flags = 0, float row_min_height = 0);

    bool table_set_column_index(int column_n);

    bool selectable_ex(const char* label, bool active, const ImVec2& size);

    bool selectable(const char* label, bool* p_selected, const ImVec2& size);

    void render();

};

inline c_gui* gui = new c_gui();

enum fade_direction : int
{
    vertically,
    horizontally,
    diagonally,
    diagonally_reversed,
};

class c_draw
{
public:
    ImU32 get_clr(const ImVec4& col, float alpha = 1.f);

    void text(ImDrawList* draw_list, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = NULL, const ImVec4* cpu_fine_clip_rect = 0);
    
    void text_outline(ImDrawList* draw_list, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = NULL, const ImVec4* cpu_fine_clip_rect = 0);

    void text_outline(const char* text);

    void text_clipped(ImDrawList* draw_list, ImFont* font, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 color, const char* text, const char* text_display_end = NULL, const ImVec2* text_size_if_known = NULL, const ImVec2& align = ImVec2(0.f, 0.f), const ImRect* clip_rect = NULL);

    void text_clipped_outline(ImDrawList* draw_list, ImFont* font, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 color, const char* text, const char* text_display_end = NULL, const ImVec2* text_size_if_known = NULL, const ImVec2& align = ImVec2(0.f, 0.f), const ImRect* clip_rect = NULL);

    void radial_gradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out);

    void set_linear_color_alpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1);

    void line(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);

    void rect(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);

    void rect_filled(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);

    void rect_filled_multi_color(ImDrawList* draw, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding = 0.f, ImDrawFlags flags = 0);

    void fade_rect_filled(ImDrawList* draw, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 col_one, ImU32 col_two, fade_direction direction, float rounding = 0.f, ImDrawFlags flags = 0);

    void shadow_rect(ImDrawList* draw_list, const ImVec2& obj_min, const ImVec2& obj_max, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, float obj_rounding = 0.0f);

    void circle(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);

    void circle_filled(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0);

    void shadow_circle(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, int obj_num_segments = 12);

    void triangle(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness = 1.f);

    void triangle_filled(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col);

    void image(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);

    void image_rounded(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, float rounding = 1.f, ImDrawFlags flags = 0);

    void shadow_convex_poly(ImDrawList* draw_list, const ImVec2* points, int points_count, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0);

    void shadow_ngon(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags, int obj_num_segments);

    void rotate_start();

    void rotate_end(float rad, ImVec2 center = ImVec2(0, 0));

    float deg_to_rad(float deg);

    void push_clip_rect(ImDrawList* draw_list, const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false);

    void pop_clip_rect(ImDrawList* draw_list);

    void window_decorations();
};

inline c_draw* draw = new c_draw();

class c_notify
{
public:

};

inline c_notify* notify = new c_notify();
