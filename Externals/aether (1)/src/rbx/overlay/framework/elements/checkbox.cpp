#include "../settings/functions.h"

bool c_gui::checkbox(std::string_view label, bool* callback)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label.data());

	const ImVec2 pos = window->DC.CursorPos;
	const float width = GetContentRegionAvail().x;
	const ImRect rect(pos, pos + ImVec2(width, elements->widgets.checkbox_size.y));
	const ImRect clickable(rect.Min, rect.Min + elements->widgets.checkbox_size);
	const ImRect text(clickable.Min, clickable.Max + ImVec2(elements->widgets.padding.x + var->font.tahoma->CalcTextSizeA(var->font.tahoma->FontSize, FLT_MAX, -1.f, label.data()).x, 0));

	ItemSize(rect, style.FramePadding.y);
	if (!ItemAdd(rect, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(text, id, &hovered, &held);
	if (pressed)
		*callback = !(*callback);

	ImVec2 outline_min = clickable.Min - ImVec2(1.5f, 1.5f);
	ImVec2 outline_max = clickable.Max + ImVec2(1.5f, 1.5f);

	draw->rect(window->DrawList, outline_min, outline_max, (0.0f, 0.0f, 0.0f, 1.0f), 0.f, 0, 1.5f);
	draw->fade_rect_filled(window->DrawList, clickable.Min + ImVec2(2, 2), clickable.Max - ImVec2(2, 2), draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
	if (*callback)
		draw->fade_rect_filled(window->DrawList, clickable.Min + ImVec2(2, 2), clickable.Max - ImVec2(2, 2), draw->get_clr(clr->accent), draw->get_clr({ clr->accent.Value.x - 0.2f, clr->accent.Value.y - 0.2f, clr->accent.Value.z - 0.2f, 1.f }), fade_direction::vertically);
	draw->rect(window->DrawList, clickable.Min, clickable.Max, draw->get_clr(var->window.hover_hightlight && hovered ? clr->accent : clr->widgets.stroke_two));
	draw->rect(window->DrawList, clickable.Min + ImVec2(1, 1), clickable.Max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));

	draw->text_clipped_outline(window->DrawList, var->font.tahoma, ImVec2(clickable.Max.x + elements->widgets.padding.x, rect.Min.y), rect.Max, draw->get_clr(clr->widgets.text), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

	return pressed;
}

bool c_gui::checkbox(std::string_view label, bool* callback, int* key, int* mode)
{
	float y_pos = GetCursorPosY();
	gui->checkbox(label, callback);
	return *callback;

	ImVec2 stored_pos = GetCursorPos();
	set_cursor_pos(ImVec2(GetWindowWidth() - elements->widgets.key_size.x - GetStyle().WindowPadding.x, y_pos));
	gui->KeybindTest((std::stringstream{} << label << "key").str().c_str(), key, mode);
	set_cursor_pos(stored_pos);
}
