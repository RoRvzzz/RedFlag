#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../imgui/imgui.h"

class c_colors
{
public:
    ImColor accent{ 85, 120, 180, 255 };

    struct
    {
        ImColor background_one{ 15, 20, 40, 255 };
        ImColor background_two{ 25, 35, 60, 255 };
        ImColor stroke{ 40, 50, 70, 255 };
    } window;

    struct
    {
        ImColor stroke_two{ 30, 40, 65, 255 };
        ImColor text{ 200, 210, 220, 255 };
        ImColor text_inactive{ 140, 150, 160, 255 };
    } widgets;
};

inline c_colors* clr = new c_colors();