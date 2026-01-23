#include <Windows.h>
#include <thread>
#include <random>
#include <vector>
#include <immintrin.h>
#include <cmath>
#include <future>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")


#include "../features.h"
#include "../../../main.h"
#include "../../../engine/math/types/Math/Math.h"
#include "../../../../globals/globals.hpp"
#include "../../../../misc/output_system/output/output.hpp"
#define M_PI 3.14159265358979323846

RBX::Instance getFreeAimClosestPart(RBX::PlayerInstance& player, const POINT& cursor_point) {

    std::vector<RBX::Instance> parts = {
        player.head, player.rootPart, player.upperTorso, player.lowerTorso,
        player.leftUpperLeg, player.leftFoot, player.rightFoot,
        player.leftUpperArm, player.leftHand, player.rightUpperArm, player.rightHand,
    };

    RBX::Vector2 dimensions = globals::visualengine.GetDimensions();
    RBX::Matrix4x4 view_matrix = globals::visualengine.GetViewMatrix();
    RBX::Vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
    float min_distance = FLT_MAX;
    RBX::Instance closest_part;

    for (size_t i = 0; i < parts.size(); ++i) {
        if (!parts[i].address) continue;

      
        RBX::Vector3 part_position = parts[i].GetPosition();

        RBX::Vector2 part_screen_position = RBX::WorldToScreen(part_position, dimensions, view_matrix);

    
        float distance = (part_screen_position - cursor).getMagnitude();

      
        if (distance < min_distance) {
            min_distance = distance;
            closest_part = parts[i];
        }
    }

    return closest_part;
}
RBX::Instance getClosestPoint(RBX::PlayerInstance& player, const POINT& cursor_point) {
    RBX::Vector2 dimensions = globals::visualengine.GetDimensions();
    RBX::Matrix4x4 view_matrix = globals::visualengine.GetViewMatrix();
    RBX::Vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
    std::vector<RBX::Instance> parts = {
      player.head, player.rootPart, player.upperTorso, player.lowerTorso,
      player.leftUpperLeg, player.leftFoot, player.rightFoot,
      player.leftUpperArm, player.leftHand, player.rightUpperArm, player.rightHand,
    };
    float min_distance = FLT_MAX;
    RBX::Instance closest_point;

    for (const auto& part : parts) {
        if (!part.address) continue;

      
        RBX::Vector3 part_position = part.GetPosition();

    
        RBX::Vector2 part_screen_position = RBX::WorldToScreen(part_position, dimensions, view_matrix);

 
        float distance = (part_screen_position - cursor).getMagnitude();

      
        if (distance < min_distance) {
            min_distance = distance;
            closest_point.GetPosition() = part_position;
        }
    }

    return closest_point;
}


static RBX::Vector3 Recalculate_Velocity(RBX::PlayerInstance player)
{
   
    RBX::Vector3 old_Position = player.rootPart.GetPosition();
    std::this_thread::sleep_for(std::chrono::milliseconds(115));
    return (player.rootPart.GetPosition() - old_Position) / 0.115;
}

static RBX::PlayerInstance getClosestPlayerFromCursor() {
  
    POINT cursor_point;
    GetCursorPos(&cursor_point);
    ScreenToClient(FindWindowA(0, ("Roblox")), &cursor_point);

    RBX::Vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
    std::vector<RBX::PlayerInstance>& cached_players = globals::cached_players;
    RBX::PlayerInstance closestPlayer{};
    int shortestDistance = 9e9;

    RBX::PlayerInstance localPlayer = globals::localplayer;
    RBX::Instance localPlayerTeam = localPlayer.team;

    RBX::Vector2 dimensions = globals::visualengine.GetDimensions();
    RBX::Matrix4x4 viewmatrix = globals::visualengine.GetViewMatrix();

    for (RBX::PlayerInstance& player : cached_players) {
        if (player.address == localPlayer.address || !player.character.address || !player.humanoid.address)
            continue;

        bool knockedCheck = globals::silent_aim_checks[0];
        bool deadCheck = globals::silent_aim_checks[1];
        bool grabbedCheck = globals::silent_aim_checks[2];
        bool teamCheck = globals::silent_aim_checks[3];

        if (knockedCheck && player.knockedOut.getBoolFromValue())
            continue;

        if (deadCheck && player.humanoid.GetHealth() <= 0)
            continue;

        if (grabbedCheck && player.ifGrabbed.address != 0)
            continue;

        if (teamCheck && player.team.address == localPlayerTeam.address)
            continue;

        RBX::Instance part = player.rootPart;
        RBX::Vector3 partPosition = part.GetPosition();
        RBX::Vector2 partPositionOnScreen = RBX::WorldToScreen(partPosition, dimensions, viewmatrix);

        float distance_from_cursor = (partPositionOnScreen - cursor).getMagnitude();
        if (shortestDistance > distance_from_cursor) {
            closestPlayer = player;
            shortestDistance = distance_from_cursor;
        }
    }

    return closestPlayer;
}

static bool isWithinFOV(const RBX::Vector3& hit_position_3D) {
    POINT cursor_point;
    GetCursorPos(&cursor_point);
    ScreenToClient(FindWindowA(0, ("Roblox")), &cursor_point);

    auto cursor_pos_x = cursor_point.x;
    auto cursor_pos_y = cursor_point.y;

    RBX::Instance visualengine = globals::visualengine;
    RBX::Vector2 screen_dimensions = visualengine.GetDimensions();
    RBX::Vector2 hit_position_2D = RBX::WorldToScreen(hit_position_3D, screen_dimensions, visualengine.GetViewMatrix());

    float magnitude = (hit_position_2D - RBX::Vector2{ static_cast<float>(cursor_pos_x), static_cast<float>(cursor_pos_y) }).getMagnitude();
    return (magnitude <= globals::free_aim_fov);
}

static void run(RBX::PlayerInstance player, POINT cursor_point)
{
    if ((globals::silent_aim_checks[0] && player.knockedOut.getBoolFromValue()) ||
        (globals::silent_aim_checks[1] && player.ifGrabbed.address != 0) ||
        (globals::silent_aim_checks[2] && player.team.address == globals::localplayer.team.address))
        return;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    if (dist(gen) > globals::hit_chance)
        return;
    RBX::Instance hitbox;
    if (globals::free_aim_closest_part) {
        hitbox = globals::font_type == 0 ? getFreeAimClosestPart(player, cursor_point)
            : getClosestPoint(player, cursor_point);
    }
    else {
        static RBX::Instance RBX::PlayerInstance::* parts[] = {
            &RBX::PlayerInstance::head, &RBX::PlayerInstance::rootPart, &RBX::PlayerInstance::upperTorso,
            &RBX::PlayerInstance::lowerTorso, &RBX::PlayerInstance::leftHand, &RBX::PlayerInstance::rightHand,
            &RBX::PlayerInstance::leftUpperArm, &RBX::PlayerInstance::rightUpperArm, &RBX::PlayerInstance::leftUpperLeg,
            &RBX::PlayerInstance::rightUpperLeg, &RBX::PlayerInstance::leftFoot, &RBX::PlayerInstance::rightFoot
        };
        hitbox = player.*parts[globals::free_aim_part];
    }

    RBX::Vector3 hit_position_3D = globals::free_aim_prediction
        ? hitbox.GetPosition() + (hitbox.GetVelocity() / RBX::Vector3{
            globals::free_aim_prediction_x, globals::free_aim_prediction_y, globals::free_aim_prediction_x })
            : hitbox.GetPosition();

    GetCursorPos(&cursor_point);
    ScreenToClient(FindWindowA(0, "Roblox"), &cursor_point);

    RBX::Vector2 screen_size = globals::visualengine.GetDimensions();
    RBX::Vector2 hit_position_2D = RBX::WorldToScreen(hit_position_3D, screen_size, globals::visualengine.GetViewMatrix());
    if (hit_position_2D.x == -1 || hit_position_2D.y == -1)
        return;

    RBX::Vector2 cursor_vec{ static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
    if (globals::free_aim_is_in_fov &&
        (hit_position_2D - cursor_vec).getMagnitude() > globals::free_aim_fov)
        return;

    uint64_t new_position_x = static_cast<uint64_t>(cursor_point.x);
    uint64_t new_position_y = static_cast<uint64_t>(
        screen_size.y - std::abs(screen_size.y - cursor_point.y) -
        ((globals::localplayer.hc_aim.address || globals::localplayer.aim.address ||
            globals::localplayer.Crosshair2.address || globals::localplayer.Hood_Game.address) ? 58 : 0));

    RBX::Instance* aim_instances[] = {
        &globals::localplayer.hc_aim,
        &globals::localplayer.aim,
        &globals::localplayer.jail_aim,
        &globals::localplayer.weapon_aim,
        &globals::localplayer.Crosshair2,
        &globals::localplayer.Hood_Game
    };

    for (auto inst : aim_instances) {
        if (inst->address) {
            inst->SetFramePositionX(new_position_x);
            inst->SetFramePositionY(new_position_y);
        }
    }


    if (globals::mouse_service) {
        RBX::Instance::CallCachedMouseService(globals::mouse_service);
        RBX::Instance().WriteMousePosition(globals::mouse_service, hit_position_2D.x, hit_position_2D.y);
    }
    else {
        utils::output::error("Mouse Service Invalid Write Failed");
    }
}


RBX::PlayerInstance aimbot_target2;
void RBX::initsilent() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    HWND rblx = FindWindowA(nullptr, "roblox");
    RBX::PlayerInstance localplayer = globals::localplayer;
    RBX::PlayerInstance savedPlayer{};
    bool isAimboting = false;

    while (true) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        if (GetForegroundWindow() != rblx) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        globals::free_aim_bind.update();
        if (!globals::free_aim_bind.enabled || !globals::free_aim) {
            isAimboting = false;
            aimbot_target2.address = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        POINT cursor_point;
        GetCursorPos(&cursor_point);

        RBX::PlayerInstance currentPlayer =
            (globals::free_aim_sticky && isAimboting && savedPlayer.address != 0)
            ? savedPlayer
            : getClosestPlayerFromCursor();

        if (currentPlayer.address == 0) {
            isAimboting = false;
            aimbot_target2.address = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        aimbot_target2 = currentPlayer;
        run(currentPlayer, cursor_point);
        savedPlayer = currentPlayer;
        isAimboting = true;
    }
}
