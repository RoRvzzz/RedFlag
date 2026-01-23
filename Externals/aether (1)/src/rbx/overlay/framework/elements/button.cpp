#include "../settings/functions.h"

bool c_gui::button(std::string_view label, int val)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2((GetWindowWidth() - style.WindowPadding.x * 2 - style.ItemSpacing.x * (val - 1)) / val, elements->widgets.dropdown_height));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    ImColor outline_color = draw->get_clr(clr->widgets.stroke_two); 
    ImVec2 outline_offset = ImVec2(2.0f, 2.0f);
    ImRect outline_rect = ImRect(rect.Min - outline_offset, rect.Max + outline_offset);
    draw->rect(window->DrawList, outline_rect.Min, outline_rect.Max, outline_color);

    draw->fade_rect_filled(window->DrawList, rect.Min + ImVec2(2, 2), rect.Max - ImVec2(2, 2), draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(var->window.hover_hightlight && hovered ? clr->accent : clr->widgets.stroke_two));
    draw->rect(window->DrawList, rect.Min + ImVec2(1, 1), rect.Max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
    draw->text_clipped_outline(window->DrawList, var->font.tahoma, rect.Min, rect.Max, draw->get_clr(hovered ? clr->widgets.text : clr->widgets.text_inactive), label.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return pressed;
}
