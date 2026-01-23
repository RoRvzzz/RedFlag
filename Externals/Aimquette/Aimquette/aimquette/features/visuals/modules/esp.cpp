#include "../visuals.h"
#include "../../../drawing/imgui/imgui.h"
#include <Windows.h>
#include <cmath>
#include "../../wallcheck/wallcheck.h"
#include "crosshair.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>
#include <algorithm>
#include <vector>
#include <cmath>
#define min
#undef min
#define max
#undef max

ESPData visuals::espData;
std::thread visuals::updateThread;
bool visuals::threadRunning = false;


void draw_text_with_shadow_enhanced(float font_size, const ImVec2& position, ImColor color, const char* text);
void render_chams_enhanced(); // Forward declaration

inline float distance_sq(const Vector3& v1, const Vector3& v2) {
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    float dz = v1.z - v2.z;
    return dx * dx + dy * dy + dz * dz;
}

inline float distance_sq(const Vector2& v1, const Vector2& v2) {
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    return dx * dx + dy * dy;
}

float vector_length(const Vector3& vec) {
    return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

void DrawChinaHat(ImDrawList* draw, const Vector3& head_pos, ImColor color) {
    auto dimensions = globals::instances::visualengine.get_dimensions();
    if (dimensions.x == 0 || dimensions.y == 0) return;
    if (!draw) return;

    const float hatRadius = 1.5f;
    const float hatHeight = 1.2f;
    const int segments = 24;

    std::vector<Vector3> vertices;
    std::vector<Vector2> screenVertices;

    vertices.push_back(head_pos + Vector3(0, hatHeight, 0));

    for (int i = 0; i <= segments; i++) {
        float angle = (i * 2.0f * 3.14159f) / segments;
        vertices.push_back(head_pos + Vector3(
            hatRadius * cosf(angle),
            0,
            hatRadius * sinf(angle)
        ));
    }

    for (int ring = 1; ring <= 3; ring++) {
        float ringRadius = hatRadius * (ring / 4.0f);
        float ringHeight = hatHeight * (1.0f - ring / 4.0f);
        for (int i = 0; i <= segments; i++) {
            float angle = (i * 2.0f * 3.14159f) / segments;
            vertices.push_back(head_pos + Vector3(
                ringRadius * cosf(angle),
                ringHeight,
                ringRadius * sinf(angle)
            ));
        }
    }

    bool allVisible = true;
    for (const auto& vertex : vertices) {
        Vector2 screen = roblox::worldtoscreen(vertex);
        if (screen.x == -1 || screen.y == -1) {
            allVisible = false;
            break;
        }
        screenVertices.push_back(screen);
    }

    if (allVisible) {
        for (int i = 1; i <= segments; i++) {
            draw->AddLine(
                ImVec2(screenVertices[0].x, screenVertices[0].y),
                ImVec2(screenVertices[i].x, screenVertices[i].y),
                color, 0.8f
            );
        }

        for (int i = 1; i <= segments; i++) {
            int next = (i == segments) ? 1 : i + 1;
            draw->AddLine(
                ImVec2(screenVertices[i].x, screenVertices[i].y),
                ImVec2(screenVertices[next].x, screenVertices[next].y),
                color, 1.0f
            );
        }

        for (int ring = 1; ring <= 3; ring++) {
            int ringStart = 1 + ring * (segments + 1);
            for (int i = 0; i < segments; i++) {
                int curr = ringStart + i;
                int next = ringStart + ((i + 1) % (segments + 1));
                draw->AddLine(
                    ImVec2(screenVertices[curr].x, screenVertices[curr].y),
                    ImVec2(screenVertices[next].x, screenVertices[next].y),
                    ImColor(color.Value.x, color.Value.y, color.Value.z, 0.8f), 0.6f
                );
            }
        }

        for (int i = 0; i < segments; i += 2) {
            for (int ring = 0; ring < 3; ring++) {
                int curr = 1 + ring * (segments + 1) + i;
                int next = 1 + (ring + 1) * (segments + 1) + i;
                draw->AddLine(
                    ImVec2(screenVertices[curr].x, screenVertices[curr].y),
                    ImVec2(screenVertices[next].x, screenVertices[next].y),
                    ImColor(color.Value.x, color.Value.y, color.Value.z, 0.6f), 0.5f
                );
            }
        }
    }
}

void render_box_enhanced(const Vector2& top_left, const Vector2& bottom_right, ImColor color) {
    if (!globals::instances::draw) return;
    auto dimensions = globals::instances::visualengine.get_dimensions();
    if (dimensions.x == 0 || dimensions.y == 0) return;

    float width = bottom_right.x - top_left.x;
    float height = bottom_right.y - top_left.y;

    if (globals::visuals::boxtype == 0) {
        // Draw fill FIRST (behind everything)
        if ((*globals::visuals::box_overlay_flags)[2]) {
            if (globals::visuals::box_gradient) {
                // For full box type, use the same rotating gradient if enabled, otherwise a simple shading
                ImColor c1 = ImColor(globals::visuals::box_gradient_color1[0], globals::visuals::box_gradient_color1[1], globals::visuals::box_gradient_color1[2], globals::visuals::box_gradient_color1[3]);
                ImColor c2 = ImColor(globals::visuals::box_gradient_color2[0], globals::visuals::box_gradient_color2[1], globals::visuals::box_gradient_color2[2], globals::visuals::box_gradient_color2[3]);
                // Always rotate when gradient is enabled
                if (globals::visuals::box_gradient_rotation) {
                    static auto start_tp2 = std::chrono::steady_clock::now();
                    auto now_tp2 = std::chrono::steady_clock::now();
                    float t2 = std::chrono::duration_cast<std::chrono::duration<float>>(now_tp2 - start_tp2).count();
                    float angle2 = t2 * globals::visuals::box_gradient_rotation_speed * 6.28318530718f;
                    float cs2 = cosf(angle2), sn2 = sinf(angle2);
                    auto weight = [&](float cx, float cy){ return (cs2 * cx + sn2 * cy) * 0.5f + 0.5f; };
                    auto mix = [](const ImColor& a, const ImColor& b, float w){
                        float iw = 1.0f - w; ImVec4 av = ImGui::ColorConvertU32ToFloat4(a); ImVec4 bv = ImGui::ColorConvertU32ToFloat4(b);
                        return ImColor(ImVec4(av.x * iw + bv.x * w, av.y * iw + bv.y * w, av.z * iw + bv.z * w, av.w * iw + bv.w * w));
                    };
                    ImColor tl = mix(c1, c2, weight(-1, -1));
                    ImColor tr = mix(c1, c2, weight( 1, -1));
                    ImColor br = mix(c1, c2, weight( 1,  1));
                    ImColor bl = mix(c1, c2, weight(-1,  1));
                    globals::instances::draw->AddRectFilledMultiColor(ImVec2(top_left.x, top_left.y), ImVec2(bottom_right.x, bottom_right.y), tl, tr, br, bl);
                } else {
                    globals::instances::draw->AddRectFilledMultiColor(
                        ImVec2(top_left.x, top_left.y),
                        ImVec2(bottom_right.x, bottom_right.y),
                        c1, c2, c2, c1
                    );
                }
            } else {
                // Use solid color for the filled box
                ImColor boxfill = ImColor(globals::visuals::boxfillcolor[0], globals::visuals::boxfillcolor[1], globals::visuals::boxfillcolor[2], globals::visuals::boxfillcolor[3]);
                globals::instances::draw->AddRectFilled(
                    ImVec2(top_left.x, top_left.y),
                    ImVec2(bottom_right.x, bottom_right.y),
                    boxfill, 0.0f, 0
                );
            }
        }

        // Draw glow (if enabled)
        if ((*globals::visuals::box_overlay_flags)[1]) {
            globals::instances::draw->AddShadowRect(
                ImVec2(top_left.x, top_left.y),
                ImVec2(bottom_right.x, bottom_right.y),
                ImGui::GetColorU32(ImVec4(
                    globals::visuals::glowcolor[0],
                    globals::visuals::glowcolor[1],
                    globals::visuals::glowcolor[2],
                    globals::visuals::glowcolor[3]
                )), 15.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
            );
        }

        // Draw outline LAST (on top)
        // Pixel perfect: 1px outer outline + 1px color + 1px inner outline = 3px total
        ImColor outline_color = ImColor(0, 0, 0, 255);
        
        // Draw complete black border all around the box (like health bar)
        // Top border
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x - 1, top_left.y - 1),
            ImVec2(bottom_right.x + 1, top_left.y),
            outline_color
        );
        
        // Bottom border
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x - 1, bottom_right.y),
            ImVec2(bottom_right.x + 1, bottom_right.y + 1),
            outline_color
        );
        
        // Left border
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x - 1, top_left.y),
            ImVec2(top_left.x, bottom_right.y),
            outline_color
        );
        
        // Right border
        globals::instances::draw->AddRectFilled(
            ImVec2(bottom_right.x, top_left.y),
            ImVec2(bottom_right.x + 1, bottom_right.y),
            outline_color
        );

        // Color box (1px)
        globals::instances::draw->AddRect(
            ImVec2(top_left.x, top_left.y),
            ImVec2(bottom_right.x, bottom_right.y),
            color, 0.0f, 0, 1.0f
        );

        // Inner outline (1px)
        globals::instances::draw->AddRect(
            ImVec2(top_left.x + 1, top_left.y + 1),
            ImVec2(bottom_right.x - 1, bottom_right.y - 1),
            outline_color, 0.0f, 0, 1.0f
        );
    }
    else if (globals::visuals::boxtype == 1) {
        float corner_size = std::min(width * 1.5f, height) * 0.25f;

        if ((*globals::visuals::box_overlay_flags)[1]) {
            globals::instances::draw->AddShadowRect(
                ImVec2(top_left.x, top_left.y),
                ImVec2(bottom_right.x, bottom_right.y),
                ImGui::GetColorU32(ImVec4(
                    globals::visuals::glowcolor[0],
                    globals::visuals::glowcolor[1],
                    globals::visuals::glowcolor[2],
                    globals::visuals::glowcolor[3]
                )), 15.f, ImVec2(0, 0),
                ImDrawFlags_ShadowCutOutShapeBackground, 0
            );
        }

        if ((*globals::visuals::box_overlay_flags)[2]) {
            if (globals::visuals::box_gradient) {
                ImColor c1 = ImColor(globals::visuals::box_gradient_color1[0], globals::visuals::box_gradient_color1[1], globals::visuals::box_gradient_color1[2], globals::visuals::box_gradient_color1[3]);
                ImColor c2 = ImColor(globals::visuals::box_gradient_color2[0], globals::visuals::box_gradient_color2[1], globals::visuals::box_gradient_color2[2], globals::visuals::box_gradient_color2[3]);
                if (globals::visuals::box_gradient_rotation) {
                    static auto start_tp3 = std::chrono::steady_clock::now();
                    auto now_tp3 = std::chrono::steady_clock::now();
                    float t3 = std::chrono::duration_cast<std::chrono::duration<float>>(now_tp3 - start_tp3).count();
                    float ang3 = t3 * globals::visuals::box_gradient_rotation_speed * 6.28318530718f;
                    float cs3 = cosf(ang3), sn3 = sinf(ang3);
                    auto weight = [&](float cx, float cy){ return (cs3 * cx + sn3 * cy) * 0.5f + 0.5f; };
                    auto mix = [](const ImColor& a, const ImColor& b, float w){
                        float iw = 1.0f - w; ImVec4 av = ImGui::ColorConvertU32ToFloat4(a); ImVec4 bv = ImGui::ColorConvertU32ToFloat4(b);
                        return ImColor(ImVec4(av.x * iw + bv.x * w, av.y * iw + bv.y * w, av.z * iw + bv.z * w, av.w * iw + bv.w * w));
                    };
                    ImColor tl = mix(c1, c2, weight(-1, -1));
                    ImColor tr = mix(c1, c2, weight( 1, -1));
                    ImColor br = mix(c1, c2, weight( 1,  1));
                    ImColor bl = mix(c1, c2, weight(-1,  1));
                    globals::instances::draw->AddRectFilledMultiColor(ImVec2(top_left.x, top_left.y), ImVec2(bottom_right.x, bottom_right.y), tl, tr, br, bl);
                } else {
                    globals::instances::draw->AddRectFilledMultiColor(
                        ImVec2(top_left.x, top_left.y),
                        ImVec2(bottom_right.x, bottom_right.y),
                        c1, c2, c2, c1
                    );
                }
            } else {
                // Use solid color for the filled box
                ImColor boxfill = ImColor(
                    globals::visuals::boxfillcolor[0],
                    globals::visuals::boxfillcolor[1],
                    globals::visuals::boxfillcolor[2],
                    globals::visuals::boxfillcolor[3]
                );
                globals::instances::draw->AddRectFilled(
                    ImVec2(top_left.x, top_left.y),
                    ImVec2(bottom_right.x, bottom_right.y),
                    boxfill, 0.0f, 0
                );
            }
        }

        // Pixel perfect: 1px outer outline + 1px color + 1px inner outline = 3px total
        ImColor outline_color = ImColor(0, 0, 0, 255);
        
        // Top-left corner - Outer outline (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x - 1, top_left.y - 1), ImVec2(top_left.x + corner_size - 1, top_left.y - 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x - 1, top_left.y - 1), ImVec2(top_left.x - 1, top_left.y + corner_size - 1), outline_color, 1.0f);
        // Top-left corner - Color (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x, top_left.y), ImVec2(top_left.x + corner_size, top_left.y), color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x, top_left.y), ImVec2(top_left.x, top_left.y + corner_size), color, 1.0f);
        // Top-left corner - Inner outline (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x + 1, top_left.y + 1), ImVec2(top_left.x + corner_size + 1, top_left.y + 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x + 1, top_left.y + 1), ImVec2(top_left.x + 1, top_left.y + corner_size + 1), outline_color, 1.0f);

        // Top-right corner - Outer outline (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x + 1, top_left.y - 1), ImVec2(bottom_right.x - corner_size + 1, top_left.y - 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x + 1, top_left.y - 1), ImVec2(bottom_right.x + 1, top_left.y + corner_size - 1), outline_color, 1.0f);
        // Top-right corner - Color (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x, top_left.y), ImVec2(bottom_right.x - corner_size, top_left.y), color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x, top_left.y), ImVec2(bottom_right.x, top_left.y + corner_size), color, 1.0f);
        // Top-right corner - Inner outline (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x - 1, top_left.y + 1), ImVec2(bottom_right.x - corner_size - 1, top_left.y + 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x - 1, top_left.y + 1), ImVec2(bottom_right.x - 1, top_left.y + corner_size + 1), outline_color, 1.0f);

        // Bottom-left corner - Outer outline (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x - 1, bottom_right.y + 1), ImVec2(top_left.x + corner_size - 1, bottom_right.y + 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x - 1, bottom_right.y + 1), ImVec2(top_left.x - 1, bottom_right.y - corner_size + 1), outline_color, 1.0f);
        // Bottom-left corner - Color (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x, bottom_right.y), ImVec2(top_left.x + corner_size, bottom_right.y), color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x, bottom_right.y), ImVec2(top_left.x, bottom_right.y - corner_size), color, 1.0f);
        // Bottom-left corner - Inner outline (1px)
        globals::instances::draw->AddLine(ImVec2(top_left.x + 1, bottom_right.y - 1), ImVec2(top_left.x + corner_size + 1, bottom_right.y - 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(top_left.x + 1, bottom_right.y - 1), ImVec2(top_left.x + 1, bottom_right.y - corner_size - 1), outline_color, 1.0f);

        // Bottom-right corner - Outer outline (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x + 1, bottom_right.y + 1), ImVec2(bottom_right.x - corner_size + 1, bottom_right.y + 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x + 1, bottom_right.y + 1), ImVec2(bottom_right.x + 1, bottom_right.y - corner_size + 1), outline_color, 1.0f);
        // Bottom-right corner - Color (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x, bottom_right.y), ImVec2(bottom_right.x - corner_size, bottom_right.y), color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x, bottom_right.y), ImVec2(bottom_right.x, bottom_right.y - corner_size), color, 1.0f);
        // Bottom-right corner - Inner outline (1px)
        globals::instances::draw->AddLine(ImVec2(bottom_right.x - 1, bottom_right.y - 1), ImVec2(bottom_right.x - corner_size - 1, bottom_right.y - 1), outline_color, 1.0f);
        globals::instances::draw->AddLine(ImVec2(bottom_right.x - 1, bottom_right.y - 1), ImVec2(bottom_right.x - 1, bottom_right.y - corner_size - 1), outline_color, 1.0f);
        
        // Draw complete black border all around the box (like health bar)
        // Top border - connects between corners
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x + corner_size, top_left.y - 1),
            ImVec2(bottom_right.x - corner_size, top_left.y),
            outline_color
        );
        
        // Bottom border - connects between corners
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x + corner_size, bottom_right.y),
            ImVec2(bottom_right.x - corner_size, bottom_right.y + 1),
            outline_color
        );
        
        // Left border - connects between corners
        globals::instances::draw->AddRectFilled(
            ImVec2(top_left.x - 1, top_left.y + corner_size),
            ImVec2(top_left.x, bottom_right.y - corner_size),
            outline_color
        );
        
        // Right border - connects between corners
        globals::instances::draw->AddRectFilled(
            ImVec2(bottom_right.x, top_left.y + corner_size),
            ImVec2(bottom_right.x + 1, bottom_right.y - corner_size),
            outline_color
        );
    }
}

void render_health_bar_enhanced(const Vector2& top_left, const Vector2& bottom_right, float health_percent, ImColor color) {
    if (!globals::instances::draw) return;
    if (!std::isfinite(health_percent)) health_percent = 0.0f;
    health_percent = std::clamp(health_percent, 0.0f, 1.0f);
    if (!globals::visuals::healthbar) return;

    float box_width = bottom_right.x - top_left.x;
    float box_height = bottom_right.y - top_left.y;

    // Pixel perfect: 1px outer outline + 1px color + 1px inner outline = 3px total width
    float bar_width = 1.f;
    float bar_height = box_height;
    float bar_x = top_left.x - 3.0f - 4.0f; // Adjust base position to account for 3px total width
    
    ImColor outline_color = ImColor(0, 0, 0, 255);
    
    // Always draw black border all around - pixel perfect 3px total width
    // Draw left and right borders first
    
    // Left border (1px) - outer outline - full height including top and bottom
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, top_left.y - 1),
        ImVec2(bar_x + 1, bottom_right.y + 1),
        outline_color
    );
    
    // Right border (1px) - inner outline - full height including top and bottom
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x + 2, top_left.y - 1),
        ImVec2(bar_x + 3, bottom_right.y + 1),
        outline_color
    );

    if ((*globals::visuals::healthbar_overlay_flags)[2]) {
        globals::instances::draw->AddShadowRect(
            ImVec2(bar_x + 1, top_left.y),
            ImVec2(bar_x + 2, bottom_right.y),
            ImGui::GetColorU32(ImVec4(
                globals::visuals::healthbarcolor[0],
                globals::visuals::healthbarcolor[1],
                globals::visuals::healthbarcolor[2],
                globals::visuals::healthbarcolor[3]
            )), 5.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
        );
    }

    ImColor background_color = ImColor(45, 45, 45, 220);
    // Background bar (1px) - middle pixel
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x + 1, top_left.y),
        ImVec2(bar_x + 2, bottom_right.y),
        background_color
    );

    color = ImColor(
        globals::visuals::healthbarcolor[0],
        globals::visuals::healthbarcolor[1],
        globals::visuals::healthbarcolor[2],
        globals::visuals::healthbarcolor[3]
    );
    auto color1 = ImColor(
        globals::visuals::healthbarcolor1[0],
        globals::visuals::healthbarcolor1[1],
        globals::visuals::healthbarcolor1[2],
        globals::visuals::healthbarcolor1[3]
    );
    float fill_height = bar_height * health_percent;

    if ((*globals::visuals::healthbar_overlay_flags)[1]) {
        globals::instances::draw->AddRectFilledMultiColor(
            ImVec2(bar_x + 1, bottom_right.y - fill_height),
            ImVec2(bar_x + 2, bottom_right.y),
            color, color, color1, color1
        );
    }
    else {
        globals::instances::draw->AddRectFilled(
            ImVec2(bar_x + 1, std::floor(bottom_right.y - fill_height)),
            ImVec2(bar_x + 2, std::ceil(bottom_right.y)),
            color
        );
    }
    
    // Top border (3px wide) - drawn LAST to ensure it's fully visible and not covered
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, top_left.y - 1),
        ImVec2(bar_x + 3, top_left.y),
        outline_color
    );
    
    // Bottom border (3px wide) - drawn LAST to ensure it's fully visible and not covered
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, bottom_right.y),
        ImVec2(bar_x + 3, bottom_right.y + 1),
        outline_color
    );

    if (globals::visuals::healthtext) {
        int health_value = static_cast<int>(std::round(health_percent * 100.0f));
        char buf[8];
        sprintf_s(buf, "%d", health_value);
        // Position health text to the left of the health bar, at the top
        ImVec2 text_pos = ImVec2(bar_x - 16.0f, top_left.y - 2.0f);

        ImColor esp_text_col = ImColor(
            globals::visuals::namecolor[0],
            globals::visuals::namecolor[1],
            globals::visuals::namecolor[2],
            globals::visuals::namecolor[3]
        );
        ::draw_text_with_shadow_enhanced(11.0f, text_pos, esp_text_col, buf);
    }
}

static void render_armor_bar_enhanced(const Vector2& top_left, const Vector2& bottom_right, float armor_percent) {
    if (!globals::instances::draw) return;
    if (!std::isfinite(armor_percent)) armor_percent = 0.0f;
    armor_percent = std::clamp(armor_percent, 0.0f, 1.0f);
    if (armor_percent <= 0.0f) return;

    float box_width = bottom_right.x - top_left.x;
    float box_height = bottom_right.y - top_left.y;

    // Pixel perfect: 1px outer outline + 1px color + 1px inner outline = 3px total width
    float bar_width = 1.f;
    float bar_height = box_height;
    // Place armor bar to the left of the health bar (which is at -3.0f - 4.0f = -7.0f)
    // Position it further left to account for 3px width + spacing
    float bar_x = top_left.x - 3.0f - 4.0f - 4.0f; // 4px spacing from health bar
    
    ImColor outline_color = ImColor(0, 0, 0, 255);
    
    // Always draw black border all around - pixel perfect 3px total width
    // Draw left and right borders first
    
    // Left border (1px) - outer outline - full height including top and bottom
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, top_left.y - 1),
        ImVec2(bar_x + 1, bottom_right.y + 1),
        outline_color
    );
    
    // Right border (1px) - inner outline - full height including top and bottom
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x + 2, top_left.y - 1),
        ImVec2(bar_x + 3, bottom_right.y + 1),
        outline_color
    );

    if ((*globals::visuals::healthbar_overlay_flags)[2]) {
        globals::instances::draw->AddShadowRect(
            ImVec2(bar_x + 1, top_left.y),
            ImVec2(bar_x + 2, bottom_right.y),
            ImGui::GetColorU32(ImVec4(100.0f / 255.0f, 150.0f / 255.0f, 255.0f / 255.0f, 1.0f)),
            5.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
        );
    }

    ImColor background_color = ImColor(45, 45, 45, 220);
    // Background bar (1px) - middle pixel
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x + 1, top_left.y),
        ImVec2(bar_x + 2, bottom_right.y),
        background_color
    );

    // Nice blue color for armor
    ImColor color = ImColor(100, 150, 255, 255);
    // Slightly darker blue for gradient
    auto color1 = ImColor(70, 120, 255, 255);
    float fill_height = bar_height * armor_percent;

    if ((*globals::visuals::healthbar_overlay_flags)[1]) {
        globals::instances::draw->AddRectFilledMultiColor(
            ImVec2(bar_x + 1, bottom_right.y - fill_height),
            ImVec2(bar_x + 2, bottom_right.y),
            color, color, color1, color1
        );
    }
    else {
        globals::instances::draw->AddRectFilled(
            ImVec2(bar_x + 1, std::floor(bottom_right.y - fill_height)),
            ImVec2(bar_x + 2, std::ceil(bottom_right.y)),
            color
        );
    }
    
    // Top border (3px wide) - drawn LAST to ensure it's fully visible and not covered
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, top_left.y - 1),
        ImVec2(bar_x + 3, top_left.y),
        outline_color
    );
    
    // Bottom border (3px wide) - drawn LAST to ensure it's fully visible and not covered
    globals::instances::draw->AddRectFilled(
        ImVec2(bar_x, bottom_right.y),
        ImVec2(bar_x + 3, bottom_right.y + 1),
        outline_color
    );
}

void render_name_enhanced(const std::string& name, const Vector2& top_left, const Vector2& bottom_right) {
    if (!globals::visuals::name || name.empty()) return;

    float center_x = (top_left.x + bottom_right.x) * 0.5f;
    ImFont* font = nullptr;
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts && io.Fonts->Fonts.Size > 0) {
        if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
    }
    float font_size = 13.0f;
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, name.c_str()) : ImGui::CalcTextSize(name.c_str());
    float y_pos = top_left.y - (text_size.y + 2.0f);
    float x_pos = center_x - text_size.x * 0.5f;
    x_pos = std::floor(x_pos + 0.5f);
    y_pos = std::floor(y_pos + 0.5f);

    ImVec2 text_pos = ImVec2(x_pos, y_pos);
    ImColor name_color = ImColor(
        globals::visuals::namecolor[0],
        globals::visuals::namecolor[1],
        globals::visuals::namecolor[2],
        globals::visuals::namecolor[3]
    );

    // draw_text_with_shadow_enhanced already handles pixel-perfect 1px outline in all 8 directions
    if ((*globals::visuals::name_overlay_flags)[1]) {
        ImColor glow_color = name_color;
        glow_color.Value.w *= 0.5f;
        ::draw_text_with_shadow_enhanced(font_size, text_pos, glow_color, name.c_str());
        ::draw_text_with_shadow_enhanced(font_size, text_pos, glow_color, name.c_str());
    }

    ::draw_text_with_shadow_enhanced(font_size, text_pos, name_color, name.c_str());
}

void render_team_name_enhanced(const roblox::player& player, const Vector2& top_left, const Vector2& bottom_right) {
    if (!globals::visuals::teams || !is_valid_address(player.team.address)) return;

    std::string team_name = player.team.get_name();
    if (team_name.empty()) return;

    float center_x = (top_left.x + bottom_right.x) * 0.5f;
    float y_pos = top_left.y - 30.0f; // Position above the name
    
    ImFont* font = nullptr;
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts && io.Fonts->Fonts.Size > 0) {
        if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
    }
    float font_size = 11.0f;
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, team_name.c_str()) : ImGui::CalcTextSize(team_name.c_str());

    float x_pos = center_x - text_size.x * 0.5f;
    x_pos = std::floor(x_pos + 0.5f);
    y_pos = std::floor(y_pos + 0.5f);

    ImVec2 text_pos = ImVec2(x_pos, y_pos);
    ImColor team_color = ImColor(
        globals::visuals::teamscolor[0],
        globals::visuals::teamscolor[1],
        globals::visuals::teamscolor[2],
        globals::visuals::teamscolor[3]
    );

    // draw_text_with_shadow_enhanced already handles pixel-perfect 1px outline in all 8 directions
    if ((*globals::visuals::team_overlay_flags)[1]) {
        ImColor glow_color = team_color;
        glow_color.Value.w *= 0.5f;
        ::draw_text_with_shadow_enhanced(font_size, text_pos, glow_color, team_name.c_str());
        ::draw_text_with_shadow_enhanced(font_size, text_pos, glow_color, team_name.c_str());
    }

    ::draw_text_with_shadow_enhanced(font_size, text_pos, team_color, team_name.c_str());
}

void render_distance_enhanced(float distance, const Vector2& top_left, const Vector2& bottom_right, bool has_tool) {
    if (!globals::visuals::distance) return;

    char distance_str[16];
    sprintf_s(distance_str, "%.0fm", distance);

    float center_x = (top_left.x + bottom_right.x) * 0.5f;
    float offset = bottom_right.y + (globals::visuals::toolesp ? 18.0f : 4.0f);
    
    ImFont* font = nullptr;
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts && io.Fonts->Fonts.Size > 0) {
        if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
    }
    float font_size = 13.0f;
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, distance_str) : ImGui::CalcTextSize(distance_str);

    float x_pos = center_x - text_size.x * 0.5f;
    x_pos = std::floor(x_pos + 0.5f);
    float y_pos = std::floor(offset + 0.5f);

    ImVec2 text_pos = ImVec2(x_pos, y_pos);
    ImColor distance_color = ImColor(
        globals::visuals::distancecolor[0],
        globals::visuals::distancecolor[1],
        globals::visuals::distancecolor[2],
        globals::visuals::distancecolor[3]
    );

    ::draw_text_with_shadow_enhanced(font_size, text_pos, distance_color, distance_str);
}

void render_tool_name_enhanced(const std::string& tool_name, const Vector2& top_left, const Vector2& bottom_right) {
    if (!globals::visuals::toolesp || tool_name.empty()) return;

    float center_x = (top_left.x + bottom_right.x) * 0.5f;
    float y_pos = bottom_right.y + 2.0f;
    
    ImFont* font = nullptr;
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts && io.Fonts->Fonts.Size > 0) {
        if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
    }
    float font_size = 13.0f;
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, tool_name.c_str()) : ImGui::CalcTextSize(tool_name.c_str());

    float x_pos = center_x - text_size.x * 0.5f;
    x_pos = std::floor(x_pos + 0.5f);
    y_pos = std::floor(y_pos + 0.5f);

    ImVec2 text_pos = ImVec2(x_pos, y_pos);
    ImColor tool_color = ImColor(
        globals::visuals::toolespcolor[0],
        globals::visuals::toolespcolor[1],
        globals::visuals::toolespcolor[2],
        globals::visuals::toolespcolor[3]
    );

    ::draw_text_with_shadow_enhanced(font_size, text_pos, tool_color, tool_name.c_str());
}

void render_snapline_enhanced(const Vector2& screen_pos) {
    if (!globals::visuals::tracers) return;

    ImVec2 origin;
    
    if (globals::visuals::tracerstype == 0) {
        // Torso type: Use local player's torso position
        if (is_valid_address(globals::instances::lp.main.address)) {
            Vector3 torsoPos = globals::instances::lp.uppertorso.get_pos();
            Vector2 torsoScreen = roblox::worldtoscreen(torsoPos);
            if (torsoScreen.x != -1.0f && torsoScreen.y != -1.0f) {
                origin = ImVec2(torsoScreen.x, torsoScreen.y);
            } else {
                // Fallback to screen center if torso not visible
                ImVec2 screen_size = ImGui::GetIO().DisplaySize;
                origin = ImVec2(screen_size.x * 0.5f, screen_size.y);
            }
        } else {
            // Fallback to screen center if local player not available
            ImVec2 screen_size = ImGui::GetIO().DisplaySize;
            origin = ImVec2(screen_size.x * 0.5f, screen_size.y);
        }
    } else {
        // Normal and Spiderweb types: Use cursor position (original behavior)
        ImVec2 screen_size = ImGui::GetIO().DisplaySize;
        origin = ImVec2(screen_size.x * 0.5f, screen_size.y);
        {
            HWND rw = FindWindowA(nullptr, "Roblox");
            POINT pt;
            if (rw && GetCursorPos(&pt) && ScreenToClient(rw, &pt)) {
                origin = ImVec2((float)pt.x, (float)pt.y);
            }
        }
    }
    ImColor col = ImColor(
        globals::visuals::tracerscolor[0],
        globals::visuals::tracerscolor[1],
        globals::visuals::tracerscolor[2],
        globals::visuals::tracerscolor[3]
    );
    col.Value.w = 0.40f;
    if (globals::visuals::tracerstype == 0 || globals::visuals::tracerstype == 1) {
        // Torso type (0) and Normal type (1) both use straight lines
        globals::instances::draw->AddLine(origin,
            ImVec2(screen_pos.x, screen_pos.y),
            col,
            1.5f
        );
        if (globals::visuals::tracers_glow) {
            ImColor glow = col; glow.Value.w *= 0.35f;
            globals::instances::draw->AddLine(origin, ImVec2(screen_pos.x, screen_pos.y), glow, 5.0f);
        }
    } else if (globals::visuals::tracerstype == 2) {
        ImVec2 target(screen_pos.x, screen_pos.y);
        float dx = fabsf(target.x - origin.x);
        float dy = fabsf(target.y - origin.y);
        float dip = 120.0f + 0.4f * dx + 0.25f * dy;
        ImVec2 ctrl((origin.x + target.x) * 0.5f, (origin.y + target.y) * 0.5f + dip);

        const int segments = 32;
        ImVec2 prev = origin;
        for (int i = 1; i <= segments; ++i) {
            float t = (float)i / (float)segments;
            float it = 1.0f - t;
            ImVec2 pt(
                it * it * origin.x + 2 * it * t * ctrl.x + t * t * target.x,
                it * it * origin.y + 2 * it * t * ctrl.y + t * t * target.y
            );
            if (globals::visuals::tracers_glow) {
                ImColor glow = col; glow.Value.w *= 0.35f;
                globals::instances::draw->AddLine(prev, pt, glow, 5.0f);
            }
            globals::instances::draw->AddLine(prev, pt, col, 1.5f);
            prev = pt;
        }
    }
}

void render_trail_enhanced(CachedPlayerData& playerData) {
    if (!globals::visuals::trail || !playerData.valid) return;
    
    auto current_time = std::chrono::steady_clock::now();
    
    // Remove old trail points that exceed the duration
    playerData.trail_points.erase(
        std::remove_if(playerData.trail_points.begin(), playerData.trail_points.end(),
            [&](const TrailPoint& point) {
                auto point_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - point.timestamp).count() / 1000.0f;
                return point_duration > globals::visuals::trail_duration;
            }),
        playerData.trail_points.end()
    );
    
    if (playerData.trail_points.size() < 2) return;
    
    // Convert trail points to screen coordinates
    std::vector<ImVec2> screen_points;
    for (const auto& point : playerData.trail_points) {
        Vector2 screen_pos = roblox::worldtoscreen(point.position);
        if (screen_pos.x != -1.0f && screen_pos.y != -1.0f) {
            screen_points.push_back(ImVec2(screen_pos.x, screen_pos.y));
        }
    }
    
    if (screen_points.size() < 2) return;
    
    // Render trail lines with fading alpha
    for (size_t i = 0; i < screen_points.size() - 1; i++) {
        auto point_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - playerData.trail_points[i].timestamp).count() / 1000.0f;
        float alpha = 1.0f - (point_duration / globals::visuals::trail_duration);
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        
        ImColor trail_color = ImColor(
            globals::visuals::trail_color[0],
            globals::visuals::trail_color[1],
            globals::visuals::trail_color[2],
            globals::visuals::trail_color[3] * alpha
        );
        
        globals::instances::draw->AddLine(
            screen_points[i],
            screen_points[i + 1],
            trail_color,
            globals::visuals::trail_thickness
        );
    }
}



static std::vector<Vector2> cached_skeleton_points;
static std::chrono::steady_clock::time_point last_skeleton_update;

void render_skeleton_enhanced(const CachedPlayerData& playerData, ImColor color) {
    if (!globals::visuals::skeletons || !playerData.valid || playerData.distance > 500.0f) return;

    roblox::player player = playerData.player;

    if (!is_valid_address(player.head.address) ||
        (!is_valid_address(player.lefthand.address) && !is_valid_address(player.leftupperarm.address)) ||
        (!is_valid_address(player.righthand.address) && !is_valid_address(player.rightupperarm.address)) ||
        (!is_valid_address(player.leftfoot.address) && !is_valid_address(player.leftupperleg.address)) ||
        (!is_valid_address(player.rightfoot.address) && !is_valid_address(player.rightupperleg.address)))
        return;

    auto dimensions = globals::instances::visualengine.get_dimensions();
    if (dimensions.x == 0 || dimensions.y == 0) return;

    auto on_screen = [&dimensions](const Vector2& pos) {
        return pos.x > 0 && pos.x < dimensions.x && pos.y > 0 && pos.y < dimensions.y;
        };

    auto draw_skel_line = [&](const ImVec2& p1, const ImVec2& p2, ImColor baseColor) {
        if ((*globals::visuals::skeleton_overlay_flags)[1]) {
            ImColor glowColor = baseColor; glowColor.Value.w *= 0.35f;
            globals::instances::draw->AddLine(p1, p2, glowColor, 4.5f);
            globals::instances::draw->AddLine(p1, p2, glowColor, 3.0f);
        }
        if ((*globals::visuals::skeleton_overlay_flags)[0]) {
            ImColor outlineColor = ImColor(0, 0, 0, 200);
            globals::instances::draw->AddLine(p1, p2, outlineColor, 2.0f);
        }
        globals::instances::draw->AddLine(p1, p2, baseColor, 1.0f);
    };

    if (player.rigtype == 0) {
        Vector2 head = roblox::worldtoscreen(player.head.get_pos());
        Vector2 torso = roblox::worldtoscreen(player.uppertorso.get_pos());
        Vector2 left_arm = roblox::worldtoscreen(player.lefthand.get_pos());
        Vector2 right_arm = roblox::worldtoscreen(player.righthand.get_pos());
        Vector2 left_leg = roblox::worldtoscreen(player.leftfoot.get_pos());
        Vector2 right_leg = roblox::worldtoscreen(player.rightfoot.get_pos());

        if (on_screen(head) && on_screen(torso)) draw_skel_line(ImVec2(head.x, head.y), ImVec2(torso.x, torso.y), color);
        if (on_screen(torso) && on_screen(left_arm)) draw_skel_line(ImVec2(torso.x, torso.y), ImVec2(left_arm.x, left_arm.y), color);
        if (on_screen(torso) && on_screen(right_arm)) draw_skel_line(ImVec2(torso.x, torso.y), ImVec2(right_arm.x, right_arm.y), color);
        if (on_screen(torso) && on_screen(left_leg)) draw_skel_line(ImVec2(torso.x, torso.y), ImVec2(left_leg.x, left_leg.y), color);
        if (on_screen(torso) && on_screen(right_leg)) draw_skel_line(ImVec2(torso.x, torso.y), ImVec2(right_leg.x, right_leg.y), color);
    }
    else {
        if (!is_valid_address(player.leftupperarm.address) ||
            !is_valid_address(player.rightupperarm.address) ||
            !is_valid_address(player.leftlowerarm.address) ||
            !is_valid_address(player.rightlowerarm.address) ||
            !is_valid_address(player.leftupperleg.address) ||
            !is_valid_address(player.rightupperleg.address) ||
            !is_valid_address(player.leftlowerleg.address) ||
            !is_valid_address(player.rightlowerleg.address))
            return;

        Vector2 head = roblox::worldtoscreen(player.head.get_pos());
        Vector2 upper_torso = roblox::worldtoscreen(player.uppertorso.get_pos());
        Vector2 lower_torso = roblox::worldtoscreen(player.lowertorso.get_pos());
        Vector2 left_upper_arm = roblox::worldtoscreen(player.leftupperarm.get_pos());
        Vector2 right_upper_arm = roblox::worldtoscreen(player.rightupperarm.get_pos());
        Vector2 left_lower_arm = roblox::worldtoscreen(player.leftlowerarm.get_pos());
        Vector2 right_lower_arm = roblox::worldtoscreen(player.rightlowerarm.get_pos());
        Vector2 left_hand = roblox::worldtoscreen(player.lefthand.get_pos());
        Vector2 right_hand = roblox::worldtoscreen(player.righthand.get_pos());
        Vector2 left_upper_leg = roblox::worldtoscreen(player.leftupperleg.get_pos());
        Vector2 right_upper_leg = roblox::worldtoscreen(player.rightupperleg.get_pos());
        Vector2 left_lower_leg = roblox::worldtoscreen(player.leftlowerleg.get_pos());
        Vector2 right_lower_leg = roblox::worldtoscreen(player.rightlowerleg.get_pos());
        Vector2 left_foot = roblox::worldtoscreen(player.leftfoot.get_pos());
        Vector2 right_foot = roblox::worldtoscreen(player.rightfoot.get_pos());

        if (on_screen(head) && on_screen(upper_torso)) { head.y -= 1.5f; draw_skel_line(ImVec2(head.x, head.y), ImVec2(upper_torso.x, upper_torso.y), color); }
        if (on_screen(upper_torso) && on_screen(lower_torso)) { lower_torso.y += 1.5f; draw_skel_line(ImVec2(upper_torso.x, upper_torso.y), ImVec2(lower_torso.x, lower_torso.y), color); }
        if (on_screen(upper_torso) && on_screen(left_upper_arm)) draw_skel_line(ImVec2(upper_torso.x, upper_torso.y), ImVec2(left_upper_arm.x, left_upper_arm.y), color);
        if (on_screen(upper_torso) && on_screen(right_upper_arm)) draw_skel_line(ImVec2(upper_torso.x, upper_torso.y), ImVec2(right_upper_arm.x, right_upper_arm.y), color);
        if (on_screen(left_upper_arm) && on_screen(left_lower_arm)) draw_skel_line(ImVec2(left_upper_arm.x, left_upper_arm.y), ImVec2(left_lower_arm.x, left_lower_arm.y), color);
        if (on_screen(right_upper_arm) && on_screen(right_lower_arm)) draw_skel_line(ImVec2(right_upper_arm.x, right_upper_arm.y), ImVec2(right_lower_arm.x, right_lower_arm.y), color);
        if (on_screen(left_lower_arm) && on_screen(left_hand)) draw_skel_line(ImVec2(left_lower_arm.x, left_lower_arm.y), ImVec2(left_hand.x, left_hand.y), color);
        if (on_screen(right_lower_arm) && on_screen(right_hand)) draw_skel_line(ImVec2(right_lower_arm.x, right_lower_arm.y), ImVec2(right_hand.x, right_hand.y), color);
        if (on_screen(lower_torso) && on_screen(left_upper_leg)) draw_skel_line(ImVec2(lower_torso.x, lower_torso.y), ImVec2(left_upper_leg.x, left_upper_leg.y), color);
        if (on_screen(lower_torso) && on_screen(right_upper_leg)) draw_skel_line(ImVec2(lower_torso.x, lower_torso.y), ImVec2(right_upper_leg.x, right_upper_leg.y), color);
        if (on_screen(left_upper_leg) && on_screen(left_lower_leg)) draw_skel_line(ImVec2(left_upper_leg.x, left_upper_leg.y), ImVec2(left_lower_leg.x, left_lower_leg.y), color);
        if (on_screen(right_upper_leg) && on_screen(right_lower_leg)) draw_skel_line(ImVec2(right_upper_leg.x, right_upper_leg.y), ImVec2(right_lower_leg.x, right_lower_leg.y), color);
        if (on_screen(left_lower_leg) && on_screen(left_foot)) draw_skel_line(ImVec2(left_lower_leg.x, left_lower_leg.y), ImVec2(left_foot.x, left_foot.y), color);
        if (on_screen(right_lower_leg) && on_screen(right_foot)) draw_skel_line(ImVec2(right_lower_leg.x, right_lower_leg.y), ImVec2(right_foot.x, right_foot.y), color);
    }
}


void draw_text_with_shadow_enhanced(float font_size, const ImVec2& position, ImColor color, const char* text) {
    if (!globals::instances::draw || !text) return;
    ImFont* font = nullptr;
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts && io.Fonts->Fonts.Size > 0) {
        if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
    }
    if (font) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                globals::instances::draw->AddText(font, font_size, ImVec2(position.x + i, position.y + j), ImColor(0, 0, 0, 255), text);
            }
        }
        globals::instances::draw->AddText(font, font_size, position, color, text);
    }
    else {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                globals::instances::draw->AddText(0, font_size, ImVec2(position.x + i, position.y + j), ImColor(0, 0, 0, 255), text);
            }
        }
        globals::instances::draw->AddText(0, font_size, position, color, text);
    }
}

// OOF arrows removed
void render_offscreen_arrow_enhanced(const Vector3& player_pos, const Vector3& local_pos, ImColor color, const std::string& player_name = "") {
    return;

    auto dimensions = globals::instances::visualengine.get_dimensions();
    if (dimensions.x == 0 || dimensions.y == 0) return;

    ImVec2 screen_center(dimensions.x / 2.f, dimensions.y / 2.f);
    auto view_matrix = globals::instances::visualengine.GetViewMatrix();
    auto camerarotation = globals::instances::camera.getRot();

    Vector3 camera_forward = camerarotation.getColumn(2);
    camera_forward = camera_forward * -1.0f;

    Vector3 dir_to_player = player_pos - local_pos;
    float distance = vector_length(dir_to_player * 0.2);
    if (distance < 1.0f) return;

    dir_to_player.x /= distance;
    dir_to_player.y /= distance;
    dir_to_player.z /= distance;

    Vector3 camera_right = camerarotation.getColumn(0);
    Vector3 camera_up = camerarotation.getColumn(1);

    float forward_dot = dir_to_player.dot(camera_forward);
    if (forward_dot > 0.98f) return;

    float right_dot = dir_to_player.dot(camera_right);
    float up_dot = dir_to_player.dot(camera_up);

    ImVec2 screen_dir(right_dot, -up_dot);
    float screen_dir_len = sqrtf(screen_dir.x * screen_dir.x + screen_dir.y * screen_dir.y);
    if (screen_dir_len < 0.001f) return;

    screen_dir.x /= screen_dir_len;
    screen_dir.y /= screen_dir_len;

    const float radius = 150.0f;
    ImVec2 pos(
        screen_center.x + screen_dir.x * radius,
        screen_center.y + screen_dir.y * radius
    );

    float angle = atan2f(screen_dir.y, screen_dir.x);
    const float arrow_size = 10.0f;
    const float arrow_angle = 0.6f;

    ImVec2 point1(
        pos.x - arrow_size * cosf(angle - arrow_angle),
        pos.y - arrow_size * sinf(angle - arrow_angle)
    );
    ImVec2 point2(
        pos.x - arrow_size * cosf(angle + arrow_angle),
        pos.y - arrow_size * sinf(angle + arrow_angle)
    );

    ImColor arrow_color = ImColor(255,255,255,255);



    globals::instances::draw->AddTriangleFilled(pos, point1, point2, arrow_color);
    globals::instances::draw->AddTriangle(pos, point1, point2, ImColor(0, 0, 0, 255), 0.5f);

    if (!player_name.empty()) {
        char distance_text[32];
        sprintf_s(distance_text, "%.0fm", distance);

        ImVec2 text_pos(pos.x - 15, pos.y + 15);
        draw_text_with_shadow_enhanced(10.0f, text_pos, arrow_color, distance_text);
    }
}

float cross_product_2d(const ImVec2& p, const ImVec2& q, const ImVec2& r) {
    return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
}

std::vector<ImVec2> convexHull(std::vector<ImVec2>& points) {
    int n = points.size();
    if (n <= 3) return points;

    int bottommost_index = 0;
    for (int i = 1; i < n; ++i) {
        if (points[i].y < points[bottommost_index].y || (points[i].y == points[bottommost_index].y && points[i].x < points[bottommost_index].x)) {
            bottommost_index = i;
        }
    }
    std::swap(points[0], points[bottommost_index]);
    ImVec2 p0 = points[0];

    std::sort(points.begin() + 1, points.end(), [&](const ImVec2& a, const ImVec2& b) {
        float cross = cross_product_2d(p0, a, b);
        if (cross == 0) {
            float distSqA = (p0.x - a.x) * (p0.x - a.x) + (p0.y - a.y) * (p0.y - a.y);
            float distSqB = (p0.x - b.x) * (p0.x - b.x) + (p0.y - b.y) * (p0.y - b.y);
            return distSqA < distSqB;
        }
        return cross > 0;
        });

    std::vector<ImVec2> hull;
    hull.push_back(points[0]);
    hull.push_back(points[1]);

    for (int i = 2; i < n; ++i) {
        while (hull.size() > 1 && cross_product_2d(hull[hull.size() - 2], hull.back(), points[i]) <= 0) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }

    return hull;
}

Vector3 Rotate(const Vector3& vec, const Matrix3& rotation_matrix) {
    return Vector3{
        vec.x * rotation_matrix.data[0] + vec.y * rotation_matrix.data[1] + vec.z * rotation_matrix.data[2],
        vec.x * rotation_matrix.data[3] + vec.y * rotation_matrix.data[4] + vec.z * rotation_matrix.data[5],
        vec.x * rotation_matrix.data[6] + vec.y * rotation_matrix.data[7] + vec.z * rotation_matrix.data[8]
    };
}

std::vector<Vector3> GetCorners(const Vector3& partCF, const Vector3& partSize) {
    std::vector<Vector3> corners;
    corners.reserve(8);

    for (int X : {-1, 1}) {
        for (int Y : {-1, 1}) {
            for (int Z : {-1, 1}) {
                corners.emplace_back(
                    partCF.x + partSize.x * X,
                    partCF.y + partSize.y * Y,
                    partCF.z + partSize.z * Z
                );
            }
        }
    }
    return corners;
}

namespace esp_helpers {
    bool on_screen(const Vector2& pos) {
        auto dimensions = globals::instances::visualengine.get_dimensions();
        if (dimensions.x == -1 || dimensions.y == -1) return false;
        return pos.x > -1 && pos.x < dimensions.x && pos.y > -1 && pos.y - 28 < dimensions.y;
    }

    bool get_player_bounds(roblox::player& player, Vector2& top_left, Vector2& bottom_right, float& margin) {
        // True bounding box: Calculate from all part corners
        std::vector<roblox::instance> parts;
        
        // Collect all valid parts based on rig type
        if (player.rigtype == 0) {
            // R6 rig
            if (is_valid_address(player.head.address)) parts.push_back(player.head);
            if (is_valid_address(player.uppertorso.address)) parts.push_back(player.uppertorso);
            if (is_valid_address(player.lefthand.address)) parts.push_back(player.lefthand);
            if (is_valid_address(player.righthand.address)) parts.push_back(player.righthand);
            if (is_valid_address(player.leftfoot.address)) parts.push_back(player.leftfoot);
            if (is_valid_address(player.rightfoot.address)) parts.push_back(player.rightfoot);
        } else {
            // R15 rig
            if (is_valid_address(player.head.address)) parts.push_back(player.head);
            if (is_valid_address(player.uppertorso.address)) parts.push_back(player.uppertorso);
            if (is_valid_address(player.lowertorso.address)) parts.push_back(player.lowertorso);
            if (is_valid_address(player.leftupperarm.address)) parts.push_back(player.leftupperarm);
            if (is_valid_address(player.rightupperarm.address)) parts.push_back(player.rightupperarm);
            if (is_valid_address(player.leftlowerarm.address)) parts.push_back(player.leftlowerarm);
            if (is_valid_address(player.rightlowerarm.address)) parts.push_back(player.rightlowerarm);
            if (is_valid_address(player.lefthand.address)) parts.push_back(player.lefthand);
            if (is_valid_address(player.righthand.address)) parts.push_back(player.righthand);
            if (is_valid_address(player.leftupperleg.address)) parts.push_back(player.leftupperleg);
            if (is_valid_address(player.rightupperleg.address)) parts.push_back(player.rightupperleg);
            if (is_valid_address(player.leftlowerleg.address)) parts.push_back(player.leftlowerleg);
            if (is_valid_address(player.rightlowerleg.address)) parts.push_back(player.rightlowerleg);
            if (is_valid_address(player.leftfoot.address)) parts.push_back(player.leftfoot);
            if (is_valid_address(player.rightfoot.address)) parts.push_back(player.rightfoot);
        }

        if (parts.empty()) {
            // Fallback: try head and HRP
            if (is_valid_address(player.head.address) && is_valid_address(player.hrp.address)) {
                Vector3 head_pos = player.head.get_pos();
                Vector3 hrp_pos = player.hrp.get_pos();
                Vector2 head2d = roblox::worldtoscreen(head_pos);
                Vector2 hrp2d = roblox::worldtoscreen(hrp_pos);
                
                if (!on_screen(head2d) && !on_screen(hrp2d)) {
                    return false;
                }

                Vector3 size3 = player.hrp.get_part_size();
                float height = std::max(10.0f, std::abs(head2d.y - hrp2d.y) * 2.0f);
                float width = (size3.x > 0.01f) ? std::clamp(size3.x * 25.0f, 10.0f, 400.0f) : std::clamp(height * 0.5f, 10.0f, 400.0f);
                Vector2 center = hrp2d;
                top_left = { center.x - width * 0.5f, head2d.y - (height * 0.2f) };
                bottom_right = { center.x + width * 0.5f, center.y + (height * 0.8f) };
                margin = 6.0f;
                return true;
            }
            return false;
        }

        // Calculate true bounding box from all part corners
        float min_x = FLT_MAX, min_y = FLT_MAX;
        float max_x = FLT_MIN, max_y = FLT_MIN;
        bool found_valid = false;

        for (auto& part : parts) {
            try {
                Vector3 part_size = part.get_part_size();
                
                Vector3 part_pos = part.get_pos();
                Matrix3 part_rotation = part.read_cframe().rotation_matrix;

                // Get all 8 corners of the part
                std::vector<Vector3> corners = GetCorners(
                    { 0, 0, 0 },
                    { part_size.x / 2, part_size.y / 2, part_size.z / 2 }
                );

                // Transform corners by rotation and position
                for (auto& corner : corners) {
                    corner = Rotate(corner, part_rotation);
                    corner = { corner.x + part_pos.x, corner.y + part_pos.y, corner.z + part_pos.z };
                    
                    // Project to screen
                    Vector2 screen_pos = roblox::worldtoscreen(corner);
                    if (on_screen(screen_pos)) {
                        min_x = std::min(min_x, screen_pos.x);
                        min_y = std::min(min_y, screen_pos.y);
                        max_x = std::max(max_x, screen_pos.x);
                        max_y = std::max(max_y, screen_pos.y);
                        found_valid = true;
                    }
                }
            } catch (...) {
                continue;
            }
        }

        if (!found_valid) {
            return false;
        }

        // Add small margin
        margin = 4.0f;
        top_left = { min_x - margin, min_y - margin };
        bottom_right = { max_x + margin, max_y + margin };
        return true;
    }
}

void visuals::updateESP() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    while (visuals::threadRunning) {
        try {
            std::this_thread::sleep_for(std::chrono::microseconds(5));
            if (!globals::visuals::visuals) {
                std::this_thread::sleep_for(std::chrono::microseconds(7500));
                continue;
            }

            // In some games like Phantom Forces and Bad Business, local player's humanoid/HRP may not be resolved the same way.
            // Detect PF and Bad Business by PlaceId and bypass strict local HRP requirement so ESP can still render from cached data.
            bool bypassLocalHRP = false;
            if (is_valid_address(globals::instances::datamodel.address)) {
                try {
                    uint64_t placeId = read<uint64_t>(globals::instances::datamodel.address + offsets::PlaceId);
                    bypassLocalHRP = (placeId == 292439477ULL) || (placeId == 3233893879ULL); // Phantom Forces or Bad Business
                } catch (...) { bypassLocalHRP = false; }
            }
            if (!bypassLocalHRP && !is_valid_address(globals::instances::lp.hrp.address)) {
                std::this_thread::sleep_for(std::chrono::microseconds(7500));
                continue;
            }

            std::vector<CachedPlayerData> tempPlayers;
            Vector3 local_pos = globals::instances::camera.getPos();
            visuals::espData.cachedmutex.lock();
            
            
            // Debug: Show Bad Business detection
            bool isBadBusiness = false;
            if (is_valid_address(globals::instances::datamodel.address)) {
                try {
                    uint64_t placeId = read<uint64_t>(globals::instances::datamodel.address + offsets::PlaceId);
                    isBadBusiness = (placeId == 3233893879ULL);
                } catch (...) { isBadBusiness = false; }
            }
            
            // Add bots to the player list
            std::vector<roblox::player> bots = GetBots(base_address);
            std::vector<roblox::player> all_players = globals::instances::cachedplayers;
            all_players.insert(all_players.end(), bots.begin(), bots.end());
            
            for (auto& player : all_players) {
                if (!is_valid_address(player.main.address)) {
                    continue;
                }
                
                // Skip local player if selfesp is disabled
                if (player.name == globals::instances::lp.name && !globals::visuals::selfesp) {
                    continue;
                }
                
                // Additional check: skip if this is the local player character
                if (player.name == globals::instances::localplayer.get_name() && !globals::visuals::selfesp) {
                    continue;
                }

                // Skip ESP for whitelisted players
                if (std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), player.name) != globals::instances::whitelist.end())
                    continue;

                // Teamcheck: use unified globals::is_teammate (covers Arsenal via offsets::Team)
                if ((globals::visuals::teamcheck || globals::combat::teamcheck) && globals::is_teammate(player)) {
                    printf("TeamCheck: SKIPPING teammate %s (globals::is_teammate)\n", player.name.c_str());
                    goto skipPlayer;
                }



                // Grabbed check - skip players who are grabbed if enabled
                if (globals::combat::grabbedcheck && globals::is_grabbed(player)) {
                    // Debug output removed to reduce console spam
                    continue; // Skip grabbed player
                }

                if (!is_valid_address(player.hrp.address)) {
                    // Debug output removed to reduce console spam
                    continue;
                }

                try {
                    CachedPlayerData data;
                    data.player = player;
                    data.position = player.hrp.get_pos();
                    data.distance = (local_pos - data.position).magnitude();
                    
                    // Preserve existing trail points for this player
                    if (globals::visuals::trail) {
                        for (const auto& existingPlayer : visuals::espData.cachedPlayers) {
                            if (existingPlayer.name == player.name) {
                                data.trail_points = existingPlayer.trail_points;
                                break;
                            }
                        }
                    }
                    
                    // Skip players beyond max distance if enabled
                    if (globals::visuals::maxdistance_enabled && data.distance > globals::visuals::maxdistance)
                        continue;

                    if (globals::visuals::nametype == 0)
                        data.name = player.name;
                    else
                        data.name = player.displayname;

                    data.tool_name = player.toolname;
                    
                    // Check if this is Phantom Forces and use cached health values
                    bool isPhantomForces = (globals::instances::gamename == "Phantom Forces" || 
                                          globals::instances::gamename.find("Phantom") != std::string::npos);
                    
                    if (isPhantomForces && player.maxhealth > 0) {
                        // For Phantom Forces, use the cached health values that were already read
                        data.health = static_cast<float>(player.health) / static_cast<float>(player.maxhealth);
                        // Debug output removed to reduce console spam
                        
                        // Additional debug: Check if the values make sense
                        if (data.health > 1.0f || data.health < 0.0f) {
                            // Debug output removed to reduce console spam
                            data.health = std::clamp(data.health, 0.0f, 1.0f);
                        }
                    } else if (player.maxhealth > 0) {
                        // For other games, try to read from humanoid directly
                        if (is_valid_address(player.humanoid.address)) {
                            try {
                                data.health = read<float>(player.humanoid.address + offsets::Health) / player.maxhealth;
                            } catch (...) {
                                data.health = static_cast<float>(player.health) / static_cast<float>(player.maxhealth);
                            }
                        } else {
                            data.health = static_cast<float>(player.health) / static_cast<float>(player.maxhealth);
                        }
                    } else {
                        // Try to fetch maxhealth if humanoid exists, otherwise default to 100
                        if (is_valid_address(player.humanoid.address)) {
                            try {
                                player.maxhealth = read<float>(player.humanoid.address + offsets::MaxHealth);
                                if (player.maxhealth > 0) {
                                    try {
                                        data.health = read<float>(player.humanoid.address + offsets::Health) / player.maxhealth;
                                    } catch (...) {
                                        data.health = static_cast<float>(player.health) / static_cast<float>(player.maxhealth);
                                    }
                                }
                            } catch (...) {
                                player.maxhealth = 100;
                                data.health = static_cast<float>(player.health) / 100.0f;
                            }
                        } else {
                            player.maxhealth = 100;
                            data.health = static_cast<float>(player.health) / 100.0f;
                        }
                    }

                    // Armor percent computed on draw to avoid struct coupling

                    Vector2 top_left, bottom_right;
                    float margin;
                    data.valid = esp_helpers::get_player_bounds(player, top_left, bottom_right, margin);

                    if (data.valid) {
                        data.top_left = top_left;
                        data.bottom_right = bottom_right;
                        data.margin = margin;
                        
                        // Add trail point for this player
                        if (globals::visuals::trail) {
                            data.trail_points.push_back(TrailPoint(data.position));
                        }
                        
                        tempPlayers.push_back(data);
                    }
                }
                catch (...) {
                    continue;
                }
                
                skipPlayer:
                continue;
            }

            if (!tempPlayers.empty()) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                std::sort(tempPlayers.begin(), tempPlayers.end(),
                    [](const CachedPlayerData& a, const CachedPlayerData& b) {
                        return a.distance < b.distance;
                    });

                // Debug: Show how many players made it through filtering
                if (globals::visuals::teamcheck || globals::combat::teamcheck) {
                    // Debug output removed to reduce console spam
                }

                visuals::espData.cachedPlayers = std::move(tempPlayers);
                visuals::espData.localPos = local_pos;
            } else {
                // Debug: Show when no players pass filtering
                if (globals::visuals::teamcheck || globals::combat::teamcheck) {
                    // Debug output removed to reduce console spam
                }
            }
        }
        catch (...) {
        }
        visuals::espData.cachedmutex.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }
}









static void render_radar_enhanced() {
    if (!globals::visuals::radar) return;

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    float radar_size = globals::visuals::radar_size;
    ImVec2 radar_pos(globals::visuals::radar_pos[0], globals::visuals::radar_pos[1]);
    
    ImDrawList* draw_list = globals::instances::draw;
    
    ImColor bg_color = ImColor(
        globals::visuals::radar_background[0],
        globals::visuals::radar_background[1],
        globals::visuals::radar_background[2],
        globals::visuals::radar_background[3]
    );
    draw_list->AddRectFilled(radar_pos, ImVec2(radar_pos.x + radar_size, radar_pos.y + radar_size), bg_color);
    
    if (globals::visuals::radar_outline) {
        ImColor border_color = ImColor(
            globals::visuals::radar_border[0],
            globals::visuals::radar_border[1],
            globals::visuals::radar_border[2],
            globals::visuals::radar_border[3]
        );
        draw_list->AddRect(radar_pos, ImVec2(radar_pos.x + radar_size, radar_pos.y + radar_size), border_color);
    }
    
    ImVec2 center(radar_pos.x + radar_size * 0.5f, radar_pos.y + radar_size * 0.5f);
    ImColor cross_color = ImColor(255, 255, 255, 100);
    draw_list->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y), cross_color);
    draw_list->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10), cross_color);
    
    roblox::camera camera = globals::instances::camera;
    Matrix3 camera_rotation = camera.getRot();
    Vector3 camera_forward = camera_rotation.getColumn(2);
    float camera_yaw = atan2(camera_forward.z, camera_forward.x);
    
    float label_angle_offset = globals::visuals::radar_rotate ? -camera_yaw : 0.0f;
    
    float north_angle = label_angle_offset;
    ImVec2 north_pos(
        center.x + cos(north_angle) * (radar_size * 0.4f),
        center.y + sin(north_angle) * (radar_size * 0.4f)
    );
    draw_text_with_shadow_enhanced(12.0f, ImVec2(north_pos.x - 5, north_pos.y - 5), 
                                 ImColor(255, 255, 255, 255), "N");
    
    float south_angle = label_angle_offset + 3.14159f;
    ImVec2 south_pos(
        center.x + cos(south_angle) * (radar_size * 0.4f),
        center.y + sin(south_angle) * (radar_size * 0.4f)
    );
    draw_text_with_shadow_enhanced(12.0f, ImVec2(south_pos.x - 5, south_pos.y - 5), 
                                 ImColor(255, 255, 255, 255), "S");
    
    float east_angle = label_angle_offset + 1.5708f;
    ImVec2 east_pos(
        center.x + cos(east_angle) * (radar_size * 0.4f),
        center.y + sin(east_angle) * (radar_size * 0.4f)
    );
    draw_text_with_shadow_enhanced(12.0f, ImVec2(east_pos.x - 5, east_pos.y - 5), 
                                 ImColor(255, 255, 255, 255), "E");
    
    float west_angle = label_angle_offset - 1.5708f;
    ImVec2 west_pos(
        center.x + cos(west_angle) * (radar_size * 0.4f),
        center.y + sin(west_angle) * (radar_size * 0.4f)
    );
    draw_text_with_shadow_enhanced(12.0f, ImVec2(west_pos.x - 5, west_pos.y - 5), 
                                 ImColor(255, 255, 255, 255), "W");
    
    visuals::espData.cachedmutex.lock();
    for (const auto& player : visuals::espData.cachedPlayers) {
        if (!player.valid) continue;
        
        Vector3 player_pos = player.position;
        Vector3 local_pos = visuals::espData.localPos;
        
        Vector3 delta = player_pos - local_pos;
        float distance = delta.magnitude();
        
        if (distance > globals::visuals::radar_range) continue;
        
        float radar_distance = (distance / globals::visuals::radar_range) * (radar_size * 0.4f);
        float angle = atan2(delta.z, delta.x);
        
        if (globals::visuals::radar_rotate) {
            angle -= camera_yaw;
        }
        
        ImVec2 radar_point(
            center.x + cos(angle) * radar_distance,
            center.y + sin(angle) * radar_distance
        );
        
        if (radar_point.x >= radar_pos.x && radar_point.x <= radar_pos.x + radar_size &&
            radar_point.y >= radar_pos.y && radar_point.y <= radar_pos.y + radar_size) {
            
            ImColor dot_color = ImColor(
                globals::visuals::radar_dot[0],
                globals::visuals::radar_dot[1],
                globals::visuals::radar_dot[2],
                globals::visuals::radar_dot[3]
            );
            draw_list->AddCircleFilled(radar_point, 3.0f, dot_color);
            
            if (globals::visuals::radar_show_distance) {
                std::string distance_text = std::to_string((int)distance) + "m";
                draw_text_with_shadow_enhanced(10.0f, ImVec2(radar_point.x + 5, radar_point.y - 5), 
                                             ImColor(255, 255, 255, 255), distance_text.c_str());
            }
        }
    }
    visuals::espData.cachedmutex.unlock();
}



void visuals::stopThread() {
    if (threadRunning) {
        threadRunning = false;
        if (updateThread.joinable()) {
            updateThread.join();
        }
    }
}

static void render_sonar_enhanced() {
    if (!globals::visuals::sonar) return;

    if (!is_valid_address(globals::instances::lp.hrp.address)) return;

    ImDrawList* draw_list = globals::instances::draw;
    if (!draw_list) return;

    auto screen_dimensions = globals::instances::visualengine.get_dimensions();
    if (screen_dimensions.x == 0 || screen_dimensions.y == 0) return;

    // Use humanoid rootpart position directly for the sonar origin
    Vector3 humanoid_pos = globals::instances::lp.hrp.get_pos();
    
    roblox::camera camera = globals::instances::camera;
    Matrix3 camera_rotation = camera.getRot();
    Vector3 camera_forward = camera_rotation.getColumn(2);
    camera_forward = camera_forward * -1.0f; // Fix camera forward direction
    float camera_yaw = atan2(camera_forward.z, camera_forward.x);

    // Get current time for animation
    static auto start_time = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
    float time_seconds = elapsed / 1000.0f;

    // Static map to track when each player's dot was last shown
    static std::map<std::string, float> player_dot_times;
    static float last_scan_cycle = 0.0f;
    static float last_scan_time = 0.0f;
    
    // Check if we're starting a new scan cycle
    float scan_cycle = fmod(time_seconds * 0.8f, 1.0f);
    
    // Clear dots when a new scan cycle starts
    if (scan_cycle < last_scan_cycle && scan_cycle < 0.1f) {
        // New scan cycle started, clear old dots
        player_dot_times.clear();
    }
    
    last_scan_cycle = scan_cycle;
    last_scan_time = scan_cycle;

    // Center pulse removed - only the expanding sonar wave remains

    // Create a single expanding wave circle that radiates from the humanoid
    float expansion_progress = fmod(time_seconds * 0.8f, 1.0f);
    float current_radius = expansion_progress * globals::visuals::sonar_range;
    
    // Fade out as the circle expands
    float circle_alpha = 0.9f * (1.0f - expansion_progress);
    
    ImColor circle_color = ImColor(
        globals::visuals::sonarcolor[0],
        globals::visuals::sonarcolor[1],
        globals::visuals::sonarcolor[2],
        globals::visuals::sonarcolor[3] * circle_alpha
    );

    // Create a smooth expanding circle with more segments for better quality
    const int num_segments = 64; // Reduced for better performance and stability
    std::vector<ImVec2> screen_points;
    screen_points.reserve(num_segments + 1);
    
    // Get the center screen position first to validate it
    Vector2 center_screen = roblox::worldtoscreen(humanoid_pos);
    if (!std::isfinite(center_screen.x) || !std::isfinite(center_screen.y)) {
        return; // Center is invalid, don't render
    }
    
    // Collect all valid screen points first
    for (int i = 0; i <= num_segments; i++) {
        float angle = (2.0f * M_PI * i) / num_segments;
        
        // Create the single expanding circle at the humanoid position
        Vector3 circle_point_3d(
            humanoid_pos.x + cos(angle) * current_radius,
            humanoid_pos.y, // Keep at humanoid height
            humanoid_pos.z + sin(angle) * current_radius
        );

        Vector2 screen_point = roblox::worldtoscreen(circle_point_3d);
        
        // Check if the point is behind the camera or invalid (-1, -1 coordinates)
        if (screen_point.x == -1 && screen_point.y == -1) {
            continue; // Skip this point to maintain circle integrity
        }
        
        // Only add valid screen coordinates that are within reasonable bounds
        if (std::isfinite(screen_point.x) && std::isfinite(screen_point.y) &&
            screen_point.x > -1000 && screen_point.x < screen_dimensions.x + 1000 &&
            screen_point.y > -1000 && screen_point.y < screen_dimensions.y + 1000) {
            screen_points.push_back(ImVec2(screen_point.x, screen_point.y));
        }
    }

    // Always render since we now have all points (clamped if needed)
    if (screen_points.size() >= 8) {
        // Draw the circle by connecting all points in sequence, but only if they're reasonably close
        for (size_t i = 0; i < screen_points.size() - 1; i++) {
            // Calculate distance between consecutive points
            float dx = screen_points[i + 1].x - screen_points[i].x;
            float dy = screen_points[i + 1].y - screen_points[i].y;
            float distance = sqrtf(dx * dx + dy * dy);
            
            // Only draw line if points are reasonably close (prevents long broken lines)
            if (distance < 500.0f) {
                draw_list->AddLine(
                    screen_points[i],
                    screen_points[i + 1],
                    circle_color,
                    globals::visuals::sonar_thickness * (1.0f + expansion_progress * 0.3f)
                );
            }
        }
        
        // Close the circle by connecting the last point to the first (if they're close enough)
        if (screen_points.size() > 2) {
            float dx = screen_points.front().x - screen_points.back().x;
            float dy = screen_points.front().y - screen_points.back().y;
            float distance = sqrtf(dx * dx + dy * dy);
            
            if (distance < 500.0f) {
                draw_list->AddLine(
                    screen_points.back(),
                    screen_points.front(),
                    circle_color,
                    globals::visuals::sonar_thickness * (1.0f + expansion_progress * 0.3f)
                );
            }
        }
    }

    visuals::espData.cachedmutex.lock();
    for (const auto& player : visuals::espData.cachedPlayers) {
        if (!player.valid) continue;
        
        // Skip if this is the local player (don't show dot on yourself)
        if (player.name == globals::instances::lp.name) continue;
        
        Vector3 player_pos = player.position;
        Vector3 delta = player_pos - humanoid_pos;
        float distance = delta.magnitude();
        
        if (distance > globals::visuals::sonar_range) continue;
        
                          // Only show player dots when the sonar wave passes over them
         // The sonar wave is currently at current_radius distance from center
         float wave_distance = current_radius;
         float distance_to_wave = abs(distance - wave_distance);
         
         // Show player dot only when the wave is very close to the player (within 25 units)
         if (distance_to_wave <= 25.0f) {
             Vector2 screen_pos = roblox::worldtoscreen(player_pos);
             if (screen_pos.x > 0 && screen_pos.x < screen_dimensions.x && 
                 screen_pos.y > 0 && screen_pos.y < screen_dimensions.y) {
                 
                 // Record the time when the wave first passes over this player (only once)
                 if (player_dot_times.find(player.name) == player_dot_times.end()) {
                     player_dot_times[player.name] = time_seconds;
                 }
                 
                                   // Calculate fade effect based on distance to wave
                  float fade_factor = 1.0f - (distance_to_wave / 25.0f);
                  fade_factor = std::max(0.0f, std::min(1.0f, fade_factor));
                 
                 // Apply smooth fade curve
                 float dot_alpha = 0.9f * fade_factor;
                 
                                                       ImColor dot_color = ImColor(
                        globals::visuals::boxcolors[0],
                        globals::visuals::boxcolors[1],
                        globals::visuals::boxcolors[2],
                        globals::visuals::boxcolors[3] * dot_alpha
                    );
                    
                    // Create a glowing effect with multiple layers
                    // Outer glow (larger, more transparent)
                    ImColor glow_color = ImColor(
                        globals::visuals::sonar_dot_color[0],
                        globals::visuals::sonar_dot_color[1],
                        globals::visuals::sonar_dot_color[2],
                        (globals::visuals::sonar_dot_color[3] * dot_alpha) * 0.3f
                    );
                    draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), 8.0f, glow_color);
                    
                    // Middle glow (medium size, medium transparency)
                    ImColor middle_glow = ImColor(
                        globals::visuals::sonar_dot_color[0],
                        globals::visuals::sonar_dot_color[1],
                        globals::visuals::sonar_dot_color[2],
                        (globals::visuals::sonar_dot_color[3] * dot_alpha) * 0.6f
                    );
                    draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), 5.0f, middle_glow);
                    
                    // Core dot (small, fully opaque)
                    ImColor core_dot = ImColor(
                        globals::visuals::sonar_dot_color[0],
                        globals::visuals::sonar_dot_color[1],
                        globals::visuals::sonar_dot_color[2],
                        globals::visuals::sonar_dot_color[3] * dot_alpha
                    );
                    draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), 3.0f, core_dot);
             }
         }
    }
    

    

    visuals::espData.cachedmutex.unlock();
}

static void render_desync_visualizer() {
    if (!globals::misc::desync_visualizer) return;
    if (!globals::misc::desync_active) return; // Only show when desync is active

    ImDrawList* draw_list = globals::instances::draw;
    if (!draw_list) return;

    auto screen_dimensions = globals::instances::visualengine.get_dimensions();
    if (screen_dimensions.x == 0 || screen_dimensions.y == 0) return;

    // Use the captured activation position
    Vector3 activation_pos = globals::misc::desync_activation_pos;
    
    // Static circle - no animation
    float current_radius = globals::misc::desync_viz_range;
    
    // Full opacity - no fade
    float circle_alpha = 1.0f;
    
    ImColor circle_color = ImColor(
        globals::misc::desync_viz_color[0],
        globals::misc::desync_viz_color[1],
        globals::misc::desync_viz_color[2],
        globals::misc::desync_viz_color[3] * circle_alpha
    );

    // Create static circle with segments for smooth rendering
    const int num_segments = 64;
    std::vector<ImVec2> screen_points;
    screen_points.reserve(num_segments + 1);
    
    // Get the center screen position
    Vector2 center_screen = roblox::worldtoscreen(activation_pos);
    if (!std::isfinite(center_screen.x) || !std::isfinite(center_screen.y)) {
        return; // Center is invalid, don't render
    }
    
    // Collect all valid screen points
    for (int i = 0; i <= num_segments; i++) {
        float angle = (2.0f * M_PI * i) / num_segments;
        
        // Create the static circle at the activation position
        Vector3 circle_point_3d(
            activation_pos.x + cos(angle) * current_radius,
            activation_pos.y, // Keep at activation height
            activation_pos.z + sin(angle) * current_radius
        );

        Vector2 screen_point = roblox::worldtoscreen(circle_point_3d);
        
        // Check if the point is behind the camera or invalid
        if (screen_point.x == -1 && screen_point.y == -1) {
            continue;
        }
        
        // Only add valid screen coordinates
        if (std::isfinite(screen_point.x) && std::isfinite(screen_point.y) &&
            screen_point.x > -1000 && screen_point.x < screen_dimensions.x + 1000 &&
            screen_point.y > -1000 && screen_point.y < screen_dimensions.y + 1000) {
            screen_points.push_back(ImVec2(screen_point.x, screen_point.y));
        }
    }

    // Draw the circle if we have enough points
    if (screen_points.size() >= 8) {
        // Draw the circle by connecting all points in sequence
        for (size_t i = 0; i < screen_points.size() - 1; i++) {
            draw_list->AddLine(
                screen_points[i], 
                screen_points[i + 1], 
                circle_color, 
                globals::misc::desync_viz_thickness
            );
        }
        
        // Close the circle by connecting last point to first
        if (screen_points.size() > 2) {
            draw_list->AddLine(
                screen_points[screen_points.size() - 1], 
                screen_points[0], 
                circle_color, 
                globals::misc::desync_viz_thickness
            );
        }
    }
}

void visuals::run() {
    if (!threadRunning) {
        updateThread = std::thread(visuals::updateESP);
        updateThread.detach();
        std::thread updatechams(utils::CacheChamsPoly);
        updatechams.detach();
        threadRunning = true;
    }

    if (!globals::visuals::visuals || !globals::instances::draw) {
        return;
    }

    auto screen_dimensions = globals::instances::visualengine.get_dimensions();
    if (screen_dimensions.x == 0 || screen_dimensions.y == 0) {
        return;
    }

    // Apply fog settings if enabled
    if (globals::visuals::fog && is_valid_address(globals::instances::lighting.address)) {
        try {
            globals::instances::lighting.write_fogstart(globals::visuals::fog_start);
            globals::instances::lighting.write_fogend(globals::visuals::fog_end);
            
            Vector3 fog_color_vec;
            
            // Rainbow fog - cycle through colors
            if (globals::visuals::rainbow_fog) {
                static float rainbow_time = 0.0f;
                rainbow_time += globals::visuals::rainbow_fog_speed * 0.01f;
                if (rainbow_time > 1.0f) rainbow_time -= 1.0f;
                
                // HSV to RGB conversion for rainbow effect
                float hue = rainbow_time * 360.0f;
                float h = hue / 60.0f;
                int i = (int)h;
                float f = h - i;
                float p = 0.0f;
                float q = 1.0f - f;
                float t = f;
                
                switch (i % 6) {
                    case 0: fog_color_vec = Vector3(1.0f, t, p); break;
                    case 1: fog_color_vec = Vector3(q, 1.0f, p); break;
                    case 2: fog_color_vec = Vector3(p, 1.0f, t); break;
                    case 3: fog_color_vec = Vector3(p, q, 1.0f); break;
                    case 4: fog_color_vec = Vector3(t, p, 1.0f); break;
                    case 5: fog_color_vec = Vector3(1.0f, p, q); break;
                    default: fog_color_vec = Vector3(1.0f, 0.0f, 0.0f); break;
                }
            } else {
                // Apply fog color if available
                if (globals::visuals::fog_color[0] >= 0.0f && globals::visuals::fog_color[1] >= 0.0f && globals::visuals::fog_color[2] >= 0.0f) {
                    fog_color_vec = Vector3(
                        globals::visuals::fog_color[0],
                        globals::visuals::fog_color[1],
                        globals::visuals::fog_color[2]
                    );
                } else {
                    fog_color_vec = Vector3(0.5f, 0.5f, 0.5f); // Default gray
                }
            }
            
            // Apply fog glow - create intense glowing effect on the fog
            if (globals::visuals::fog_glow) {
                Vector3 glow_color = Vector3(
                    globals::visuals::fog_glow_color[0],
                    globals::visuals::fog_glow_color[1],
                    globals::visuals::fog_glow_color[2]
                );
                
                // Create intense glow by heavily weighting towards the glow color
                float glow_strength = globals::visuals::fog_glow_intensity * 0.8f; // 0.8f max, scales with intensity
                
                // Blend strongly towards glow color (makes fog "pop" with the glow color)
                fog_color_vec.x = fog_color_vec.x * (1.0f - glow_strength) + glow_color.x * glow_strength;
                fog_color_vec.y = fog_color_vec.y * (1.0f - glow_strength) + glow_color.y * glow_strength;
                fog_color_vec.z = fog_color_vec.z * (1.0f - glow_strength) + glow_color.z * glow_strength;
                
                // Massive brightness boost to make the fog actually glow (not just change color)
                float glow_brightness = 1.0f + (globals::visuals::fog_glow_intensity * 2.5f); // Much stronger brightness boost
                
                // Apply brightness boost with saturation preservation
                float max_component = std::max({fog_color_vec.x, fog_color_vec.y, fog_color_vec.z});
                if (max_component > 0.001f) {
                    fog_color_vec.x = std::min(1.0f, fog_color_vec.x * glow_brightness);
                    fog_color_vec.y = std::min(1.0f, fog_color_vec.y * glow_brightness);
                    fog_color_vec.z = std::min(1.0f, fog_color_vec.z * glow_brightness);
                    
                    // Add extra intensity by pushing towards white/bright when intensity is high
                    if (globals::visuals::fog_glow_intensity > 2.0f) {
                        float white_boost = (globals::visuals::fog_glow_intensity - 2.0f) * 0.3f;
                        fog_color_vec.x = std::min(1.0f, fog_color_vec.x + (1.0f - fog_color_vec.x) * white_boost);
                        fog_color_vec.y = std::min(1.0f, fog_color_vec.y + (1.0f - fog_color_vec.y) * white_boost);
                        fog_color_vec.z = std::min(1.0f, fog_color_vec.z + (1.0f - fog_color_vec.z) * white_boost);
                    }
                }
                
                // Make fog appear closer/more intense by adjusting fog end slightly when glow is active
                // This makes the glow effect more visible
                float original_fog_end = globals::visuals::fog_end;
                float glow_fog_adjustment = original_fog_end * (0.15f * globals::visuals::fog_glow_intensity / 5.0f);
                globals::instances::lighting.write_fogend(original_fog_end - glow_fog_adjustment);
            }
            
            // Write the final fog color
            globals::instances::lighting.write_fogcolor(fog_color_vec);
            
            // Restore original fog end (glow modifies it temporarily above)
            if (!globals::visuals::fog_glow) {
                globals::instances::lighting.write_fogend(globals::visuals::fog_end);
            }
        } catch (...) {
            // Silently handle any errors with fog writing
        }
    }
    
    // Apply lighting modifications if enabled
    if (globals::visuals::lighting_modifications && is_valid_address(globals::instances::lighting.address)) {
        try {
            // Apply brightness (affects ambient and overall lighting)
            Vector3 ambient_color = Vector3(
                globals::visuals::lighting_ambient[0] * globals::visuals::lighting_brightness,
                globals::visuals::lighting_ambient[1] * globals::visuals::lighting_brightness,
                globals::visuals::lighting_ambient[2] * globals::visuals::lighting_brightness
            );
            globals::instances::lighting.write_ambiance(ambient_color);
            
            // Apply color correction
            Vector3 color_correction = Vector3(
                globals::visuals::lighting_color_correction[0],
                globals::visuals::lighting_color_correction[1],
                globals::visuals::lighting_color_correction[2]
            );
            // Color correction would typically affect fog color or use a different method
            // This is a simplified implementation
        } catch (...) {
            // Silently handle errors
        }
    }


    // Workspace viewer - modify workspace instances
    if (globals::visuals::workspace_viewer && is_valid_address(globals::instances::workspace.address)) {
        try {
            auto workspace_children = globals::instances::workspace.get_children();
            for (auto& instance : workspace_children) {
                if (!is_valid_address(instance.address)) continue;
                
                try {
                    // Apply transparency modifier
                    if (globals::visuals::workspace_transparency_modifier) {
                        // Note: Transparency is typically set per-part, this is a simplified implementation
                        // You may need to get the part and set its transparency property
                    }
                    
                    // Apply CanCollide modifier
                    if (globals::visuals::workspace_cancollide_modifier) {
                        instance.write_cancollide(globals::visuals::workspace_cancollide_value);
                    }
                    
                    // Apply Anchored modifier
                    if (globals::visuals::workspace_anchored_modifier) {
                        instance.setAnchored(globals::visuals::workspace_anchored_value);
                    }
                    
                    // Display position, size, velocity if enabled (this would typically be shown in a UI list, not in ESP)
                    // For now, we'll just apply the modifications
                } catch (...) {
                    // Silently handle errors for individual instances
                }
            }
        } catch (...) {
            // Silently handle workspace access errors
        }
    }
    
    // Radar removed
    render_sonar_enhanced();
    render_desync_visualizer();
    render_chams_enhanced(); // Render chams in overlay
    visuals::espData.cachedmutex.lock();

    // Debug: Show how many players are being rendered
    if (globals::visuals::teamcheck || globals::combat::teamcheck) {
        // Debug output removed to reduce console spam
    }
    
    for (auto& player : visuals::espData.cachedPlayers) {
        if (!player.valid)
            continue;

        // Debug: Show which players are being rendered
        if (globals::visuals::teamcheck || globals::combat::teamcheck) {
            // Debug output removed to reduce console spam
        }

        Vector2 screen_check = roblox::worldtoscreen(player.position);
        if (screen_check.x <= 0 || screen_check.x >= screen_dimensions.x ||
            screen_check.y <= 0 || screen_check.y >= screen_dimensions.y) {
            if (player.distance > 100.0f) {
                continue;
            }
        }

        Vector2 top_left = player.top_left;
        Vector2 bottom_right = player.bottom_right;

        if (!esp_helpers::on_screen(top_left) || !esp_helpers::on_screen(bottom_right))
            continue;

        float alpha_scale = 1.0f;

        // Render health bar FIRST so its top border is always visible
        if (globals::visuals::healthbar) {
            // Use the calculated health percentage from CachedPlayerData
            float health_percent = player.health;
            
            // Debug output removed to reduce console spam
            
            render_health_bar_enhanced(top_left, bottom_right, health_percent, ImColor(255, 0, 0));
        }

        ImColor player_color = ImColor(255, 255, 255);

        if (globals::visuals::boxes) {
            player_color = ImColor(
                globals::visuals::boxcolors[0],
                globals::visuals::boxcolors[1],
                globals::visuals::boxcolors[2],
                globals::visuals::boxcolors[3]
            );
            player_color.Value.w *= alpha_scale;
            if (globals::visuals::lockedindicator) {
                if (globals::instances::cachedtarget.name == player.name) {
                    player_color = ImColor(
                        globals::visuals::lockedcolor[0],
                        globals::visuals::lockedcolor[1],
                        globals::visuals::lockedcolor[2],
                        globals::visuals::lockedcolor[3]
                    );
                    player_color.Value.w *= alpha_scale;
                }
            }

            render_box_enhanced(top_left, bottom_right, player_color);
        }

		if (globals::visuals::name) {
			render_name_enhanced(player.name, top_left, bottom_right);
		}

        // Team name ESP
        if (globals::visuals::teams) {
            render_team_name_enhanced(player.player, top_left, bottom_right);
        }

        // Distance ESP
        if (globals::visuals::distance) {
            render_distance_enhanced(player.distance, top_left, bottom_right, !player.tool_name.empty());
        }

        if (globals::visuals::toolesp && !player.tool_name.empty()) {
            render_tool_name_enhanced(player.tool_name, top_left, bottom_right);
        }

        if (globals::visuals::tracers) {
            render_snapline_enhanced(screen_check);
        }
        
        if (globals::visuals::trail) {
            render_trail_enhanced(player);
        }

        // Armor bar (universal): compute from BodyEffects if available
        if (globals::visuals::armorbar) {
            float armor_percent = 0.0f;
            try {
                if (is_valid_address(player.player.bodyeffects.address)) {
                    roblox::instance body = player.player.bodyeffects;
                    const char* armor_candidates[] = { "Armor", "Armour", "Defense", "Defence" };
                    int armorValue = -1;
                    for (const char* nm : armor_candidates) {
                        auto node = body.findfirstchild(nm);
                        if (!node.address) continue;
                        try { armorValue = node.read_int_value(); } catch (...) {}
                        if (armorValue < 0) { try { armorValue = (int)node.read_double_value(); } catch (...) {} }
                        if (armorValue >= 0) break;
                    }
                    if (armorValue >= 0) armor_percent = std::clamp(armorValue / 100.0f, 0.0f, 1.0f);
                }
            } catch (...) {}
            if (armor_percent > 0.0f) {
                render_armor_bar_enhanced(top_left, bottom_right, armor_percent);
            }
        }

        if (globals::visuals::skeletons) {
            render_skeleton_enhanced(player, ImColor(
                globals::visuals::skeletonscolor[0],
                globals::visuals::skeletonscolor[1],
                globals::visuals::skeletonscolor[2],
                globals::visuals::skeletonscolor[3]
            ));
        }

        // HeadDot feature removed

        // Flags: Display player state information
        if (globals::visuals::flags) {
            std::vector<std::string> flags;
            
            // Get player velocity for movement flags
            if (is_valid_address(player.player.hrp.address)) {
                Vector3 velocity = player.player.hrp.get_velocity();
                float velocity_magnitude = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
                float vertical_velocity = velocity.y;

                if (velocity_magnitude < 0.5f) {
                    flags.push_back("IDLE");
                }
                else if (velocity_magnitude > 0.5f && velocity_magnitude < 16.0f && abs(vertical_velocity) < 2.0f) {
                    flags.push_back("WALKING");
                }
                else if (velocity_magnitude >= 16.0f && abs(vertical_velocity) < 2.0f) {
                    flags.push_back("RUNNING");
                }

                if (vertical_velocity > 5.0f) {
                    flags.push_back("JUMPING");
                }
                else if (vertical_velocity < -5.0f) {
                    flags.push_back("FALLING");
                }
            }

            // Health status flag
            if (player.health <= 0) {
                flags.push_back("DEAD");
            }

            // Render flags
            if (!flags.empty()) {
                float flag_y_offset = 0;
                ImColor flag_color = ImColor(
                    globals::visuals::flagscolor[0],
                    globals::visuals::flagscolor[1],
                    globals::visuals::flagscolor[2],
                    globals::visuals::flagscolor[3]
                );
                
                // Get font for consistent rendering with names
                ImFont* font = nullptr;
                ImGuiIO& io = ImGui::GetIO();
                if (io.Fonts && io.Fonts->Fonts.Size > 0) {
                    if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1]; else font = io.Fonts->Fonts[0];
                }
                float font_size = globals::visuals::flags_font_size; // Configurable font size for flags
                
                for (const std::string& flag : flags) {
                    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, flag.c_str()) : ImGui::CalcTextSize(flag.c_str());
                    ImVec2 flag_pos = ImVec2(bottom_right.x + globals::visuals::flags_offset_x, top_left.y + flag_y_offset);
                    
                    // draw_text_with_shadow_enhanced already handles pixel-perfect 1px outline in all 8 directions
                    // Apply glow effect if enabled
                    if ((*globals::visuals::flag_overlay_flags)[1]) {
                        ImColor glow_color = flag_color;
                        glow_color.Value.w *= 0.5f;
                        ::draw_text_with_shadow_enhanced(font_size, flag_pos, glow_color, flag.c_str());
                        ::draw_text_with_shadow_enhanced(font_size, flag_pos, glow_color, flag.c_str());
                    }
                    
                    // Draw main flag text
                    ::draw_text_with_shadow_enhanced(font_size, flag_pos, flag_color, flag.c_str());
                    
                    flag_y_offset += text_size.y + 1;
                }
            }
        }

        // China Hat: draw a 3D hat above the player's head
        if (globals::visuals::chinahat) {
            // Check if target-only mode is enabled and if this player is the locked target
            bool should_draw_hat = true;
            if (globals::visuals::chinahat_target_only) {
                // Only show china hat if we have a valid target and this player is that target
                should_draw_hat = (globals::instances::cachedtarget.head.address != 0) && 
                                 (globals::instances::cachedtarget.name == player.player.name);
            }
            
            if (should_draw_hat && is_valid_address(player.player.head.address)) {
                Vector3 head_pos = player.player.head.get_pos();
                head_pos.y += 0.4f; // Offset above the head
                ImColor hat_color = ImColor(
                    globals::visuals::chinahat_color[0],
                    globals::visuals::chinahat_color[1],
                    globals::visuals::chinahat_color[2],
                    globals::visuals::chinahat_color[3]
                );
                hat_color.Value.w *= alpha_scale;
                DrawChinaHat(globals::instances::draw, head_pos, hat_color);
            }
        }

        // Glow ESP: Draw a glow effect around the player
        if (globals::visuals::glowesp) {
            ImColor glow_color = ImColor(
                globals::visuals::glowcolor[0],
                globals::visuals::glowcolor[1],
                globals::visuals::glowcolor[2],
                globals::visuals::glowcolor[3]
            );
            glow_color.Value.w *= alpha_scale;
            
            // Draw multiple layers of glow for a better effect
            float box_width = bottom_right.x - top_left.x;
            float box_height = bottom_right.y - top_left.y;
            float glow_size = std::max(box_width, box_height) * 0.1f; // 10% of box size
            
            // Outer glow (larger, more transparent)
            ImColor outer_glow = glow_color;
            outer_glow.Value.w *= 0.3f;
            globals::instances::draw->AddShadowRect(
                ImVec2(top_left.x - glow_size * 2, top_left.y - glow_size * 2),
                ImVec2(bottom_right.x + glow_size * 2, bottom_right.y + glow_size * 2),
                ImGui::GetColorU32(ImVec4(outer_glow.Value.x, outer_glow.Value.y, outer_glow.Value.z, outer_glow.Value.w)),
                20.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
            );
            
            // Middle glow (medium size, medium transparency)
            ImColor middle_glow = glow_color;
            middle_glow.Value.w *= 0.5f;
            globals::instances::draw->AddShadowRect(
                ImVec2(top_left.x - glow_size, top_left.y - glow_size),
                ImVec2(bottom_right.x + glow_size, bottom_right.y + glow_size),
                ImGui::GetColorU32(ImVec4(middle_glow.Value.x, middle_glow.Value.y, middle_glow.Value.z, middle_glow.Value.w)),
                15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
            );
            
            // Inner glow (smaller, more opaque)
            ImColor inner_glow = glow_color;
            inner_glow.Value.w *= 0.7f;
            globals::instances::draw->AddShadowRect(
                ImVec2(top_left.x - glow_size * 0.5f, top_left.y - glow_size * 0.5f),
                ImVec2(bottom_right.x + glow_size * 0.5f, bottom_right.y + glow_size * 0.5f),
                ImGui::GetColorU32(ImVec4(inner_glow.Value.x, inner_glow.Value.y, inner_glow.Value.z, inner_glow.Value.w)),
                10.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0
            );
        }
    }

    for (auto& player : visuals::espData.cachedPlayers) {
        if (!player.valid) continue;

        Vector2 screen_pos = roblox::worldtoscreen(player.position);
        auto dimensions = globals::instances::visualengine.get_dimensions();

        bool is_offscreen = screen_pos.x <= 0 || screen_pos.x >= dimensions.x ||
            screen_pos.y <= 0 || screen_pos.y >= dimensions.y;

    }

    // Render crosshair
    crosshair::render();
    


    visuals::espData.cachedmutex.unlock();
}

// Chams implementation
namespace chams {
    // Static cache for player part addresses to avoid repeated lookups
    static std::unordered_set<uintptr_t> valid_player_addresses;
    static std::chrono::steady_clock::time_point last_cache_update;
    static constexpr auto CACHE_UPDATE_INTERVAL = std::chrono::milliseconds(100);
    
    bool chams::is_valid_player(c_scene_animatable_object* object) {
        if (!object || !object->address) return false;
        
        // Update cache periodically
        auto now = std::chrono::steady_clock::now();
        if (now - last_cache_update > CACHE_UPDATE_INTERVAL) {
            valid_player_addresses.clear();
            
            visuals::espData.cachedmutex.lock();
            for (const auto& player : visuals::espData.cachedPlayers) {
                if (!player.valid) continue;
                
                // Cache all relevant part addresses
                if (is_valid_address(player.player.head.address))
                    valid_player_addresses.insert(player.player.head.address);
                if (is_valid_address(player.player.hrp.address))
                    valid_player_addresses.insert(player.player.hrp.address);
                if (is_valid_address(player.player.uppertorso.address))
                    valid_player_addresses.insert(player.player.uppertorso.address);
                if (is_valid_address(player.player.lowertorso.address))
                    valid_player_addresses.insert(player.player.lowertorso.address);
                if (is_valid_address(player.player.leftupperarm.address))
                    valid_player_addresses.insert(player.player.leftupperarm.address);
                if (is_valid_address(player.player.rightupperarm.address))
                    valid_player_addresses.insert(player.player.rightupperarm.address);
                if (is_valid_address(player.player.leftlowerarm.address))
                    valid_player_addresses.insert(player.player.leftlowerarm.address);
                if (is_valid_address(player.player.rightlowerarm.address))
                    valid_player_addresses.insert(player.player.rightlowerarm.address);
                if (is_valid_address(player.player.leftupperleg.address))
                    valid_player_addresses.insert(player.player.leftupperleg.address);
                if (is_valid_address(player.player.rightupperleg.address))
                    valid_player_addresses.insert(player.player.rightupperleg.address);
                if (is_valid_address(player.player.leftlowerleg.address))
                    valid_player_addresses.insert(player.player.leftlowerleg.address);
                if (is_valid_address(player.player.rightlowerleg.address))
                    valid_player_addresses.insert(player.player.rightlowerleg.address);
                if (is_valid_address(player.player.leftfoot.address))
                    valid_player_addresses.insert(player.player.leftfoot.address);
                if (is_valid_address(player.player.rightfoot.address))
                    valid_player_addresses.insert(player.player.rightfoot.address);
                if (is_valid_address(player.player.lefthand.address))
                    valid_player_addresses.insert(player.player.lefthand.address);
                if (is_valid_address(player.player.righthand.address))
                    valid_player_addresses.insert(player.player.righthand.address);
            }
            visuals::espData.cachedmutex.unlock();
            
            last_cache_update = now;
        }
        
        // Fast O(1) lookup instead of O(n) iteration
        return valid_player_addresses.find(object->address) != valid_player_addresses.end();
    }
    
    void chams::override_material(c_mesh_draw_primitive* mesh, void* material, const ImColor& color) {
        if (!mesh) return;
        
        mesh->m_material = material;
        mesh->m_material2 = material;
        mesh->m_tint_color = color;
    }
    
    bool chams::render(void* __this, c_scene_animatable_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf) {
        if (!globals::visuals::chams || !render_buf || !object) {
            return false;
        }
        
        if (!is_valid_player(object)) {
            return false;
        }
        
        // Get chams color
        ImColor chams_color = ImColor(
            globals::visuals::chamscolor[0],
            globals::visuals::chamscolor[1],
            globals::visuals::chamscolor[2],
            globals::visuals::chamscolor[3]
        );
        
        ImColor ignorez_color = ImColor(
            globals::visuals::chams_ignorez_color[0],
            globals::visuals::chams_ignorez_color[1],
            globals::visuals::chams_ignorez_color[2],
            globals::visuals::chams_ignorez_color[3]
        );
        
        // If ignorez is enabled, render invisible part first
        if (globals::visuals::chams_ignorez) {
            int prev_count = render_buf->m_start_primitive;
            
            // Call original function (would be: hooks[hook::animatable_generateprimitives].get_fastcall<void>(__this, object, a3, render_buf);)
            // For now, we'll skip the actual hook call as it requires the hook infrastructure
            
            if (prev_count <= render_buf->m_start_primitive) {
                for (int i = prev_count; i < render_buf->m_start_primitive; i++) {
                    auto mesh = &render_buf->m_out[i];
                    if (!mesh) continue;
                    
                    // Override material for invisible part
                    override_material(mesh, nullptr, ignorez_color);
                }
            }
        }
        
        // Render visible chams
        int prev_count = render_buf->m_start_primitive;
        
        // Call original function (would be: hooks[hook::animatable_generateprimitives].get_fastcall<void>(__this, object, a3, render_buf);)
        
        if (prev_count <= render_buf->m_start_primitive) {
            for (int i = prev_count; i < render_buf->m_start_primitive; i++) {
                auto mesh = &render_buf->m_out[i];
                if (!mesh) continue;
                
                // Override material for visible part
                override_material(mesh, nullptr, chams_color);
            }
        }
        
        return true;
    }
}

// Hook function for animatable_generateprimitives
// This would need to be integrated with your hook system
// Placeholder structure:
/*
void __fastcall hooked_animatable_generateprimitives(void* __this, c_scene_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf) {
    if (sdk::engine_client->is_in_game() && sdk::engine_client->is_connected()) {
        auto& visual = chams::get_chams();
        
        c_scene_animatable_object* animatable_obj = reinterpret_cast<c_scene_animatable_object*>(object);
        if (visual.render(__this, animatable_obj, a3, render_buf))
            return;
    }
    
    // Call original function
    // hooks[hook::animatable_generateprimitives].get_fastcall<void>(__this, object, a3, render_buf);
}
*/

// Chams polygon caching and rendering for overlay
static std::mutex chamsMutex;
static std::vector<Polygon_t> chamsPolys;

void utils::CacheChamsPoly() {
    // Performance constants
    constexpr float MAX_CHAMS_DISTANCE = 400.0f;  // Max distance to render chams
    constexpr float MAX_CHAMS_DISTANCE_SQ = MAX_CHAMS_DISTANCE * MAX_CHAMS_DISTANCE;
    constexpr size_t MAX_POLYGONS = 800;  // Maximum polygons to process
    constexpr float LOD_DISTANCE_THRESHOLD = 200.0f;  // Distance for LOD reduction
    
    while (true) {
        if (!globals::visuals::visuals || !globals::visuals::chams) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            continue;
        }

        std::vector<Polygon_t> temp_polygons;
        temp_polygons.reserve(MAX_POLYGONS);  // Pre-allocate to reduce allocations
        
        auto& playerlist = globals::instances::cachedplayers;
        if (playerlist.empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
            continue;
        }
        
        if (playerlist.empty() || !is_valid_address(globals::instances::lp.main.address)) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
            continue;
        }
        
        auto& local_player = playerlist[0];
        if (!local_player.main.address) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
            continue;
        }

        // Check if this is Phantom Forces for special handling
        bool isPhantomForces = false;
        if (is_valid_address(globals::instances::datamodel.address)) {
            try {
                uint64_t placeId = read<uint64_t>(globals::instances::datamodel.address + offsets::PlaceId);
                isPhantomForces = (placeId == 292439477ULL);
            } catch (...) {
                isPhantomForces = false;
            }
        }

        // Get local player position for distance calculations
        Vector3 local_pos;
        try {
            // Phantom Forces may need different HRP handling
            if (isPhantomForces && !is_valid_address(local_player.hrp.address)) {
                // Try to use camera position as fallback for Phantom Forces
                local_pos = globals::instances::camera.getPos();
            } else {
                local_pos = local_player.hrp.get_pos();
            }
        } catch (...) {
            try {
                // Fallback to camera position
                local_pos = globals::instances::camera.getPos();
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::microseconds(5000));
                continue;
            }
        }

        // Get screen dimensions for early culling
        auto screen_dimensions = globals::instances::visualengine.get_dimensions();
        if (screen_dimensions.x == 0 || screen_dimensions.y == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
            continue;
        }

        // Minimal sleep for faster rescan - always scan quickly
        size_t player_count = playerlist.size();
        auto sleep_time = player_count > 20 ? std::chrono::microseconds(50) : 
                         player_count > 10 ? std::chrono::microseconds(25) : 
                         std::chrono::microseconds(10);

        for (roblox::player& player : playerlist) {
            // Early exit if polygon limit reached
            if (temp_polygons.size() >= MAX_POLYGONS)
                break;

            if (player.main.address == local_player.main.address && !globals::visuals::selfesp) continue;

            // Team check
            if ((globals::visuals::teamcheck || globals::combat::teamcheck) && globals::is_teammate(player)) {
                continue;
            }

            // Distance-based culling - calculate distance early
            Vector3 player_pos;
            try {
                // Phantom Forces may need different HRP handling
                if (!is_valid_address(player.hrp.address)) {
                    if (isPhantomForces) {
                        // For Phantom Forces, try to use head position as fallback
                        if (is_valid_address(player.head.address)) {
                            player_pos = player.head.get_pos();
                        } else {
                            continue;
                        }
                    } else {
                        continue;
                    }
                } else {
                    player_pos = player.hrp.get_pos();
                }
            } catch (...) {
                continue;
            }

            float dist_sq = distance_sq(local_pos, player_pos);
            if (dist_sq > MAX_CHAMS_DISTANCE_SQ) {
                continue;  // Skip players beyond max distance
            }

            float distance = sqrtf(dist_sq);
            bool use_lod = distance > LOD_DISTANCE_THRESHOLD;

            // Early screen-space culling - check if player center is on screen
            Vector2 player_screen = roblox::worldtoscreen(player_pos);
            if (!esp_helpers::on_screen(player_screen) && distance > 100.0f) {
                continue;  // Skip if off-screen and far away
            }

            std::vector<roblox::instance> parts_chams;

            // Phantom Forces typically uses R15 (rigtype 1), but handle both
            // Auto-detect rigtype if not set properly
            int detected_rigtype = player.rigtype;
            if (isPhantomForces && detected_rigtype == 0) {
                // Check if R15 parts exist
                if (is_valid_address(player.lowertorso.address) || 
                    is_valid_address(player.leftupperarm.address)) {
                    detected_rigtype = 1;
                }
            }

            // LOD: Use fewer parts for distant players
            if (use_lod) {
                // Reduced part set for distant players
                switch (detected_rigtype) {
                case 0:
                    parts_chams = {
                        player.head,
                        player.uppertorso
                    };
                    break;
                case 1:
                    parts_chams = {
                        player.head,
                        player.uppertorso,
                        player.lowertorso
                    };
                    break;
                default:
                    // Fallback: try to use available parts
                    if (is_valid_address(player.head.address)) parts_chams.push_back(player.head);
                    if (is_valid_address(player.uppertorso.address)) parts_chams.push_back(player.uppertorso);
                    if (parts_chams.empty()) continue;
                    break;
                }
            } else {
                // Full part set for close players
                switch (detected_rigtype) {
                case 0:
                    parts_chams = {
                        player.head,
                        player.uppertorso,
                        player.leftfoot,
                        player.rightfoot,
                        player.lefthand,
                        player.righthand
                    };
                    break;
                case 1:
                    parts_chams = {
                        player.head,
                        player.uppertorso,
                        player.lowertorso,
                        player.leftupperarm,
                        player.rightupperarm,
                        player.leftlowerleg,
                        player.rightlowerleg,
                        player.leftfoot,
                        player.rightfoot,
                        player.leftupperleg,
                        player.rightupperleg,
                        player.leftlowerarm,
                        player.rightlowerarm,
                        player.lefthand,
                        player.righthand
                    };
                    break;
                default:
                    // Fallback: try to use available parts
                    if (is_valid_address(player.head.address)) parts_chams.push_back(player.head);
                    if (is_valid_address(player.uppertorso.address)) parts_chams.push_back(player.uppertorso);
                    if (is_valid_address(player.lowertorso.address)) parts_chams.push_back(player.lowertorso);
                    if (parts_chams.empty()) continue;
                    break;
                }
            }

            for (auto& part : parts_chams) {
                // Early exit if polygon limit reached
                if (temp_polygons.size() >= MAX_POLYGONS)
                    break;

                try {
                    if (!is_valid_address(part.address)) continue;
                    
                    // Early screen-space culling for part center
                    Vector3 part_pos = part.get_pos();
                    Vector2 part_screen = roblox::worldtoscreen(part_pos);
                    if (!esp_helpers::on_screen(part_screen) && distance > 150.0f) {
                        continue;  // Skip off-screen parts for distant players
                    }
                    
                    Vector3 part_size = part.get_part_size();
                    
                    Matrix3 part_rotation = part.read_cframe().rotation_matrix;

                    std::vector<Vector3> CubeVertices = GetCorners(
                        { 0, 0, 0 },
                        { part_size.x / 2, part_size.y / 2, part_size.z / 2 }
                    );

                    for (auto& vertex : CubeVertices) {
                        vertex = Rotate(vertex, part_rotation);
                        vertex = { vertex.x + part_pos.x, vertex.y + part_pos.y, vertex.z + part_pos.z };
                    }

                    std::vector<ImVec2> screen_vertices;
                    screen_vertices.reserve(8);
                    float total_depth = 0;
                    bool valid_polygon = true;
                    int on_screen_count = 0;

                    for (const auto& vertex : CubeVertices) {
                        Vector2 screen_pos = roblox::worldtoscreen(vertex);
                        if (esp_helpers::on_screen(screen_pos)) {
                            on_screen_count++;
                            screen_vertices.emplace_back(ImVec2(screen_pos.x, screen_pos.y));
                            total_depth += vertex.z;
                        } else {
                            // For distant players, allow some vertices off-screen
                            if (use_lod && on_screen_count >= 3) {
                                screen_vertices.emplace_back(ImVec2(screen_pos.x, screen_pos.y));
                                total_depth += vertex.z;
                            } else if (!use_lod) {
                                // For close players, require all vertices on screen
                                valid_polygon = false;
                                break;
                            }
                        }
                    }

                    if (!valid_polygon || screen_vertices.size() < 3)
                        continue;

                    std::vector<ImVec2> hull = convexHull(screen_vertices);

                    if (hull.size() >= 3) {
                        temp_polygons.emplace_back(Polygon_t{ hull, total_depth / CubeVertices.size() });
                    }
                }
                catch (...) {
                    continue;
                }
            }
        }

        if (!temp_polygons.empty()) {
            // Only sort if we have a reasonable number of polygons
            if (temp_polygons.size() < 1000) {
                std::sort(temp_polygons.begin(), temp_polygons.end(), [](const Polygon_t& a, const Polygon_t& b) {
                    return a.depth > b.depth;
                });
            }
            std::lock_guard<std::mutex> lock(chamsMutex);
            chamsPolys = std::move(temp_polygons);
        }
        else {
            std::lock_guard<std::mutex> lock(chamsMutex);
            chamsPolys.clear();
        }

        std::this_thread::sleep_for(sleep_time);
    }
}

void render_chams_enhanced() {
    if (!globals::visuals::chams || !globals::instances::draw)
        return;

    std::vector<Polygon_t> polygons_to_draw;
    {
        std::lock_guard<std::mutex> lock(chamsMutex);
        polygons_to_draw = chamsPolys;
    }

    if (polygons_to_draw.empty())
        return;

    globals::instances::draw->_FringeScale = 0.f;
    ImDrawListFlags oldFlags = globals::instances::draw->Flags;

    ImColor fillColor(
        (int)(globals::visuals::chamscolor[0] * 255),
        (int)(globals::visuals::chamscolor[1] * 255),
        (int)(globals::visuals::chamscolor[2] * 255),
        (int)(globals::visuals::chamscolor[3] * 255)
    );

    ImColor outlineColor(0, 0, 0, (int)(globals::visuals::chamscolor[3] * 255));

    for (const auto& polygon : polygons_to_draw) {
        if (polygon.vertices.size() < 3)
            continue;

        // Glow effect
        if ((*globals::visuals::chams_overlay_flags)[1]) {
            ImColor glow_color = fillColor;
            glow_color.Value.w *= 0.5f;
            globals::instances::draw->AddShadowConvexPoly(
                polygon.vertices.data(),
                polygon.vertices.size(),
                glow_color,
                10.f,
                ImVec2(0, 0)
            );
        }
        
        // Fill
        if ((*globals::visuals::chams_overlay_flags)[2]) {
            globals::instances::draw->AddConvexPolyFilled(
                polygon.vertices.data(),
                polygon.vertices.size(),
                fillColor
            );
        }
        
        // Outline
        if ((*globals::visuals::chams_overlay_flags)[0]) {
            globals::instances::draw->AddPolyline(
                polygon.vertices.data(),
                polygon.vertices.size(),
                outlineColor,
                true,
                1.5f
            );
        }
    }
    
    globals::instances::draw->Flags = oldFlags;
}

