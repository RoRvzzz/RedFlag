#include "../settings/functions.h"

bool dd_selectable_ex(const char* label, bool active)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float width = GetContentRegionAvail().x;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(width, 15));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered = IsItemHovered();
    bool pressed = hovered && g.IO.MouseClicked[0];
    if (pressed)
        MarkItemEdited(id);

    draw->text_clipped_outline(window->DrawList, var->font.tahoma, rect.Min + ImVec2(10, -1), rect.Max - ImVec2(0, 1), draw->get_clr(active && !hovered ? clr->accent : hovered ? clr->widgets.text_inactive : clr->widgets.text), label, NULL, NULL, ImVec2(0.f, 0.5f));

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool dd_selectable(const char* label, bool* p_selected)
{
    if (dd_selectable_ex(label, *p_selected))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

static float calc_max_popup_height(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool begin_dropdown_popup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags, int val)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(popup_id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    // Set popup size
    float w = bb.GetWidth();
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    }
    else
    {
        if ((flags & ImGuiComboFlags_HeightMask_) == 0)
            flags |= ImGuiComboFlags_HeightRegular;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
        int popup_max_height_in_items = -1;
        if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
        else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
        else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
        ImVec2 constraint_min(0.0f, 0.0f), constraint_max(FLT_MAX, FLT_MAX);
        if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f) // Don't apply constraints if user specified a size
            constraint_min.x = w;
        if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f)
            constraint_max.y = calc_max_popup_height(popup_max_height_in_items);
        SetNextWindowSize(ImVec2(constraint_min.x, (15 * val + 3 * (val - 1)) + 5));
    }

    // This is essentially a specialized version of BeginPopupEx()
    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginComboDepth); // Recycle windows based on depth

    // Set position given a custom constraint (peak into expected window size so we can position it)
    // FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
    // FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            // Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
            ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
            ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos + ImVec2(0, 2));
        }

    // We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    gui->push_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(3, 3));
    gui->push_style_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    gui->push_style_var(ImGuiStyleVar_PopupBorderSize, 0.f);
    gui->push_style_color(ImGuiCol_Border, draw->get_clr(clr->window.stroke));
    gui->push_style_color(ImGuiCol_PopupBg, draw->get_clr(clr->window.background_one, 0.f));
    bool ret = gui->begin(name, NULL, window_flags);
    
    draw->rect(GetWindowDrawList(), GetWindowPos(), GetWindowPos() + GetWindowSize(), draw->get_clr(clr->widgets.stroke_two));
    draw->rect(GetWindowDrawList(), GetWindowPos() + ImVec2(1, 1), GetWindowPos() + GetWindowSize() - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
    draw->rect_filled(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 2), GetWindowPos() + GetWindowSize() - ImVec2(2, 2), draw->get_clr(clr->window.background_one));

    gui->set_cursor_pos(ImVec2(3, 3));
    gui->begin_group();

    if (!ret)
    {
        EndPopup();
        IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
        return false;
    }
    g.BeginComboDepth++;
    return true;
}

bool begin_dropdown(const char* label, const char* preview_value, bool wo_label, int val, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | (ImGuiComboFlags)ImGuiComboFlags_CustomPreview)) == 0);

    const ImVec2 pos = window->DC.CursorPos;
    const float width = GetContentRegionAvail().x;
    const ImRect rect(pos, pos + ImVec2(width, 14 + elements->widgets.dropdown_height));
    ImRect clickable(rect.Min + ImVec2(0, 14), rect.Max);

    if (wo_label)
        clickable = { pos, pos + ImVec2(width, elements->widgets.dropdown_height) };

    ItemSize(wo_label ? clickable : rect, style.FramePadding.y);
    if (!ItemAdd(wo_label ? clickable : rect, id, &clickable))
        return false;

    // Open on click
    bool hovered, held;
    bool pressed = ButtonBehavior(clickable, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    // Render shape
    draw->fade_rect_filled(window->DrawList, clickable.Min + ImVec2(2, 2), clickable.Max - ImVec2(2, 2), draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
    draw->rect(window->DrawList, clickable.Min, clickable.Max, draw->get_clr(var->window.hover_hightlight && hovered || popup_open ? clr->accent : clr->widgets.stroke_two));
    draw->rect(window->DrawList, clickable.Min + ImVec2(1, 1), clickable.Max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));

    draw->text_clipped_outline(window->DrawList, var->font.tahoma, clickable.Min + ImVec2(10, 0), clickable.Max - ImVec2(20, 0), draw->get_clr(clr->widgets.text), preview_value, NULL, NULL, ImVec2(0.f, 0.5f));

    draw->text_clipped_outline(window->DrawList, var->font.icons[1], clickable.Min, clickable.Max - ImVec2(10, 0), draw->get_clr(clr->widgets.text), popup_open ? "B" : "A", NULL, NULL, ImVec2(1.f, 0.5f));

    if (!wo_label)
        draw->text_clipped_outline(window->DrawList, var->font.tahoma, rect.Min - ImVec2(-1, 2), ImVec2(rect.Max.x, clickable.Min.y - 2), draw->get_clr(clr->widgets.text), label, NULL, NULL, ImVec2(0.f, 0.f));

    if (!popup_open)
        return false;

    g.NextWindowData.Flags = backup_next_window_data_flags;
    return begin_dropdown_popup(popup_id, clickable, flags, val);
}

void end_dropdown()
{
    PopStyleVar(3);
    gui->pop_style_color(2);

    gui->end_group();
    ImGuiContext& g = *GImGui;
    EndPopup();
    g.BeginComboDepth--;
}

static const char* items_array_getter(void* data, int idx)
{
    const char* const* items = (const char* const*)data;
    return items[idx];
}

bool dropdown_ex(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, bool wo_label, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;

    // Call the getter to obtain the preview string which is a parameter to BeginCombo()
    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, calc_max_popup_height(popup_max_height_in_items)));

    if (!begin_dropdown(label, preview_value, wo_label, items_count, ImGuiComboFlags_None))
        return false;

    // Display items
    // FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        const char* item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";

        PushID(i);
        const bool item_selected = (i == *current_item);
        if (dd_selectable_ex(item_text, item_selected) && *current_item != i)
        {
            value_changed = true;
            *current_item = i;
        }
        if (item_selected)
            SetItemDefaultFocus();
        PopID();
    }

    end_dropdown();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool c_gui::dropdown(std::string_view label, int* current_item, const char* const items[], int items_count, bool wo_label, int height_in_items)
{
    const bool value_changed = dropdown_ex(label.data(), current_item, items_array_getter, (void*)items, items_count, wo_label, height_in_items);
    return value_changed;
}

void c_gui::multi_dropdown(std::string_view label, std::vector<int>& variable, const char* labels[], int count, bool wo_label)
{
    std::string preview = "-";

    for (auto i = 0, j = 0; i < count; i++)
    {
        if (variable[i])
        {
            if (j)
                preview += (", ") + (std::string)labels[i];
            else
                preview = labels[i];

            j++;
        }
    }

    if (begin_dropdown(label.data(), preview.c_str(), wo_label, count, 0))
    {
        for (auto i = 0; i < count; i++)
        {
            dd_selectable(labels[i], (bool*)&variable[i]);
        }
        end_dropdown();
    }

    preview = ("-");
}
