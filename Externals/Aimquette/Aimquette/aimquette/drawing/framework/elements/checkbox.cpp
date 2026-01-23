#include "../settings/functions.h"

bool c_gui::checkbox(std::string_view label, bool* callback, float width_override)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const float width = width_override > 0 ? width_override : GetContentRegionAvail().x;
    const ImRect rect(pos, pos + ImVec2(width, elements->widgets.checkbox_size.y));
    const ImRect clickable(rect.Min, rect.Min + elements->widgets.checkbox_size);
    // Constrain text rect to rect.Max when width is overridden
    const float text_width = var->font.tahoma->CalcTextSizeA(var->font.tahoma->FontSize, FLT_MAX, -1.f, label.data()).x;
    const ImVec2 text_end = (width_override > 0) ? ImVec2(ImMin(clickable.Max.x + elements->widgets.padding.x + text_width, rect.Max.x), rect.Max.y) : ImVec2(clickable.Max.x + elements->widgets.padding.x + text_width, rect.Max.y);
    const ImRect text(clickable.Min, text_end);

    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(text, id, &hovered, &held);
    if (pressed)
        *callback = !(*callback);

    draw->fade_rect_filled(window->DrawList, clickable.Min + ImVec2(2, 2), clickable.Max - ImVec2(2, 2), draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
    // Add thin black border
    draw->rect(window->DrawList, clickable.Min, clickable.Max, draw->get_clr(ImColor(0, 0, 0)), 0, 0, 1.0f);
    if (*callback)
        draw->fade_rect_filled(window->DrawList, clickable.Min + ImVec2(2, 2), clickable.Max - ImVec2(2, 2), draw->get_clr(clr->accent), draw->get_clr({ clr->accent.Value.x - 0.2f, clr->accent.Value.y - 0.2f, clr->accent.Value.z - 0.2f, 1.f}), fade_direction::vertically);

    draw->text_clipped_outline(window->DrawList, var->font.tahoma, ImVec2(clickable.Max.x + elements->widgets.padding.x, rect.Min.y), rect.Max, draw->get_clr(clr->widgets.text), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    return pressed;
}

bool c_gui::checkbox(std::string_view label, bool* callback, int* key, int* mode)
{
    float y_pos = GetCursorPosY();
    gui->checkbox(label, callback);

    ImVec2 stored_pos = GetCursorPos();

    set_cursor_pos(ImVec2(GetWindowWidth() - elements->widgets.key_size.x - GetStyle().WindowPadding.x, y_pos));
    bool keybind_result = gui->keybind((std::stringstream{} << label << "key").str().c_str(), key, mode);

    set_cursor_pos(stored_pos); // reset pos
    
    return keybind_result;
}