#include "../visuals.h"
#include "../../../drawing/imgui/imgui.h"
#include <Windows.h>
#include <cmath>
#include <algorithm>
#include "../../../util/classes/classes.h"

namespace crosshair {
    void render() {
        if (!globals::visuals::crosshair_enabled)
            return;

        // Check if window is focused
        if (!globals::focused)
            return;

        static float fadeParam = 0.0f;
        static int lastStyle = globals::visuals::crosshair_styleIdx;
        static float currentSpeed = 0.0f;
        static float angle = 0.0f;
        static ImVec2 lastPos = ImVec2(0, 0);

        float time = ImGui::GetTime();
        float dt = ImGui::GetIO().DeltaTime;
        POINT pt;

        GetCursorPos(&pt);
        ScreenToClient((HWND)ImGui::GetMainViewport()->PlatformHandle, &pt);
        ImVec2 mpos{ (float)pt.x, (float)pt.y };

        // Check if we have a locked silent aim target
        bool hasSilentTarget = false;
        ImVec2 silentTargetPos = mpos;
        
        // Get the current silent aim target if available
        if (globals::combat::silentaim && globals::combat::silentaimkeybind.enabled && globals::focused) {
            if (globals::instances::cachedtarget.head.address != 0) {
                math::Vector2 targetScreen = roblox::worldtoscreen(globals::instances::cachedtarget.head.get_pos());
                if (targetScreen.x != -1.0f && targetScreen.y != -1.0f) {
                    hasSilentTarget = true;
                    silentTargetPos = ImVec2(targetScreen.x, targetScreen.y);
                }
            }
        }
        
        // Simple mouse position tracking with lock-on support
        float lerpSpeed = 5.0f;
        ImVec2 targetPos = hasSilentTarget ? silentTargetPos : mpos;
        mpos.x = lastPos.x + (targetPos.x - lastPos.x) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
        mpos.y = lastPos.y + (targetPos.y - lastPos.y) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
        lastPos = mpos;

        if (globals::visuals::crosshair_styleIdx != lastStyle) {
            fadeParam = 0.0f;
            lastStyle = globals::visuals::crosshair_styleIdx;
        }

        float targetSpeed = globals::visuals::crosshair_baseSpeed * 0.5f;
        switch (globals::visuals::crosshair_styleIdx) {
        case 0: // Static
            targetSpeed = globals::visuals::crosshair_baseSpeed * 0.5f;
            fadeParam = 0.0f;
            break;
        case 1: // Pulse
            fadeParam = std::clamp(fadeParam + dt / globals::visuals::crosshair_fadeDuration, 0.0f, 1.0f);
            targetSpeed = globals::visuals::crosshair_baseSpeed * (0.1f + 0.9f * (fadeParam * fadeParam));
            break;
        }

        const float accel = 5.0f;
        currentSpeed += (targetSpeed - currentSpeed) * std::clamp(accel * dt, 0.0f, 1.0f);
        angle += currentSpeed * (3.1415926f / 180.0f) * dt;
        if (angle > 6.2831853f) angle -= 6.2831853f;

        float showGap = globals::visuals::crosshair_gap;
        if (globals::visuals::crosshair_gapTween) {
            float raw = fmodf(time * globals::visuals::crosshair_gapSpeed, 2.0f);
            float e = raw < 1.0f ? (1.0f - (1.0f - raw) * (1.0f - raw)) : 1.0f - ((raw - 1.0f) * (raw - 1.0f));
            showGap = globals::visuals::crosshair_gap * e;
        }

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        
        // Set smallest pixel font
        ImFont* smallestPixelFont = ImGui::GetIO().Fonts->Fonts[0]; // Default font
        for (int i = 0; i < ImGui::GetIO().Fonts->Fonts.Size; i++) {
            ImFont* font = ImGui::GetIO().Fonts->Fonts[i];
            if (font && strstr(font->GetDebugName(), "smallest_pixel")) {
                smallestPixelFont = font;
                break;
            }
        }
        ImGui::PushFont(smallestPixelFont);
        
        ImU32 colLine = IM_COL32(
            (int)(globals::visuals::crosshair_color[0] * 255),
            (int)(globals::visuals::crosshair_color[1] * 255),
            (int)(globals::visuals::crosshair_color[2] * 255),
            (int)(globals::visuals::crosshair_color[3] * 255)
        );
        ImU32 colOut = IM_COL32(0, 0, 0, 255);
        float thick = globals::visuals::crosshair_thickness;

        for (int i = 0; i < 4; ++i) {
            float a = angle + i * 3.1415926f * 0.5f; // pi
            ImVec2 d{ cosf(a), sinf(a) };
            ImVec2 p0{ mpos.x + d.x * showGap, mpos.y + d.y * showGap };
            ImVec2 p1{ mpos.x + d.x * (showGap + globals::visuals::crosshair_size),
                    mpos.y + d.y * (showGap + globals::visuals::crosshair_size) };

            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    if (dx || dy)
                        dl->AddLine({ p0.x + dx, p0.y + dy },
                            { p1.x + dx, p1.y + dy },
                            colOut, thick);

            dl->AddLine(p0, p1, colLine, thick);
        }

        const char* txt = "Aimquette";
        const char* xyz = ".public";
        const char* aimquetteOnly = "Aimquette";

        float wordW = ImGui::CalcTextSize(txt).x;
        float xyzW = ImGui::CalcTextSize(xyz).x;
        float aimquetteOnlyW = ImGui::CalcTextSize(aimquetteOnly).x;
        float totalW = wordW + xyzW;

        // Always show full text
        bool showFull = true;
        float tweenProgress = 1.0f;
        float currentWordW = wordW;
        float currentXyzW = xyzW;
        float currentTotalW = currentWordW + currentXyzW;

        ImVec2 base{ mpos.x - currentTotalW * 0.5f, mpos.y + showGap + globals::visuals::crosshair_size + 4 };

        // Text directly follows crosshair position without lerp
        ImVec2 aimquettePos = base;

        ImU32 colW = IM_COL32(255, 255, 255, 255);
        ImU32 colM = IM_COL32(255, 100, 100, 255); // accent color of the .public :)

        // Always show full text with outline effect
        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        
        // Draw outline for "Aimquette"
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx || dy) {
                    dl->AddText(ImVec2(aimquettePos.x + dx, aimquettePos.y + dy), outlineColor, aimquetteOnly);
                }
            }
        }
        
        // Draw main text for "Aimquette"
        dl->AddText(aimquettePos, colW, aimquetteOnly);
        
        // Draw outline for ".public"
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx || dy) {
                    dl->AddText(ImVec2(aimquettePos.x + aimquetteOnlyW + dx, aimquettePos.y + dy), outlineColor, xyz);
                }
            }
        }
        
        // Draw main text for ".public"
        ImU32 accentFadeColor = IM_COL32(255, 100, 100, 255);
        dl->AddText(ImVec2(aimquettePos.x + aimquetteOnlyW, aimquettePos.y), accentFadeColor, xyz);
        
        // Restore default font
        ImGui::PopFont();
    }
}
