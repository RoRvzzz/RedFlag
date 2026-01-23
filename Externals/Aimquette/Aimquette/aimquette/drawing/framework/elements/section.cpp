#include "../settings/functions.h"

bool c_gui::section(std::string_view icon, bool* callback)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID((std::stringstream{} << icon << "section").str().c_str());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + elements->section.size);
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);

    if (pressed)
        *callback = !*callback;

    draw->fade_rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->window.background_one), draw->get_clr(clr->window.background_two), fade_direction::vertically);
    draw->rect(window->DrawList, rect.Min, rect.Max, var->window.hover_hightlight && hovered ? draw->get_clr(clr->accent) : draw->get_clr(clr->window.stroke));
    draw->text_clipped(window->DrawList, var->font.icons[0], rect.Min, rect.Max, draw->get_clr(clr->accent, *callback ? 1.f : 0.5f), icon.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    gui->sameline();

    return pressed;
}

bool c_gui::sub_section(std::string_view label, int section_id, int& section_variable, float count)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    const bool selected = section_id == section_variable;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2((GetWindowWidth() - elements->content.window_padding.x * 2 - g.Style.ItemSpacing.x * (count - 1)) / count, elements->section.height));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);

    if (pressed)
        section_variable = section_id;

    if (selected || hovered)
        draw->fade_rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->window.background_one), draw->get_clr(clr->window.background_two), fade_direction::vertically);
    else
        draw->fade_rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);

    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->window.stroke));

    if (selected)
        draw->line(GetForegroundDrawList(), ImVec2(rect.Min.x + 1, rect.Max.y - 1), rect.Max - ImVec2(1, 1), draw->get_clr(clr->window.background_two));

    draw->text_clipped_outline(window->DrawList, var->font.tahoma, rect.Min, rect.Max, selected ? draw->get_clr(clr->accent) : draw->get_clr(clr->widgets.text_inactive), label.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    gui->sameline();

    return pressed;
}
