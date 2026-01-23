#pragma once
#include "../../imgui/imgui.h"

class c_colors
{
public:
	// Purple accent color (#656BB8)
	ImColor accent{ 101, 107, 184 };

	struct
	{
		// Nice black backgrounds (like the image - very dark/black)
		ImColor background_one{ 12, 12, 12 }; // Almost pure black
		ImColor background_two{ 16, 16, 16 }; // Slightly lighter black
		ImColor stroke{ 25, 25, 25 }; // Dark gray for borders
	} window;

	struct
	{
		ImColor stroke_two{ 20, 20, 20 }; // Very dark gray
		ImColor text{ 255, 255, 255 }; // Pure white text
		ImColor text_inactive{ 180, 180, 180 }; // Light gray for inactive text
	} widgets;
};

inline c_colors* clr = new c_colors();