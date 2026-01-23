#include <Windows.h>
#include <thread>
#include <vector>
#include <immintrin.h>
#include <cmath>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "../features.h"
#include "../../../main.h"
#include "../../../engine/math/types/Math/Math.h"
#include "../../../../globals/globals.hpp"

RBX::Instance GetClosestPart(RBX::PlayerInstance& Player, const POINT& CursorPoint)
{
    const std::vector<RBX::Instance> Parts = {
        Player.head, Player.rootPart, Player.upperTorso, Player.lowerTorso,
        Player.leftUpperLeg, Player.leftFoot, Player.rightFoot,
        Player.leftUpperArm, Player.leftHand, Player.rightUpperArm, Player.rightHand,
    };

    const RBX::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };
    const RBX::Vector2 ScreenDimensions = globals::visualengine.GetDimensions();
    const RBX::Matrix4x4 ViewMatrix = globals::visualengine.GetViewMatrix();

    float ClosestDistance = FLT_MAX;
    RBX::Instance ClosestPart{};

    for (const auto& Part : Parts)
    {
        if (!Part.address) continue;

        const RBX::Vector3 Position = Part.GetPosition();
        const RBX::Vector2 ScreenPos = RBX::WorldToScreen(Position, ScreenDimensions, ViewMatrix);
        const float Distance = (ScreenPos - Cursor).getMagnitude();

        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestPart = Part;
        }
    }

    return ClosestPart;
}

RBX::PlayerInstance GetClosestPlayerFromCursor(const RBX::Vector2& Cursor, const RBX::Vector2& ScreenDimensions, const RBX::Matrix4x4& ViewMatrix)
{
    RBX::PlayerInstance ClosestPlayer{};
    float ClosestDistance = FLT_MAX;
    const auto& CachedPlayers = globals::cached_players;
    const RBX::PlayerInstance LocalPlayer = globals::localplayer;

    for (const auto& Player : CachedPlayers)
    {
        if (!Player.address || Player.address == LocalPlayer.address) continue;
        if (!Player.character.address || !Player.humanoid.address) continue;

        bool knockedCheck = globals::triggerbot_checks[0];
        bool deadCheck = globals::triggerbot_checks[1];
        bool grabbedCheck = globals::triggerbot_checks[2];
        bool teamCheck = globals::triggerbot_checks[3];
        

        if (knockedCheck && Player.knockedOut.getBoolFromValue())
            continue;

        if (deadCheck == true && Player.humanoid.GetHealth() <= 0)
            continue;

        if (grabbedCheck && Player.ifGrabbed.address != 0)
            continue;

        if (teamCheck && Player.team.address == globals::localplayer.team.address)
            continue;
        const RBX::Vector3 Position = Player.rootPart.GetPosition();
        const RBX::Vector2 ScreenPos = RBX::WorldToScreen(Position, ScreenDimensions, ViewMatrix);
        const float Distance = (ScreenPos - Cursor).getMagnitude();

        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestPlayer = Player;
        }
    }

    return ClosestPlayer;
}

void RBX::TriggerBotLoop()
{
 
    while (true)
    {
        globals::triggerBind.update();

        if (!globals::triggerBind.enabled || !globals::triggerbot_enabled)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (globals::localplayer.currentToolName == "[Knife]" && globals::knife_CHeck) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        const RBX::PlayerInstance LocalPlayer = globals::localplayer;
        if (!LocalPlayer.address || !LocalPlayer.character.address)
            continue;

        POINT CursorPoint;
        GetCursorPos(&CursorPoint);
        ScreenToClient(FindWindowA(0, "Roblox"), &CursorPoint);

        const RBX::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };
        const RBX::Vector2 ScreenDimensions = globals::visualengine.GetDimensions();
        const RBX::Matrix4x4 ViewMatrix = globals::visualengine.GetViewMatrix();

        RBX::PlayerInstance ClosestPlayer = GetClosestPlayerFromCursor(Cursor, ScreenDimensions, ViewMatrix);
        if (!ClosestPlayer.address || !ClosestPlayer.character.address || !ClosestPlayer.humanoid.address)
            continue;

        if (globals::triggerbot_team_check && ClosestPlayer.team.address == LocalPlayer.team.address)
            continue;

        RBX::Instance ClosestPart = GetClosestPart(ClosestPlayer, CursorPoint);
        if (!ClosestPart.address)
            continue;


        const RBX::Vector3 PartPosition = ClosestPart.GetPosition();
        const RBX::Vector3 Velocity = ClosestPart.GetVelocity();
        const RBX::Vector3 PredictedPosition = {
            PartPosition.x + (Velocity.x * globals::trigger_bot_prediction_x),
            PartPosition.y + (Velocity.y * globals::trigger_bot_prediction_y),
            PartPosition.z + (Velocity.z * globals::trigger_bot_prediction_x),
        };

        const RBX::Vector2 ScreenPos = RBX::WorldToScreen(PredictedPosition, ScreenDimensions, ViewMatrix);
        const float DistanceFromCursor = (ScreenPos - Cursor).getMagnitude();

        if (DistanceFromCursor < globals::triggerbot_fov)
        {
            INPUT Input = {};
            Input.type = INPUT_MOUSE;
            Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &Input, sizeof(INPUT));

            std::this_thread::sleep_for(std::chrono::milliseconds(globals::triggerbot_delay));

            Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &Input, sizeof(INPUT));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
