#include "../features.h"
#include "render.h"
#include <d3d11.h>  

#include <mmsystem.h>
#include "../../../engine/math/types/Math/Math.h"
#include "../../../overlay/imgui/imgui.h"
#include "../../../main.h"
#include "../../../../globals/globals.hpp"
#include "../../../../misc/Umodule/Umodule.hpp"
#include "font2.h"
#include "../../../overlay/overlay.hpp"


#include "image/johnpork.h"
#include "image/abyss.h"
#include "image/espimage.h"
#include "image/lebron.h"
#include "image/Egirl.h"
#include "image/Fulcrum.h"
#include "image/hunter.h"
#include "image/fatswampnigga.h"
#include "image/koda.h"

#pragma comment(lib, "d3dx11.lib")

#include "../../../overlay/textures/stb_image.h"

static ID3D11ShaderResourceView* box_textures[7] = { nullptr };
static bool textures_loaded = false;

ImTextureID LoadImageTexture(ID3D11Device* device, unsigned char* image_data, int image_size) {
    if (!device) return nullptr;
    
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(image_data, image_size, &width, &height, &channels, 4);
    if (!pixels) return nullptr;
    
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    
    ID3D11Texture2D* pTexture = nullptr;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    
    if (FAILED(device->CreateTexture2D(&desc, &subResource, &pTexture))) {
        stbi_image_free(pixels);
        return nullptr;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    ID3D11ShaderResourceView* srv = nullptr;
    if (FAILED(device->CreateShaderResourceView(pTexture, &srvDesc, &srv))) {
        pTexture->Release();
        stbi_image_free(pixels);
        return nullptr;
    }
    
    pTexture->Release();
    stbi_image_free(pixels);
    return (ImTextureID)srv;
}

void LoadBoxTextures() {
    if (textures_loaded) return;
    ID3D11Device* device = overlay::devicleoad();
    if (!device) {
        textures_loaded = false;
        return;
    }
    
    box_textures[0] = (ID3D11ShaderResourceView*)LoadImageTexture(device, johnpork, sizeof(johnpork));
    box_textures[1] = (ID3D11ShaderResourceView*)LoadImageTexture(device, abyss, sizeof(abyss));
    box_textures[2] = (ID3D11ShaderResourceView*)LoadImageTexture(device, LebronIMG, sizeof(LebronIMG));
    box_textures[3] = (ID3D11ShaderResourceView*)LoadImageTexture(device, EgirlImage, sizeof(EgirlImage));
    box_textures[4] = (ID3D11ShaderResourceView*)LoadImageTexture(device, FullCrumIMG, sizeof(FullCrumIMG));
    box_textures[5] = (ID3D11ShaderResourceView*)LoadImageTexture(device, esplol, sizeof(esplol));
    box_textures[6] = (ID3D11ShaderResourceView*)LoadImageTexture(device, HunterImage, sizeof(HunterImage));
    
    textures_loaded = true;
}


#define M_PI 3.14159265358979323846

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

inline static ImFont* verdana_12;

RBX::Vector3 Rotate(const RBX::Vector3& vec, const RBX::Matrix3x3& rotation_matrix) {

    const auto& row1 = rotation_matrix.data[0];
    const auto& row2 = rotation_matrix.data[3];
    const auto& row3 = rotation_matrix.data[6];

    float Sigma = vec.x * row1 + vec.y * rotation_matrix.data[1] + vec.z * rotation_matrix.data[2];
    float Sigma1 = vec.x * row2 + vec.y * rotation_matrix.data[4] + vec.z * rotation_matrix.data[5];
    float Sigma2 = vec.x * row3 + vec.y * rotation_matrix.data[7] + vec.z * rotation_matrix.data[8];

    return RBX::Vector3{ Sigma, Sigma1, Sigma2 };

}

ImVec2 RotatePointAroundCenter(const ImVec2& point, const ImVec2& center, float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);

    ImVec2 p = { point.x - center.x, point.y - center.y };

    float xnew = p.x * c - p.y * s;
    float ynew = p.x * s + p.y * c;

    p.x = xnew + center.x;
    p.y = ynew + center.y;

    return p;
}

void DrawFOV(ImVec2 center, float radius, ImU32 color) {
    ImGui::GetBackgroundDrawList()->AddCircleFilled(center, radius, color, 64);
}






void DrawSkeletonESP(const RBX::PlayerInstance& player, const RBX::Matrix4x4& viewMatrix, const RBX::Vector2& dimensions, ImDrawList* draw) {
    if (!player.address) return;

    std::vector<std::pair<RBX::Instance, RBX::Instance>> boneConnections;
    if (player.r15) {
        boneConnections = {
            {player.upperTorso, player.lowerTorso},
            {player.upperTorso, player.rightUpperArm},
            {player.rightUpperArm, player.rightLowerArm},
            {player.rightLowerArm, player.rightHand},
            {player.upperTorso, player.leftUpperArm},
            {player.leftUpperArm, player.leftLowerArm},
            {player.leftLowerArm, player.leftHand},
            {player.lowerTorso, player.rightUpperLeg},
            {player.rightUpperLeg, player.rightLowerLeg},
            {player.rightLowerLeg, player.rightFoot},
            {player.lowerTorso, player.leftUpperLeg},
            {player.leftUpperLeg, player.leftLowerLeg},
            {player.leftLowerLeg, player.leftFoot}
        };
    }
    else {
        boneConnections = {
            {player.rootPart, player.rootPart},
            {player.rootPart, player.head},
            {player.rootPart, player.rightUpperArm},
            {player.rightUpperArm, player.rightLowerArm},
            {player.rightLowerArm, player.rightHand},
            {player.rootPart, player.leftUpperArm},
            {player.leftUpperArm, player.leftLowerArm},
            {player.leftLowerArm, player.leftHand},
            {player.rootPart, player.rightUpperLeg},
            {player.rightUpperLeg, player.rightLowerLeg},
            {player.rightLowerLeg, player.rightFoot},
            {player.rootPart, player.leftUpperLeg},
            {player.leftUpperLeg, player.leftLowerLeg},
            {player.leftLowerLeg, player.leftFoot}
        };
    }

    const RBX::Vector3 upperTorsoOffset(0, 0.5f, 0);
    const RBX::Vector3 upperArmOffset(0, 0.4f, 0);
    const RBX::Vector3 spineOffset(0, 0.7f, 0);

    for (const auto& connection : boneConnections) {
        const auto& start = connection.first;
        const auto& end = connection.second;
        if (!start.address || !end.address) continue;

        RBX::Vector3 startPos = start.GetPosition();
        RBX::Vector3 endPos = end.GetPosition();

        if (!player.r15) {
            if (start == player.rootPart && end == player.rootPart) {
                startPos += RBX::Vector3(0, 0.3f, 0);
                endPos += spineOffset;
            }
            else if (start == player.rootPart) {
                if (end == player.rightUpperArm || end == player.leftUpperArm) startPos += spineOffset - RBX::Vector3(0, 0.2f, 0);
                else if (end == player.rightUpperLeg || end == player.leftUpperLeg) startPos += RBX::Vector3(0, 0.3f, 0);
                else if (end == player.head) startPos += spineOffset;
            }
        }
        else {
            if (start == player.upperTorso) startPos += upperTorsoOffset;
            if (end == player.upperTorso) endPos += upperTorsoOffset;
            if (start == player.leftUpperArm) startPos += upperArmOffset;
            if (end == player.leftUpperArm) endPos += upperArmOffset;
            if (start == player.rightUpperArm) startPos += upperArmOffset;
            if (end == player.rightUpperArm) endPos += upperArmOffset;
        }

        RBX::Vector2 startScreen = RBX::WorldToScreen(startPos, dimensions, viewMatrix);
        RBX::Vector2 endScreen = RBX::WorldToScreen(endPos, dimensions, viewMatrix);
        if (startScreen.x == -1 || startScreen.y == -1 || endScreen.x == -1 || endScreen.y == -1) continue;

        ImVec2 startPoint = ImVec2(floorf(startScreen.x) + 0.5f, floorf(startScreen.y) + 0.5f);
        ImVec2 endPoint = ImVec2(floorf(endScreen.x) + 0.5f, floorf(endScreen.y) + 0.5f);

        if (globals::skeletonOutline) draw->AddLine(startPoint, endPoint, IM_COL32(0, 0, 0, 255), globals::skeletonThickness + 1.0f);

        draw->AddLine(startPoint, endPoint, ImGui::ColorConvertFloat4ToU32(ImVec4(globals::skeleton_Color[0], globals::skeleton_Color[1], globals::skeleton_Color[2], globals::alpha)), globals::skeletonThickness);

    }
}

int current_armor = 0;
float shield_padding = 0.0f;
int current_flamearmor = 0;
float flame_padding = 0.0f;

float GetAnimatedGap(float baseGap, float amplitude, float frequency, float time) {
    return baseGap + amplitude * std::sin(frequency * time);
}
static std::vector< RBX::Vector3> GetCorners(const RBX::Vector3& partCF, const RBX::Vector3& partSize) {
    std::vector<RBX::Vector3> corners;

    for (int X = -1; X <= 1; X += 2) {
        for (int Y = -1; Y <= 1; Y += 2) {
            for (int Z = -1; Z <= 1; Z += 2) {
                RBX::Vector3 cornerPosition = {
                    partCF.x + partSize.x * X,
                    partCF.y + partSize.y * Y,
                    partCF.z + partSize.z * Z
                };
                corners.push_back(cornerPosition);
            }
        }
    }

    return corners;
}
inline float cross_product_2d(const ImVec2& O, const ImVec2& A, const ImVec2& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

inline float distance_sq(const ImVec2& A, const ImVec2& B) {
    return (A.x - B.x) * (A.x - B.x) + (A.y - B.y) * (A.y - B.y);
}

std::vector<ImVec2> convexHull(std::vector<ImVec2>& points) {
    if (points.size() <= 3) return points;

    auto it = std::min_element(points.begin(), points.end(), [](const ImVec2& a, const ImVec2& b) {
        return (a.y < b.y) || (a.y == b.y && a.x < b.x);
        });

    std::swap(points[0], *it);
    ImVec2 p0 = points[0];

    std::sort(points.begin() + 1, points.end(), [&p0](const ImVec2& a, const ImVec2& b) {
        float cross = cross_product_2d(p0, a, b);
        return (cross > 0) || (cross == 0 && distance_sq(p0, a) < distance_sq(p0, b));
        });

    std::vector<ImVec2> hull;
    hull.push_back(points[0]);
    hull.push_back(points[1]);

    for (size_t i = 2; i < points.size(); i++) {
        while (hull.size() > 1 && cross_product_2d(hull[hull.size() - 2], hull.back(), points[i]) <= 0) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }

    return hull;
}
static float dotProduct(const RBX::Vector3& vec1, const RBX::Vector3& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

static RBX::Vector3 crossProduct(const RBX::Vector3& vec1, const RBX::Vector3& vec2) {
    return {
        vec1.y * vec2.z - vec1.z * vec2.y,
        vec1.z * vec2.x - vec1.x * vec2.z,
        vec1.x * vec2.y - vec1.y * vec2.x
    };
}
static RBX::Vector3 normalize(const RBX::Vector3& vec) {
    float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    if (length != 0) {
        return { vec.x / length, vec.y / length, vec.z / length };
    }
    else {
        return vec;
    }
}




static auto AddVector(const RBX::Vector3& one, const RBX::Vector3& two) -> RBX::Vector3 {
    return { one.x + two.x, one.y + two.y, one.z + two.z };
}
RBX::Vector3 SubTractVector(RBX::Vector3 one, RBX::Vector3 two)
{
    return { one.x - two.x, one.y - two.y, one.z - two.z };
}
ImVec4 getTransparentDarkRedColor(float segmentY, float centerY, float radius) {

    float normalizedY = (segmentY - centerY) / radius;

    normalizedY = std::clamp(normalizedY, -1.0f, 1.0f);

    float alpha = 1.0f - std::abs(normalizedY);

    return ImVec4(0.5f, 0.0f, 0.0f, alpha);
}
 ImU32 color_from_globals() {
    return IM_COL32(
        (int)(globals::crosshair_color[0] * 255.0f),
        (int)(globals::crosshair_color[1] * 255.0f),
        (int)(globals::crosshair_color[2] * 255.0f),
        255
    );
}

 void RBX::CrosshairLoop() {
     ImDrawList* draw = ImGui::GetBackgroundDrawList();
     if (!draw) return;

     HWND hwnd = FindWindowA(nullptr, "Roblox");
     if (!hwnd) return;

     POINT cursor_pos;
     if (!GetCursorPos(&cursor_pos)) return;
     if (!ScreenToClient(hwnd, &cursor_pos)) return;

     RBX::Instance visualengine = globals::visualengine;
     RBX::Vector2 dimensions = visualengine.GetDimensions();
     RBX::Matrix4x4 viewMatrix = visualengine.GetViewMatrix();

     if (!globals::crosshair) return;

     static ImVec2 prev_crosshair_pos = ImVec2((float)cursor_pos.x, (float)cursor_pos.y);
     ImVec2 target_crosshair_pos;

     switch (globals::crosshair_origin) {
     case 0:
         target_crosshair_pos = ImVec2((float)cursor_pos.x, (float)cursor_pos.y);
         break;

     case 1:
         target_crosshair_pos = ImVec2(dimensions.x / 2.0f, (float)dimensions.y);
         break;

     case 2:
         target_crosshair_pos = ImVec2(dimensions.x / 2.0f, dimensions.y / 2.0f);
         break;

     case 3: {
         if (aimbot_target.address) {
             Vector3 pos3d = aimbot_target.rootPart.GetPosition();
             Vector2 pos2d = RBX::WorldToScreen(pos3d, dimensions, viewMatrix);
             target_crosshair_pos = ImVec2(pos2d.x, pos2d.y);
         }
         else if (aimbot_target2.address) {
             Vector3 pos3d = aimbot_target2.rootPart.GetPosition();
             Vector2 pos2d = RBX::WorldToScreen(pos3d, dimensions, viewMatrix);
             target_crosshair_pos = ImVec2(pos2d.x, pos2d.y);
         }
         else {
             target_crosshair_pos = ImVec2((float)cursor_pos.x, (float)cursor_pos.y);
         }
         break;
     }

     case 4: {
         auto tool = globals::players.GetLocalPlayer().FindFirstChildOfClass("Tool");
         if (tool.address) {
             Vector3 pos3d = tool.GetPosition();
             Vector2 pos2d = RBX::WorldToScreen(pos3d, dimensions, viewMatrix);
             target_crosshair_pos = ImVec2(dimensions.x / 2.0f, dimensions.y * 0.85f);
         }
         else {
             target_crosshair_pos = ImVec2((float)cursor_pos.x, (float)cursor_pos.y);
         }
         break;
     }

     default:
         target_crosshair_pos = ImVec2((float)cursor_pos.x, (float)cursor_pos.y);
         break;
     }

     float delta_time = ImGui::GetIO().DeltaTime;
     float lerp_speed = globals::crosshair_speed * delta_time;
     if (lerp_speed > 1.0f) lerp_speed = 1.0f;
     if (lerp_speed < 0.0f) lerp_speed = 0.0f;

     prev_crosshair_pos.x += (target_crosshair_pos.x - prev_crosshair_pos.x) * lerp_speed;
     prev_crosshair_pos.y += (target_crosshair_pos.y - prev_crosshair_pos.y) * lerp_speed;

     ImVec2 crosshair_pos = prev_crosshair_pos;

     float base_size = globals::crosshair_size;
     float gap = globals::crosshair_gap;
     float thickness = 0.6f;
     float outline_thickness = thickness + 1.4f;
     float time = ImGui::GetTime();
     float angle = globals::cashier_esp ? (time * globals::crosshair_spinspeed) : 0.0f;

     float pulse = globals::pulse_effect ? (sinf(time * 4.0f) * 0.5f + 1.0f) : 1.0f;
     float size = base_size * pulse;
     float adj_gap = gap * pulse;

     ImVec2 leftStart(-size, 0), leftEnd(-adj_gap, 0);
     ImVec2 rightStart(adj_gap, 0), rightEnd(size, 0);
     ImVec2 topStart(0, -size), topEnd(0, -adj_gap);
     ImVec2 bottomStart(0, adj_gap), bottomEnd(0, size);

     float cosA = cosf(angle), sinA = sinf(angle);
     auto rotate = [&](ImVec2 v) -> ImVec2 {
         return ImVec2(v.x * cosA - v.y * sinA + crosshair_pos.x, v.x * sinA + v.y * cosA + crosshair_pos.y);
         };

     uint32_t col = color_from_globals();
     uint32_t outline_col = IM_COL32(0, 0, 0, 255);
   if (globals::crosshair_outline) {
     draw->AddLine(rotate(leftStart), rotate(leftEnd), outline_col, outline_thickness);
     draw->AddLine(rotate(rightStart), rotate(rightEnd), outline_col, outline_thickness);
     draw->AddLine(rotate(topStart), rotate(topEnd), outline_col, outline_thickness);
     draw->AddLine(rotate(bottomStart), rotate(bottomEnd), outline_col, outline_thickness);
   }
     draw->AddLine(rotate(leftStart), rotate(leftEnd), col, thickness);
     draw->AddLine(rotate(rightStart), rotate(rightEnd), col, thickness);
     draw->AddLine(rotate(topStart), rotate(topEnd), col, thickness);
     draw->AddLine(rotate(bottomStart), rotate(bottomEnd), col, thickness);
     if (globals::vehicle_esp) {
         std::string full = "aether.gg";
         ImVec2 text_size = ImGui::CalcTextSize(full.c_str());
         float text_x = crosshair_pos.x - text_size.x / 2.0f + 14.0f;
         float text_y = crosshair_pos.y + base_size + 6.0f;

         int flash_int = (int)((sinf(time * 2.5f) * 0.5f + 0.5f) * 255.0f);
         Visualize.text_Voided(ImVec2(text_x, text_y), full, ImColor(flash_int, flash_int, flash_int, 255), verdana_12);
     }

 }



void RBX::initVisuals()
{

    static bool priority_set = false;
    if (!priority_set) {
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        priority_set = true;
    }


    ImDrawList* draw = ImGui::GetBackgroundDrawList();
 
    if (!draw)
        return;

    POINT cursor_pos;
    GetCursorPos(&cursor_pos);
    ScreenToClient(FindWindowA(0, ("roblox")), &cursor_pos);

    RBX::Instance visualengine = globals::visualengine;
    RBX::Vector2 dimensions = visualengine.GetDimensions();
    float screen_width = dimensions.x;
    float screen_height = dimensions.y;
    RBX::PlayerInstance localplayer = globals::localplayer;
    RBX::Instance localplayerHead = localplayer.head;

    static float rotation_offset = 0.0f;
    static float time_elapsed = 0.0f;

    if (globals::draw_triggerbot_fov) {

        ImVec2 center = ImVec2(cursor_pos.x, cursor_pos.y);
        float radius = globals::free_aim_fov;

        float filledRadius = radius - 1.0f;

        ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::FovColor[0], globals::FovColor[1], globals::FovColor[2], 0.5));
        draw->AddCircleFilled(center, filledRadius, sigma);

    }

    if (globals::free_aim_draw_fov) {

        ImVec2 center = ImVec2(cursor_pos.x, cursor_pos.y);
        float radius = globals::free_aim_fov;

        float filledRadius = radius - 1.0f;
        ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::FovColor[0], globals::FovColor[1], globals::FovColor[2], 0.5));
        draw->AddCircleFilled(center, filledRadius, sigma);


    }
    if (globals::autoparry) {
        draw->Flags = ImDrawListFlags_AntiAliasedLines;
    }

    for (RBX::PlayerInstance& player : globals::cached_players)
    {
        if (!globals::esp)
            return;
  
   
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        if (globals::autoparry) {
            draw->Flags = ImDrawListFlags_AntiAliasedLines;
        }

        RBX::Matrix4x4 viewMatrix = visualengine.GetViewMatrix();


        if (globals::esp_checks[0] && player.knockedOut.getBoolFromValue())
            continue;
        if (globals::esp_checks[1] && player.humanoid.GetHealth() <= 0)
            continue;
        if (globals::esp_checks[2] && player.ifGrabbed.address != 0)
            continue;
        RBX::Instance localPlayerTeam = localplayer.team;
        RBX::Instance local_team = localPlayerTeam;
        RBX::Instance other_team = player.team;
        RBX::Instance character = player.character;
        RBX::Instance humanoidRootPart = player.rootPart;
        if (globals::esp_checks[3] && local_team.address == other_team.address)
            continue;
        if (globals::esp_checks[3] && local_team.GetName() == other_team.GetName())
            continue;
        RBX::Instance humanoid = player.humanoid;
        if (!character.address || !humanoid.address)
            continue;

        bool is_aimbot_target =
            (aimbot_target.address != 0 && player.address == aimbot_target.address

                || player.address == aimbot_target2.address
                || globals::target.address != 0 && player.address == globals::target.address);
        if (globals::linkTarget) {
            aimbot_target2 = aimbot_target;
        }
        if (globals::onlytarget) {
            if (!is_aimbot_target)
                continue;
        }
        float distanceMagnitude = (globals::camera.GetCameraPosition() - player.rootPart.GetPosition()).magnitude();
        if (globals::distancecheck) {
            if (distanceMagnitude > globals::max_render_distance) {
                continue;
            }
        }

        float alpha_factor = 1.0f - (distanceMagnitude / globals::max_render_distance);
        alpha_factor = std::clamp(alpha_factor, 0.0f, 1.0f);

        RBX::Vector2 headPosition2D = RBX::WorldToScreen(player.head.GetPosition(), dimensions, viewMatrix);

        if (headPosition2D.x == -1 || headPosition2D.y == -1)
            continue;

        if (globals::offscreen_Check) {

            if (headPosition2D.x < 0 || headPosition2D.x >= screen_width || headPosition2D.y < 0 || headPosition2D.y >= screen_height) {
                continue;
            }
        }
        if (globals::tracer_esp) {
            RBX::Vector2 player_head_2d = RBX::WorldToScreen(player.head.GetPosition(), dimensions, viewMatrix);
            ImVec2 player_head_screen(player_head_2d.x, player_head_2d.y);

            if (player_head_2d.x != -1 && player_head_2d.y != -1) {
                ImVec2 origin_screen;

                switch (globals::TracerOrigin) {
                case 1: // Bottom-Center
                    origin_screen = ImVec2(dimensions.x / 2.0f, dimensions.y);
                    break;
                case 2: // Center
                    origin_screen = ImVec2(dimensions.x / 2.0f, dimensions.y / 2.0f);
                    break;
                case 3: // Top-Left
                    origin_screen = ImVec2(0.0f, 0.0f);
                    break;
                default: // Mouse Cursor (Default)
                    origin_screen = ImVec2(cursor_pos.x, cursor_pos.y);
                    break;
                }

                ImU32 tracer_color = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(globals::TracerColor[0], globals::TracerColor[1], globals::TracerColor[2], globals::alpha)
                );

                ImU32 outline_color = IM_COL32(0, 0, 0, static_cast<int>(globals::alpha * 255.0f));
                float tracer_thickness = globals::tracer_thickness;
                float outline_thickness = tracer_thickness + 2.0f;
                if (globals::soccer_ball_esp) {
                draw->AddLine(origin_screen, player_head_screen, outline_color, outline_thickness);
                }
                draw->AddLine(origin_screen, player_head_screen, tracer_color, tracer_thickness);
            }
        }
    





    
        if (globals::HeadDotEsp) {
            RBX::Vector3 headPosition = player.head.GetPosition();
            headPosition.y += globals::HeadDotOffset;
            RBX::Vector2 headScreenPos = RBX::WorldToScreen(headPosition, dimensions, viewMatrix);
            if (headScreenPos.x != -1 && headScreenPos.y != -1) {
                ImVec2 screenHead(headScreenPos.x, headScreenPos.y);
                ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::TracerColor[0], globals::TracerColor[1], globals::TracerColor[2], globals::alpha));
                if (globals::HeadDotMode == 1) {
                    draw->AddCircleFilled(screenHead, globals::HeadDotSize, color, globals::HeadDotSegments);
                }

                else if (globals::HeadDotMode == 0) {
                    draw->AddCircle(screenHead, globals::HeadDotSize, color, globals::HeadDotSegments, 1.5f);
                }
                else if (globals::HeadDotMode == 3) {
                    draw->AddCircleFilled(screenHead, globals::HeadDotSize, color, globals::HeadDotSegments);
                    draw->AddCircle(screenHead, globals::HeadDotSize, color, globals::HeadDotSegments, 1.5f);
                }
            }
        }


        ImVec2 top_left, bottom_right;
      
        RBX::Instance parts[] = {
            player.head, player.upperTorso, player.lowerTorso, player.leftFoot, player.rightFoot,
            player.leftUpperLeg, player.rightUpperLeg, player.rightLowerLeg, player.leftLowerLeg,
            player.leftHand, player.rightHand, player.leftUpperArm, player.rightUpperArm,
            player.leftLowerArm, player.rightLowerArm, player.rootPart
        };

        float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

        for (const auto& p : parts) {
            if (!p.address) continue;

            RBX::Vector3 s = p.GetSize(), pos = p.GetPosition();
            RBX::Matrix3x3 r = p.GetRotation();
            RBX::Vector3 halfSize = s * 0.5f;

            for (int j = 0; j < 8; j++) {
                RBX::Vector3 local = {
                    (j & 1) ? halfSize.x : -halfSize.x,
                    (j & 2) ? halfSize.y : -halfSize.y,
                    (j & 4) ? halfSize.z : -halfSize.z
                };

                RBX::Vector3 rotated = {
                    r.data[0] * local.x + r.data[1] * local.y + r.data[2] * local.z,
                    r.data[3] * local.x + r.data[4] * local.y + r.data[5] * local.z,
                    r.data[6] * local.x + r.data[7] * local.y + r.data[8] * local.z
                };

                RBX::Vector2 screen = RBX::WorldToScreen(rotated + pos, dimensions, viewMatrix);
                if (screen.x != -1 && screen.y != -1) {
                    minX = std::min(minX, screen.x);
                    minY = std::min(minY, screen.y);
                    maxX = std::max(maxX, screen.x);
                    maxY = std::max(maxY, screen.y);
                }
            }
        }

        if (minX < maxX && minY < maxY) {
            RBX::Vector2 center = { (minX + maxX) * 0.5f, (minY + maxY) * 0.5f };
            RBX::Vector2 halfSize = { (maxX - minX) * 0.5f, (maxY - minY) * 0.5f };

            top_left.x = minX;
            top_left.y = minY;
            bottom_right.x = maxX;
            bottom_right.y = maxY;

            RBX::Matrix3x3 humanoidRootPart_rotation_matrix = player.rootPart.GetRotation();
            RBX::Vector3 humanoidRootPart_rotation = { -humanoidRootPart_rotation_matrix.data[2] / 100, -humanoidRootPart_rotation_matrix.data[5] / 100, -humanoidRootPart_rotation_matrix.data[8] / 100 };
            RBX::Vector3 look_position_3D = AddVector(player.rootPart.GetPosition(), humanoidRootPart_rotation);
            RBX::Vector3 look_direction = normalize(humanoidRootPart_rotation);
            RBX::Vector3 side_vector1 = crossProduct({ 0.0f, 1.0f, 0.0f }, look_direction);

            if (globals::box_esp) {
                ImVec4 base_color;

                ImVec4 sigma = ImGui::ColorConvertU32ToFloat4(
                    ImGui::ColorConvertFloat4ToU32(
                        ImVec4(globals::BoxOutlineColor[0], globals::BoxOutlineColor[1], globals::BoxOutlineColor[2], globals::alpha)
                    )
                );

                if (is_aimbot_target) {

                    base_color = ImVec4(1.0f, 0.0f, 0.0f, globals::alpha);
                }
                else {

                    base_color = sigma;
                }

                ImU32 Base_color2 = ImGui::ColorConvertFloat4ToU32(base_color);
                ImVec2 pos = { top_left.x, top_left.y };
                ImVec2 size = { bottom_right.x - top_left.x, bottom_right.y - top_left.y };

                auto draw = ImGui::GetBackgroundDrawList();

                float glow_size = 34.0f;

                ImU32 glowboy = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));
                ImU32 glow_color = glowboy;

                ImVec2 glow_offset = { 0, 0 };
                if (globals::boxGlow)
                    draw->AddShadowRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), glow_color, globals::glow_size, glow_offset, ImDrawFlags_ShadowCutOutShapeBackground, 0.0f);

                if (globals::fill_box_render) {
                    LoadBoxTextures();
                    if (globals::image_type >= 0 && globals::image_type < 7 && box_textures[globals::image_type]) {
                        ImU32 image_color = IM_COL32(255, 255, 255, 255);
                        draw->AddImage((ImTextureID)box_textures[globals::image_type], pos, ImVec2(pos.x + size.x, pos.y + size.y), ImVec2(0, 0), ImVec2(1, 1), image_color);
                    }
                }

                switch (globals::box_type) {
                case 0:
                    Visualize.outlined_rect(pos, size, Base_color2, 0.0f);
                    break;

                case 1: {
                    ImU32 color = ImGui::GetColorU32(Base_color2);

                    std::vector<RBX::Vector3> CubeVertices = GetCorners(
                        SubTractVector(player.rootPart.GetPosition(), { 0, 0.5, 0 }),
                        { 1.5, 3.25, 1.5 }
                    );

                    for (int i = 0; i < 8; ++i) {
                        RBX::Vector3 relativeCorner = SubTractVector(CubeVertices[i], player.rootPart.GetPosition());
                        RBX::Vector3 rotatedCorner = {
                            dotProduct(relativeCorner, side_vector1),
                            relativeCorner.y,
                            -dotProduct(relativeCorner, look_direction)
                        };
                        CubeVertices[i] = AddVector(rotatedCorner, player.rootPart.GetPosition());
                    }

                    std::vector<RBX::Vector2> CubeScreenVertices;
                    bool valid = true;
                    for (int i = 0; i < 8; ++i) {
                        RBX::Vector2 screenPos = RBX::WorldToScreen(CubeVertices[i], dimensions, viewMatrix);
                        if (screenPos.x == -1) {
                            valid = false;
                            break;
                        }
                        CubeScreenVertices.push_back(screenPos);
                    }
                    if (!valid) return;

                    draw->AddLine(ImVec2(CubeScreenVertices[0].x, CubeScreenVertices[0].y),
                        ImVec2(CubeScreenVertices[1].x, CubeScreenVertices[1].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[1].x, CubeScreenVertices[1].y),
                        ImVec2(CubeScreenVertices[5].x, CubeScreenVertices[5].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[5].x, CubeScreenVertices[5].y),
                        ImVec2(CubeScreenVertices[4].x, CubeScreenVertices[4].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[4].x, CubeScreenVertices[4].y),
                        ImVec2(CubeScreenVertices[0].x, CubeScreenVertices[0].y), color);

                    draw->AddLine(ImVec2(CubeScreenVertices[0].x, CubeScreenVertices[0].y),
                        ImVec2(CubeScreenVertices[2].x, CubeScreenVertices[2].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[1].x, CubeScreenVertices[1].y),
                        ImVec2(CubeScreenVertices[3].x, CubeScreenVertices[3].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[5].x, CubeScreenVertices[5].y),
                        ImVec2(CubeScreenVertices[7].x, CubeScreenVertices[7].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[4].x, CubeScreenVertices[4].y),
                        ImVec2(CubeScreenVertices[6].x, CubeScreenVertices[6].y), color);

                    draw->AddLine(ImVec2(CubeScreenVertices[2].x, CubeScreenVertices[2].y),
                        ImVec2(CubeScreenVertices[3].x, CubeScreenVertices[3].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[3].x, CubeScreenVertices[3].y),
                        ImVec2(CubeScreenVertices[7].x, CubeScreenVertices[7].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[7].x, CubeScreenVertices[7].y),
                        ImVec2(CubeScreenVertices[6].x, CubeScreenVertices[6].y), color);
                    draw->AddLine(ImVec2(CubeScreenVertices[6].x, CubeScreenVertices[6].y),
                        ImVec2(CubeScreenVertices[2].x, CubeScreenVertices[2].y), color);
                    break;
                }

                case 2: {
                    ImU32 color = ImGui::GetColorU32(Base_color2);

                    RBX::Vector3 headPos = AddVector(player.rootPart.GetPosition(), { 0, 3, 0 });
                    RBX::Vector3 footPos = SubTractVector(player.rootPart.GetPosition(), { 0, 4, 0 });

                    RBX::Vector2 screenHead = RBX::WorldToScreen(headPos, dimensions, viewMatrix);
                    RBX::Vector2 screenFoot = RBX::WorldToScreen(footPos, dimensions, viewMatrix);

                    if (screenHead.x == -1 || screenFoot.x == -1)
                        return;

                    float height = screenFoot.y - screenHead.y;
                    float width = height / 2.0f;

                    ImVec2 topLeft(screenHead.x - width / 2, screenHead.y);
                    ImVec2 topRight(screenHead.x + width / 2, screenHead.y);
                    ImVec2 bottomLeft(screenHead.x - width / 2, screenFoot.y);
                    ImVec2 bottomRight(screenHead.x + width / 2, screenFoot.y);

                    draw->AddLine(topLeft, topRight, color);
                    draw->AddLine(topRight, bottomRight, color);
                    draw->AddLine(bottomRight, bottomLeft, color);
                    draw->AddLine(bottomLeft, topLeft, color);
                    break;
                }
                case 3: {
                    Visualize.cornered_rect(pos, size, Base_color2);
                }
                }

            }


        }

        if (globals::skeleton_esp) {
            DrawSkeletonESP(player, viewMatrix, dimensions, draw);

        }
        if (globals::TeamVisual) {
            auto team = player.team;
        }
        if (globals::shield_bar) {
            auto armor_obj = player.armor_obj;
            int current_armor = 0;
            if (armor_obj.address) {
                RBX::Instance cool;
                current_armor = player.shield;
            }
            ImVec2 shield_pos = { top_left.x, bottom_right.y + 1.f };
            ImVec2 shield_size = { bottom_right.x - top_left.x, 5.f };

            Visualize.shield_bar(200.f, current_armor, shield_pos, shield_size, alpha_factor);

            shield_padding = shield_size.y + 1.f;
        }

        if (globals::flame_bar) {
            auto flame_obj = player.flame_obj;
            if (flame_obj.address) {
                current_flamearmor = Umodule::read<int>(flame_obj.address + Offsets::Value);
            }
            ImVec2 flame_bar_pos = { top_left.x, bottom_right.y + shield_padding };
            ImVec2 flame_bar_size = { bottom_right.x - top_left.x, 6.f };
            Visualize.flame_bar(200.f, current_flamearmor, flame_bar_pos, flame_bar_size, alpha_factor);
            flame_padding = flame_bar_size.y + 1.f;
        }



        if (globals::dropped_items) {
            ImVec2 screenCenter = ImVec2(dimensions.x / 2.0f, dimensions.y / 2.0f);
            float arrowSize = globals::arrowSize;
            float edgePadding = 10.0f;
            float arrowDistance = dimensions.y / globals::arrowDistance;

            for (const auto& player : globals::cached_players) {
                if (!player.rootPart.address) continue;



                RBX::Vector2 screenPos = RBX::WorldToScreen(player.rootPart.GetPosition(), dimensions, viewMatrix);


                if (screenPos.x == -1 || screenPos.y == -1) {

                    RBX::Vector3 dir = (player.rootPart.GetPosition() - globals::camera.GetCameraPosition()).normalize();
                    float angle = atan2(dir.x, dir.z);
                    //  angle += M_PI;
                    ImVec2 projectedPos = ImVec2(
                        screenCenter.x + cos(angle) * arrowDistance,
                        screenCenter.y + sin(angle) * arrowDistance
                    );


                    projectedPos.x = std::clamp(projectedPos.x, edgePadding, dimensions.x - edgePadding);
                    projectedPos.y = std::clamp(projectedPos.y, edgePadding, dimensions.y - edgePadding);


                    ImVec2 arrowTip = projectedPos;
                    ImVec2 arrowLeft = ImVec2(
                        arrowTip.x + cos(angle + 2.5f) * arrowSize,
                        arrowTip.y + sin(angle + 2.5f) * arrowSize
                    );
                    ImVec2 arrowRight = ImVec2(
                        arrowTip.x + cos(angle - 2.5f) * arrowSize,
                        arrowTip.y + sin(angle - 2.5f) * arrowSize
                    );


                    ImU32 arrowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::box_color[0], globals::box_color[1], globals::box_color[2], globals::alpha));


                    draw->AddTriangleFilled(arrowTip, arrowLeft, arrowRight, arrowColor);
                }
            }
        }


        if (globals::health_bar) {
            float Health = player.health;
            float MaxHealth = player.maxhealth;

            float BoxHeight = maxY - minY;
            float BoxWidth = bottom_right.x - top_left.x;

            ImVec2 Pos;
            ImVec2 Size;
            float VerticalBarExtra = 5.0f;
            switch (globals::HealthBarSide) {
            case 0: // Left
                Pos = { top_left.x - 4.5f , top_left.y - (VerticalBarExtra / 2.0f) + 0.5f };
                Size = { 4.0f, BoxHeight + VerticalBarExtra - 1.0f };
                break;



            case 1: // Right
                Pos = { bottom_right.x + 6.9f, top_left.y - (VerticalBarExtra / 2.0f) };
                Size = { 4.0f, BoxHeight + VerticalBarExtra };
                break;


            }

            auto draw = ImGui::GetBackgroundDrawList();

            ImU32 GlowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));
            if (globals::enable_health_glow)
                draw->AddShadowRect(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), GlowColor, globals::glow_size, { 0, 0 }, ImDrawFlags_ShadowCutOutShapeBackground, 0.0f);

            Visualize.health_bar(MaxHealth, Health, Pos, Size, globals::alpha);

            if (globals::health_bar_text) {
                char HealthText[8];
                snprintf(HealthText, sizeof(HealthText), "%.0f", (Health / MaxHealth) * 100.0f);

                float TextX = Pos.x + Size.x - globals::health_x;
                float TextY = Pos.y + Size.y - (Size.y * (Health / MaxHealth)) - globals::health_y;

                Visualize.text_Voided(ImVec2(TextX, TextY), HealthText, IM_COL32(globals::healthBar_Color[0], globals::healthBar_Color[1], globals::healthBar_Color[2], 255), verdana_12);
            }
        }
        std::string playerState = "Standing";
        if (globals::flag_esp) {

            Vector3 velocity = player.rootPart.GetVelocity();



            float velocityMagnitude = velocity.magnitude();

            const float walkingThreshold = 16.0f;
            const float runningThreshold = 3.0f;

            const float idleThreshold = 0.0f;


            if (velocityMagnitude == idleThreshold) {
                playerState = "Idle";
            }

            if (velocityMagnitude < walkingThreshold) {
                if (velocityMagnitude == 0) {
                    playerState = "Idle";
                }
                else {
                    playerState = "Walking";
                }


            }
            else if (velocityMagnitude >= walkingThreshold && velocityMagnitude < runningThreshold) {
                playerState = "Running";
            }
            else if (velocityMagnitude >= runningThreshold) {
                playerState = "Running";
            }



            ImVec2 stateTextSize = ImGui::CalcTextSize(playerState.c_str());
            float shiftAmount;
            if (playerState == "Idle") {
                shiftAmount = 26.0f;
            }
            else {
                shiftAmount = 34.0f;
            }
            ImVec2 boxTopRight((top_left.x + bottom_right.x) / 2, top_left.y);
            ImVec2 distancePos = { boxTopRight.x + (bottom_right.x - top_left.x) / 2.f - stateTextSize.x / 2.f + shiftAmount, boxTopRight.y - 3.0f };



            ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::flagcolor[0], globals::flagcolor[1], globals::flagcolor[2], globals::alpha));
         


            if (globals::tool_outline) {
                Visualize.text_Voided(distancePos, playerState.c_str(), sigma, verdana_12);
            }
            else {
                Visualize.text(distancePos, playerState.c_str(), sigma, verdana_12);
            }
        }


        if (globals::TeamVisual) {



            std::string teamName;
            float shiftAmount;
            teamName = player.team.GetName();
            if (teamName.empty()) {
                teamName = "No Team";
            }
            if (teamName == "TRC") {
                teamName = "Red";
            }
            if (teamName == "TBC") {
                teamName = "Blue";
            }
            if (teamName == "TPC") {
                teamName = "Purple";
            }
            if (teamName == "TOC") {
                teamName = "Orange";
            } // simplifying them arsenal names
            ImVec2 teamNameTextSize = ImGui::CalcTextSize(teamName.c_str());
            if (playerState == "Idle") {
                shiftAmount = 34.0f;
            }
            else {
                shiftAmount = 34.0f;
            }

            float verticalShiftAmount = 13.0f;


            ImVec2 boxTopRight((top_left.x + bottom_right.x) / 2, top_left.y);


            ImVec2 distancePos = {
                boxTopRight.x + (bottom_right.x - top_left.x) / 2.f - teamNameTextSize.x / 2.f + shiftAmount,
                boxTopRight.y - 3.0f + verticalShiftAmount
            };

            ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::teamNameColor[0], globals::teamNameColor[1], globals::teamNameColor[2], globals::alpha));

            ImU32 teamNameColor = sigma;

            if (globals::tool_outline) {
                Visualize.text_Voided(distancePos, teamName.c_str(), teamNameColor, verdana_12);
            }
            else {
                Visualize.text(distancePos, teamName.c_str(), teamNameColor, verdana_12);
            }
        }

        if (globals::distance_esp) {
            float distance_magnitude = (globals::camera.GetCameraPosition() - player.rootPart.GetPosition()).magnitude();
            distance_magnitude = roundf(distance_magnitude * 100) / 100;

            char distance_str[32];
            sprintf(distance_str, "%.0f", distance_magnitude);

            ImVec2 distance_text_size = ImGui::CalcTextSize(distance_str);

            float tool_text_padding = 13.5f;
            ImVec2 distance_position;
            ImVec2 box_bottom_center((top_left.x + bottom_right.x) / 2, bottom_right.y);
            if (globals::tool_esp) {
                distance_position = { box_bottom_center.x - distance_text_size.x / 2.f, box_bottom_center.y + 4.0f };
            }
            else {
                distance_position = { box_bottom_center.x - distance_text_size.x / 2.f, box_bottom_center.y - 9.0f };
            }

            ImVec2 tool_text_position = { distance_position.x, distance_position.y + tool_text_padding };

            ImU32 distance_color = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::distance_color[0], globals::distance_color[1], globals::distance_color[2], globals::alpha));
            ImU32 glow_color = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));

            if (globals::distance_outline) {
                Visualize.text_Voided(tool_text_position, distance_str, distance_color, verdana_12);
            }
            else {
                Visualize.text(tool_text_position, distance_str, distance_color, verdana_12);
            }
        }
 


        std::vector<RBX::Instance> boneParts;

        if (!player.r15) {
            boneParts = {
                player.head,
                player.upperTorso,
                player.lowerTorso,
                player.rightUpperArm,
                player.rightLowerArm,
                player.rightHand,
                player.leftUpperArm,
                player.leftLowerArm,
                player.leftHand,
                player.rightUpperLeg,
                player.rightLowerLeg,
                player.rightFoot,
                player.leftUpperLeg,
                player.leftLowerLeg,
                player.leftFoot,
                player.rootPart
            };
        }
        else {
            boneParts = {
                player.head,
                player.upperTorso,
                player.lowerTorso,
                player.rightUpperArm,
                player.rightLowerArm,
                player.rightHand,
                player.leftUpperArm,
                player.leftLowerArm,
                player.leftHand,
                player.rightUpperLeg,
                player.rightLowerLeg,
                player.rightFoot,
                player.leftUpperLeg,
                player.leftLowerLeg,
                player.leftFoot
            };
        }
        std::vector<RBX::Instance> bonePartsA2;

        if (!player.r15) {
            bonePartsA2 = {

                player.upperTorso,
                player.lowerTorso,

            };
        }
        else {
            bonePartsA2 = {

                player.upperTorso,
                player.lowerTorso,

            };
        }
        if (globals::highlight) {
            for (const auto& bone : boneParts) {
                if (!bone.address)
                    continue;

                RBX::Vector3 position = bone.GetPosition();
                RBX::Vector3 size = bone.GetSize();
                RBX::Matrix3x3 rotation = bone.GetRotation();

                std::vector<RBX::Vector3> vertices = GetCorners({ 0, 0, 0 }, { size.x / 2, size.y / 2, size.z / 2 });

                for (auto& vertex : vertices) {
                    vertex = Rotate(vertex, rotation);
                    vertex = { vertex.x + position.x, vertex.y + position.y, vertex.z + position.z };
                }

                std::vector<ImVec2> screenCoords;
                for (const auto& vertex : vertices) {
                    RBX::Vector2 screenPos = RBX::WorldToScreen(vertex, dimensions, viewMatrix);
                    if (screenPos.x == -1 || screenPos.y == -1) {
                        screenCoords.clear();
                        break;
                    }
                    screenCoords.push_back(ImVec2(screenPos.x, screenPos.y));
                }

                if (screenCoords.size() < 3)
                    continue;

                auto hull = convexHull(screenCoords);

                ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    globals::HighLight_color[0],
                    globals::HighLight_color[1],
                    globals::HighLight_color[2],
                    1.0f
                ));

                draw->AddConvexPolyFilled(hull.data(), hull.size(), color);
                if (globals::chamsglow) {
             
                ImVec2 topLeft = hull[0];
                ImVec2 bottomRight = hull[0];
                for (const auto& p : hull) {
                    topLeft.x = std::min(topLeft.x, p.x);
                    topLeft.y = std::min(topLeft.y, p.y);
                    bottomRight.x = std::max(bottomRight.x, p.x);
                    bottomRight.y = std::max(bottomRight.y, p.y);
                }

                ImColor glowColor = ImColor(
                    globals::HighLight_color[0],
                    globals::HighLight_color[1],
                    globals::HighLight_color[2],
                    1.0f
                );

                draw->AddShadowRect(
                    topLeft,
                    bottomRight,
                    glowColor,
                    globals::chamsGlowSize,
                    ImVec2(0, 0),
                    ImDrawFlags_ShadowCutOutShapeBackground,
                    2.0f
                );
                }
            }
        }

        if (globals::chams) {
            for (const auto& bone : boneParts) {
                if (!bone.address)
                    continue;

                RBX::Vector3 position = bone.GetPosition();
                RBX::Vector3 size = bone.GetSize();
                RBX::Matrix3x3 rotation = bone.GetRotation();

                std::vector<RBX::Vector3> vertices = GetCorners({ 0, 0, 0 }, { size.x / 2, size.y / 2, size.z / 2 });

                for (auto& vertex : vertices) {
                    vertex = Rotate(vertex, rotation);
                    vertex = { vertex.x + position.x, vertex.y + position.y, vertex.z + position.z };
                }

                std::vector<ImVec2> screenCoords;
                for (const auto& vertex : vertices) {
                    RBX::Vector2 screenPos = RBX::WorldToScreen(vertex, dimensions, viewMatrix);
                    if (screenPos.x == -1 || screenPos.y == -1) {
                        screenCoords.clear();
                        break;
                    }
                    screenCoords.push_back(ImVec2(screenPos.x, screenPos.y));
                }

                if (screenCoords.size() < 3)
                    continue;

                ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_4[0], globals::color_4[1], globals::color_4[2], 0.4f));
                auto hull = convexHull(screenCoords);
                draw->AddConvexPolyFilled(hull.data(), hull.size(), color);
                if (globals::chamsglow) {

                    ImVec2 topLeft = hull[0];
                    ImVec2 bottomRight = hull[0];
                    for (const auto& p : hull) {
                        topLeft.x = std::min(topLeft.x, p.x);
                        topLeft.y = std::min(topLeft.y, p.y);
                        bottomRight.x = std::max(bottomRight.x, p.x);
                        bottomRight.y = std::max(bottomRight.y, p.y);
                    }

                    ImColor glowColor = ImColor(
                        globals::color_4[0],
                        globals::color_4[1],
                        globals::color_4[2],
                        1.0f
                    );

                    draw->AddShadowRect(
                        topLeft,
                        bottomRight,
                        glowColor,
                        globals::chamsGlowSize,
                        ImVec2(0, 0),
                        ImDrawFlags_ShadowCutOutShapeBackground,
                        2.0f
                    );
                }
            }
        }
        if (globals::FrameChams) {
            for (const auto& bone : boneParts) {
                if (!bone.address)
                    continue;

                RBX::Vector3 position = bone.GetPosition();
                RBX::Vector3 size = bone.GetSize();
                RBX::Matrix3x3 rotation = bone.GetRotation();

                RBX::Vector3 half = { size.x / 2, size.y / 2, size.z / 2 };

                std::vector<std::pair<RBX::Vector3, RBX::Vector3>> outlineLines = {
                    { { -half.x,  half.y, -half.z }, {  half.x,  half.y, -half.z } },
                    { {  half.x,  half.y, -half.z }, {  half.x,  half.y,  half.z } },
                    { {  half.x,  half.y,  half.z }, { -half.x,  half.y,  half.z } },
                    { { -half.x,  half.y,  half.z }, { -half.x,  half.y, -half.z } },

                    { { -half.x, -half.y, -half.z }, {  half.x, -half.y, -half.z } },
                    { {  half.x, -half.y, -half.z }, {  half.x, -half.y,  half.z } },
                    { {  half.x, -half.y,  half.z }, { -half.x, -half.y,  half.z } },
                    { { -half.x, -half.y,  half.z }, { -half.x, -half.y, -half.z } },

                    { { -half.x, -half.y, -half.z }, { -half.x,  half.y, -half.z } },
                    { {  half.x, -half.y, -half.z }, {  half.x,  half.y, -half.z } },
                    { {  half.x, -half.y,  half.z }, {  half.x,  half.y,  half.z } },
                    { { -half.x, -half.y,  half.z }, { -half.x,  half.y,  half.z } }
                };

                ImU32 colorMain = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    globals::color_4[0],
                    globals::color_4[1],
                    globals::color_4[2],
                    1.0f
                ));

                ImU32 colorOutline = IM_COL32(0, 0, 0, 255);

                float thicknessOutline = 3.0f;
                float thicknessMain = 1.0f;

                std::vector<ImVec2> screenPoints;

                for (const auto& [startLocal, endLocal] : outlineLines) {
                    RBX::Vector3 startWorld = Rotate(startLocal, rotation);
                    RBX::Vector3 endWorld = Rotate(endLocal, rotation);

                    startWorld = { startWorld.x + position.x, startWorld.y + position.y, startWorld.z + position.z };
                    endWorld = { endWorld.x + position.x, endWorld.y + position.y, endWorld.z + position.z };

                    RBX::Vector2 startScreen = RBX::WorldToScreen(startWorld, dimensions, viewMatrix);
                    RBX::Vector2 endScreen = RBX::WorldToScreen(endWorld, dimensions, viewMatrix);

                    if (startScreen.x == -1 || startScreen.y == -1 || endScreen.x == -1 || endScreen.y == -1)
                        continue;

                    ImVec2 p1(startScreen.x, startScreen.y);
                    ImVec2 p2(endScreen.x, endScreen.y);

                    screenPoints.push_back(p1);
                    screenPoints.push_back(p2);

                    if (globals::FrameOutline)
                        draw->AddLine(p1, p2, colorOutline, thicknessOutline);

                    draw->AddLine(p1, p2, colorMain, thicknessMain);
                }
                if (globals::chamsglow) {
                if (!screenPoints.empty()) {
                    ImVec2 topLeft = screenPoints[0];
                    ImVec2 bottomRight = screenPoints[0];
                    for (const auto& p : screenPoints) {
                        topLeft.x = std::min(topLeft.x, p.x);
                        topLeft.y = std::min(topLeft.y, p.y);
                        bottomRight.x = std::max(bottomRight.x, p.x);
                        bottomRight.y = std::max(bottomRight.y, p.y);
                    }

                    ImColor glowColor = ImColor(
                        globals::color_4[0],
                        globals::color_4[1],
                        globals::color_4[2],
                        1.0f
                    );

                    draw->AddShadowRect(
                        topLeft,
                        bottomRight,
                        glowColor,
                        globals::chamsGlowSize,
                        ImVec2(0, 0),
                        ImDrawFlags_ShadowCutOutShapeBackground,
                        2.0f
                    );
                }
                }
            }
        }

        if (globals::WireFrameChams) {
            for (const auto& bone : boneParts) {
                if (!bone.address)
                    continue;

                RBX::Vector3 position = bone.GetPosition();
                RBX::Vector3 size = bone.GetSize();
                RBX::Matrix3x3 rotation = bone.GetRotation();

                int circleSegments = 24;
                int verticalSegments = 6;

                ImU32 colorMain = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    globals::color_4[0],
                    globals::color_4[1],
                    globals::color_4[2],
                    1.0f
                ));

                ImU32 colorOutline = IM_COL32(0, 0, 0, 255);
                float thicknessOutline = 2.0f;
                float thicknessMain = 1.0f;

                float radiusX = size.x / 2.0f;
                float radiusY = size.y / 2.0f;
                float radiusZ = size.z / 2.0f;

                bool isCylindrical = (fabs(radiusX - radiusZ) < 0.01f && radiusX < radiusY); //  round base
                bool isSpherical = (fabs(radiusX - radiusY) < 0.01f && fabs(radiusX - radiusZ) < 0.01f);

                if (isSpherical || isCylindrical) {
                    for (int i = 0; i <= verticalSegments; ++i) {
                        float t = (float)i / verticalSegments;
                        float y = -radiusY + t * size.y;

                        std::vector<ImVec2> ringPoints;

                        for (int j = 0; j <= circleSegments; ++j) {
                            float angle = j * 2.0f * 3.14159265f / circleSegments;
                            float x = cosf(angle) * radiusX;
                            float z = sinf(angle) * radiusZ;

                            RBX::Vector3 local = { x, y, z };
                            RBX::Vector3 rotated = Rotate(local, rotation);

                            rotated.x += position.x;
                            rotated.y += position.y;
                            rotated.z += position.z;

                            RBX::Vector2 screen = RBX::WorldToScreen(rotated, dimensions, viewMatrix);
                            if (screen.x == -1 || screen.y == -1) continue;

                            ringPoints.push_back(ImVec2(screen.x, screen.y));
                        }

                        for (size_t k = 0; k + 1 < ringPoints.size(); ++k) {
                            if (globals::FrameOutline)
                                draw->AddLine(ringPoints[k], ringPoints[k + 1], colorOutline, thicknessOutline);
                            draw->AddLine(ringPoints[k], ringPoints[k + 1], colorMain, thicknessMain);
                        }
                    }

                    for (int j = 0; j <= circleSegments; ++j) {
                        float angle = j * 2.0f * 3.14159265f / circleSegments;
                        float x = cosf(angle) * radiusX;
                        float z = sinf(angle) * radiusZ;

                        RBX::Vector3 bottom = Rotate({ x, -radiusY, z }, rotation);
                        RBX::Vector3 top = Rotate({ x, +radiusY, z }, rotation);

                        bottom.x += position.x; bottom.y += position.y; bottom.z += position.z;
                        top.x += position.x; top.y += position.y; top.z += position.z;

                        RBX::Vector2 screen1 = RBX::WorldToScreen(bottom, dimensions, viewMatrix);
                        RBX::Vector2 screen2 = RBX::WorldToScreen(top, dimensions, viewMatrix);

                        if (screen1.x == -1 || screen1.y == -1 || screen2.x == -1 || screen2.y == -1)
                            continue;

                        ImVec2 p1(screen1.x, screen1.y);
                        ImVec2 p2(screen2.x, screen2.y);
                        if (globals::FrameOutline)
                            draw->AddLine(p1, p2, colorOutline, thicknessOutline);
                        draw->AddLine(p1, p2, colorMain, thicknessMain);
                    }
                }
                else {

                    RBX::Vector3 half = { size.x / 2, size.y / 2, size.z / 2 };
                    std::vector<std::pair<RBX::Vector3, RBX::Vector3>> outlineLines = {
                        { { -half.x,  half.y, -half.z }, {  half.x,  half.y, -half.z } },
                        { {  half.x,  half.y, -half.z }, {  half.x,  half.y,  half.z } },
                        { {  half.x,  half.y,  half.z }, { -half.x,  half.y,  half.z } },
                        { { -half.x,  half.y,  half.z }, { -half.x,  half.y, -half.z } },

                        { { -half.x, -half.y, -half.z }, {  half.x, -half.y, -half.z } },
                        { {  half.x, -half.y, -half.z }, {  half.x, -half.y,  half.z } },
                        { {  half.x, -half.y,  half.z }, { -half.x, -half.y,  half.z } },
                        { { -half.x, -half.y,  half.z }, { -half.x, -half.y, -half.z } },

                        { { -half.x, -half.y, -half.z }, { -half.x,  half.y, -half.z } },
                        { {  half.x, -half.y, -half.z }, {  half.x,  half.y, -half.z } },
                        { {  half.x, -half.y,  half.z }, {  half.x,  half.y,  half.z } },
                        { { -half.x, -half.y,  half.z }, { -half.x,  half.y,  half.z } }
                    };

                    for (const auto& [startLocal, endLocal] : outlineLines) {
                        RBX::Vector3 startWorld = Rotate(startLocal, rotation);
                        RBX::Vector3 endWorld = Rotate(endLocal, rotation);

                        startWorld.x += position.x;
                        startWorld.y += position.y;
                        startWorld.z += position.z;

                        endWorld.x += position.x;
                        endWorld.y += position.y;
                        endWorld.z += position.z;

                        RBX::Vector2 startScreen = RBX::WorldToScreen(startWorld, dimensions, viewMatrix);
                        RBX::Vector2 endScreen = RBX::WorldToScreen(endWorld, dimensions, viewMatrix);

                        if (startScreen.x == -1 || startScreen.y == -1 || endScreen.x == -1 || endScreen.y == -1)
                            continue;

                        ImVec2 p1(startScreen.x, startScreen.y);
                        ImVec2 p2(endScreen.x, endScreen.y);

                        if (globals::FrameOutline)
                            draw->AddLine(p1, p2, colorOutline, thicknessOutline);
                        draw->AddLine(p1, p2, colorMain, thicknessMain);
                    }
                }
            }
        }

        if (globals::PlayerGlow) {
            for (const auto& bone : bonePartsA2) {
                if (!bone.address)
                    continue;

                RBX::Vector3 position = bone.GetPosition();
                RBX::Vector2 screen = RBX::WorldToScreen(position, dimensions, viewMatrix);
                if (screen.x == -1 || screen.y == -1)
                    continue;

                ImVec2 center(screen.x, screen.y);

                float glow_box_size = 1.0f; // smaller = tighter glow core
                float half = glow_box_size * 0.5f;

                ImVec2 topLeft(center.x - half, center.y - half);
                ImVec2 bottomRight(center.x + half, center.y + half);

                ImU32 glowboy = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));
                ImU32 glow_color = glowboy;
                ImVec2 glow_offset = { 0, 0 };

                draw->AddShadowRect(
                    topLeft,
                    bottomRight,
                    glow_color,
                    100,
                    { 0, 0 },
                    ImDrawFlags_ShadowCutOutShapeBackground,
                    2.0f
                );
            }
        }




        if (globals::tool_esp) {
            RBX::Instance equippedTool = player.currentTool;
            std::string toolName = "None";

            if (equippedTool.address) {
                toolName = player.currentToolName;
            }

            ImVec2 toolTextSize = ImGui::CalcTextSize(toolName.c_str());



            ImVec2 boxBottomCenter((top_left.x + bottom_right.x) / 2, bottom_right.y);

            ImVec2 distancePos = { boxBottomCenter.x - toolTextSize.x / 2.f, boxBottomCenter.y + 3.0f };
            ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_2[0], globals::color_2[1], globals::color_2[2], globals::alpha));
            ImU32 toolColor = sigma;
            ImU32 glowboy = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));
            ImU32 glow_color = glowboy;


            ImVec2 glow_offset = { 0, 0 };


            //        draw->AddShadowRect(distancePos, ImVec2(distancePos.x + toolTextSize.x, distancePos.y + toolTextSize.y), glow_color, globals::glow_size, glow_offset, ImDrawFlags_ShadowCutOutShapeBackground, 0.0f);

            if (globals::tool_outline) {
                Visualize.text_Voided(distancePos, toolName.c_str(), toolColor, verdana_12);
            }
            else {
                Visualize.text(distancePos, toolName.c_str(), toolColor, verdana_12);
            }
        }
        if (globals::name_esp) {

            std::string player_name = player.name;

            auto top_text_sz = ImGui::CalcTextSize(player_name.c_str());

            ImVec2 box_top_center((top_left.x + bottom_right.x) / 2, top_left.y);

            ImVec2 text_position = { box_top_center.x - top_text_sz.x / 2.f, box_top_center.y - top_text_sz.y - 3.5f };
            ImU32 glowboy = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::color_1[0], globals::color_1[1], globals::color_1[2], globals::glow_opacity));
            ImU32 glow_color = glowboy;

            ImVec2 glow_offset = { 0, 0 };

            ImU32 sigma = ImGui::ColorConvertFloat4ToU32(ImVec4(globals::name_color[0], globals::name_color[1], globals::name_color[2], globals::alpha));
            ImU32 name_color = sigma;
            if (globals::name_outline) {
                Visualize.text_Voided(text_position, player_name.c_str(), name_color, verdana_12);
            }
            else {
                Visualize.text(text_position, player_name.c_str(), name_color, verdana_12);
            }

        }

    }
}