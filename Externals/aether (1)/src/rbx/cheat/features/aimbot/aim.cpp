#include <Windows.h>
#include <thread>
#include <random>
#include <vector>
#include <immintrin.h>
#include <cmath>
#include <future>
#include <mmsystem.h>
static std::unordered_map<std::uintptr_t, float> LastKnownHealth;

#pragma comment(lib, "winmm.lib")
#include "applepay.h"

#include "../features.h"
#include "../../../main.h"
#include "../../../engine/math/types/Math/Math.h"
#include "../../../../globals/globals.hpp"

#define M_PI 3.14159265358979323846

static RBX::Vector3 Recalculate_Velocity(RBX::PlayerInstance player)
{
    RBX::Vector3 old_Position = player.rootPart.GetPosition();
    std::this_thread::sleep_for(std::chrono::milliseconds(115));
    return (player.rootPart.GetPosition() - old_Position) / 0.115;
}

RBX::Instance FindPartByName(RBX::Instance& character, const std::string& partName) {
    return character.FindFirstChild(partName);
}

static float sigmoid(float x) {
    return 1 / (1 + std::exp(-x));
}

static RBX::Vector3 Random_Vector3(const float x, const float y) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis_x(-x, x);
    std::uniform_real_distribution<float> dis_y(-y, y);
    return { dis_x(gen) , dis_y(gen) , dis_x(gen) };
}
 float RandomFloat(float min, float max)
{
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

static float Ease(float t) {
    switch (globals::aimbot_easing_style) {
    case 0: return t;
    case 1: return 1 - std::cos((t * M_PI) / 2);
    case 2: return t * t;
    case 3: return t * t * t;
    case 4: return t * t * t * t;
    case 5: return t * t * t * t * t;
    case 6: return t == 0 ? 0 : std::pow(2, 10 * (t - 1));
    case 7: return 1 - std::sqrt(1 - std::pow(t, 2));
    case 8: return t * t * (2.70158f * t - 1.70158f);
    case 9:
        if (t < 1 / 2.75f) return 7.5625f * t * t;
        else if (t < 2 / 2.75f) return 7.5625f * (t -= 1.5f / 2.75f) * t + 0.75f;
        else if (t < 2.5f / 2.75f) return 7.5625f * (t -= 2.25f / 2.75f) * t + 0.9375f;
        else return 7.5625f * (t -= 2.625f / 2.75f) * t + 0.984375f;
    case 10: return t == 0 ? 0 : t == 1 ? 1 : -std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * M_PI);
    default: return t;
    }
}


RBX::Matrix3x3 CreateRotationMatrix(float pitch, float yaw, float roll)
{
    float cx = cosf(pitch), sx = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);
    float cz = cosf(roll), sz = sinf(roll);

    RBX::Vector3 row0 = {
        cy * cz,
        cz * sx * sy - cx * sz,
        cx * cz * sy + sx * sz
    };

    RBX::Vector3 row1 = {
        cy * sz,
        cx * cz + sx * sy * sz,
        -cz * sx + cx * sy * sz
    };

    RBX::Vector3 row2 = {
        -sy,
        cy * sx,
        cx * cy
    };

    return RBX::Matrix3x3{
        row0.x, row0.y, row0.z,
        row1.x, row1.y, row1.z,
        row2.x, row2.y, row2.z
    };
}



static float SmoothEase(float t, float strength = 1.0f) {
    t = Ease(t);
    return std::pow(t, strength);
}

static RBX::Matrix3x3 Slerp_Matrix3(const RBX::Matrix3x3& a, const RBX::Matrix3x3& b, float t) {
    t = SmoothEase(t, 1.2f);
    if (t == 1) return b;

    RBX::Matrix3x3 result{};
    for (int i = 0; i < 9; ++i) {
        result.data[i] = a.data[i] + (b.data[i] - a.data[i]) * t;
    }
    return result;
}

static RBX::Vector2 SmoothMouseDelta(const RBX::Vector2& current, const RBX::Vector2& target, float smoothFactor) {
    return current + (target - current) * smoothFactor;
}


static RBX::Matrix3x3 Lerp_Matrix3(const RBX::Matrix3x3& a, const RBX::Matrix3x3& b, float t) {
    t = Ease(t);
    if (t == 1) return b;

    RBX::Matrix3x3 result{};
    for (int i = 0; i < 9; ++i) {
        result.data[i] = a.data[i] + (b.data[i] - a.data[i]) * t;
    }
    return result;
}

static RBX::Vector3 Cross_Product(const RBX::Vector3& vec1, const RBX::Vector3& vec2) {
    return {
        vec1.y * vec2.z - vec1.z * vec2.y,
        vec1.z * vec2.x - vec1.x * vec2.z,
        vec1.x * vec2.y - vec1.y * vec2.x
    };
}

static RBX::Matrix3x3 Look_At_To_Matrix(const RBX::Vector3& cameraPosition, const RBX::Vector3& targetPosition) {
    RBX::Vector3 forward = (targetPosition - cameraPosition).normalize();
    RBX::Vector3 right = Cross_Product({ 0, 1, 0 }, forward).normalize();
    RBX::Vector3 up = Cross_Product(forward, right);

    RBX::Matrix3x3 lookAtMatrix{};
    lookAtMatrix.data[0] = -right.x;  lookAtMatrix.data[1] = up.x;  lookAtMatrix.data[2] = -forward.x;
    lookAtMatrix.data[3] = right.y;  lookAtMatrix.data[4] = up.y;  lookAtMatrix.data[5] = -forward.y;
    lookAtMatrix.data[6] = -right.z;  lookAtMatrix.data[7] = up.z;  lookAtMatrix.data[8] = -forward.z;

    return lookAtMatrix;
}

void InitializePlayerParts(RBX::PlayerInstance& player) {
    std::vector<std::string> partNames = { "Head", "HumanoidRootPart", "UpperTorso", "LowerTorso", "LeftUpperLeg", "LeftUpperArm", "LeftHand", "RightUpperArm", "RightHand" };

    std::vector<RBX::Instance> parts;
    for (const std::string& partName : partNames) {
        parts.push_back(FindPartByName(player.character, partName));
    }

    player.head = parts[0];
    player.rootPart = parts[1];
    player.upperTorso = parts[2];
    player.lowerTorso = parts[3];
    player.leftUpperLeg = parts[4];
    player.leftUpperArm = parts[5];
    player.leftHand = parts[6];
    player.rightUpperArm = parts[7];
    player.rightHand = parts[8];
}

RBX::Instance getClosestPart(RBX::PlayerInstance& player, const POINT& cursor_point) {
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

static RBX::PlayerInstance getClosestPlayerFromCursor() {
    
    std::vector<RBX::PlayerInstance>& cached_players = globals::cached_players;
    RBX::PlayerInstance closestPlayer{};
    float shortestDistance = 9e9f;

    RBX::PlayerInstance localPlayer = globals::localplayer;
    RBX::Instance localPlayerTeam = localPlayer.team;

    POINT cursor_point;
    if (globals::aimbot_mode == 1) {
        GetCursorPos(&cursor_point);
        ScreenToClient(FindWindowA(0, ("Roblox")), &cursor_point);
    }
    RBX::Vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
    RBX::Vector2 screenDimensions = globals::visualengine.GetDimensions();
    RBX::Matrix4x4 viewMatrix = globals::visualengine.GetViewMatrix();

    for (RBX::PlayerInstance& player : cached_players) {
  
        if (player.address == localPlayer.address || !player.character.address || !player.humanoid.address)
            continue;


        if (player.address == 0)
            continue;

        bool knockedCheck = globals::aimbot_checks[0];
        bool deadCheck = globals::aimbot_checks[1];
        bool grabbedCheck = globals::aimbot_checks[2];
        bool teamCheck = globals::aimbot_checks[3];
        bool sticky_target = globals::aimbot_sticky;

        if (knockedCheck && player.knockedOut.getBoolFromValue())
            continue;

        if (deadCheck == true && player.humanoid.GetHealth() <= 0)
            continue;

        if (grabbedCheck && player.ifGrabbed.address != 0)
            continue;

        if (teamCheck && player.team.address == localPlayerTeam.address)
            continue;

        RBX::Instance part = player.rootPart;
        RBX::Vector3 partPosition = part.GetPosition();

        if (globals::aimbot_mode == 0) {
            float distance_to_player = (globals::camera.GetCameraPosition() - partPosition).magnitude();

            if (distance_to_player > globals::max_aimbot_distance)
                continue;

            if (shortestDistance > distance_to_player) {
                closestPlayer = player;
                shortestDistance = distance_to_player;
            }
        }
        else if (globals::aimbot_mode == 1) {
            RBX::Vector2 partPositionOnScreen = RBX::WorldToScreen(partPosition, screenDimensions, viewMatrix);

            float distance_from_cursor = (partPositionOnScreen - cursor).getMagnitude();
            if (shortestDistance > distance_from_cursor) {
                closestPlayer = player;
                shortestDistance = distance_from_cursor;
            }
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
    return (magnitude <= globals::aimbot_fov_size);
}

static void run(RBX::PlayerInstance player, RBX::Vector3 resolved_velocity)
{
    if (globals::autotype)
    {
        float distance_vec = (globals::camera.GetCameraPosition() - globals::localplayer.head.GetPosition()).magnitude();
        globals::aimbot_type = (distance_vec <= 2.0f) ? 1 : 0;
    }

    static POINT game_mouse_pos{};
    if (game_mouse_pos.x == 0 && game_mouse_pos.y == 0)
    {
        GetCursorPos(&game_mouse_pos);
        ScreenToClient(FindWindowA(0, "roblox"), &game_mouse_pos);
    }

    if ((globals::aimbot_checks[0] & player.knockedOut.getBoolFromValue()) ||
        (globals::aimbot_checks[1] && player.humanoid.GetHealth() <= 0) ||
        (globals::aimbot_checks[2] && player.ifGrabbed.address != 0) ||
        (globals::aimbot_checks[3] && player.team.address == globals::localplayer.team.address))
        return;

    POINT cursor_point;
    GetCursorPos(&cursor_point);
    ScreenToClient(FindWindowA(0, "Roblox"), &cursor_point);
    float cursor_x = static_cast<float>(cursor_point.x);
    float cursor_y = static_cast<float>(cursor_point.y);

    RBX::Instance hitbox = globals::closest_part ? getClosestPart(player, cursor_point) : [&] {
        switch (globals::aimbot_part)
        {
        case 0: return player.head;
        case 1: return player.rootPart;
        case 2: return player.upperTorso;
        case 3: return player.lowerTorso;
        case 4: return player.leftHand;
        case 5: return player.rightHand;
        case 6: return player.leftUpperArm;
        case 7: return player.rightUpperArm;
        case 8: return player.leftUpperLeg;
        case 9: return player.rightUpperLeg;
        case 10: return player.leftFoot;
        case 11: return player.rightFoot;
        default: return player.head;
        }
        }();

    RBX::Vector3 velocity_vec = globals::resolver ? resolved_velocity : hitbox.GetVelocity();
    if (globals::aimbot_type == 0)
        velocity_vec = velocity_vec / RBX::Vector3{ globals::camera_prediction_x, globals::camera_prediction_y, globals::camera_prediction_x };
    else
        velocity_vec = velocity_vec / RBX::Vector3{ globals::free_aim_prediction_x, globals::free_aim_prediction_y, globals::free_aim_prediction_x };

    RBX::Vector3 predicted_pos = hitbox.GetPosition() + (globals::camera_prediction ? velocity_vec : RBX::Vector3{});
    RBX::Vector3 shake_offset = globals::shake ? Random_Vector3(globals::shake_x, globals::shake_y) : RBX::Vector3{};
    RBX::Vector3 target_world_pos = predicted_pos + shake_offset;

    RBX::Vector2 target_screen_pos = RBX::WorldToScreen(target_world_pos, globals::visualengine.GetDimensions(), globals::visualengine.GetViewMatrix());
    if (target_screen_pos.x == -1 || target_screen_pos.y == -1) return;

    if (globals::aimbot_type == 0)
    {
        float smooth_factor = Ease((100.1f - globals::smoothness_camera) / 100.0f);
        RBX::Instance camera = globals::workspace.FindFirstChildOfClass("Camera");
        RBX::Matrix3x3 target_matrix = Look_At_To_Matrix(camera.GetCameraPosition(), target_world_pos);
        RBX::Matrix3x3 smooth_matrix = Slerp_Matrix3(camera.GetCameraRotation(), target_matrix, smooth_factor);

        if (globals::shake)
        {
            float shake_x = RandomFloat(-globals::shake_x, globals::shake_x);
            float shake_y = RandomFloat(-globals::shake_y, globals::shake_y);
            float shake_z = RandomFloat(-globals::shake_x, globals::shake_x);
            RBX::Matrix3x3 shake_matrix = CreateRotationMatrix(shake_x, shake_y, shake_z);
            camera.SetCameraRotation(smooth_matrix * shake_matrix);
        }
        else
        {
            camera.SetCameraRotation(smooth_matrix);
        }
    }
    else
    {
        constexpr float max_move = 100.0f;
        float smooth = std::clamp(globals::mouse_smoothness, 1.0f, 100.0f);
        float scale = globals::mouse_sensitivity / smooth;
        RBX::Vector2 delta = target_screen_pos - RBX::Vector2{ cursor_x, cursor_y };

        bool in_deadzone = globals::usedeadzone && (std::abs(delta.x) <= globals::deadzoneX && std::abs(delta.y) <= globals::deadzoneY);
        if (globals::usedeadzone && in_deadzone) return;

        float shake_x = globals::shake ? RandomFloat(-globals::shake_x, globals::shake_x) : 0.0f;
        float shake_y = globals::shake ? RandomFloat(-globals::shake_y, globals::shake_y) : 0.0f;

        RBX::Vector2 move_vec = delta * scale;
        move_vec.x += shake_x;
        move_vec.y += shake_y;

        move_vec.x = std::clamp(move_vec.x, -max_move, max_move);
        move_vec.y = std::clamp(move_vec.y, -max_move, max_move);

        if (std::abs(move_vec.x) >= 1.0f || std::abs(move_vec.y) >= 1.0f)
        {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dx = static_cast<LONG>(move_vec.x);
            input.mi.dy = static_cast<LONG>(move_vec.y);
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
}

RBX::PlayerInstance aimbot_target;
/**/
/*
std::mutex HealthMutex;

void StartHealthMonitor()
{
    std::thread([]()
        {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));


                std::lock_guard<std::mutex> Lock(HealthMutex);

                for (RBX::PlayerInstance& Player : globals::cached_players)
                {
                    if (!Player.address || !Player.character.address || !Player.humanoid.address)
                        continue;




                    if (Player.address != aimbot_target.address && Player.address != aimbot_target2.address)
                        continue;

                    float CurrentHealth = Player.humanoid.GetHealth();
                    std::uintptr_t PlayerKey = Player.address;

                    auto Iterator = LastKnownHealth.find(PlayerKey);
                    if (Iterator != LastKnownHealth.end())
                    {
                        float PreviousHealth = Iterator->second;
                        if (CurrentHealth < PreviousHealth)
                        {
                            float Damage = PreviousHealth - CurrentHealth;
                            std::ostringstream Stream;
                            Stream << Player.name << " took " << std::fixed << std::setprecision(2) << Damage << " damage";

                            if (!PlaySoundA(reinterpret_cast<char*>(applepay), NULL, SND_ASYNC | SND_MEMORY))
                            {
                             //   MessageBoxA(NULL, "PlaySoundA failed", "Error", MB_OK);
                            }

                            LastKnownHealth[PlayerKey] = CurrentHealth;
                        }

                    }
                    else
                    {
                        LastKnownHealth[PlayerKey] = CurrentHealth;
                    }
                }
            }
        }).detach();
}
*/
bool firstthreadoptset = false;
bool secondthreadoptset = false;
void RBX::InitializeAimbot() {
	if (!firstthreadoptset) {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		firstthreadoptset = true;
	}
   // ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    RBX::PlayerInstance saved_player{};
    bool is_aimboting = false;
    RBX::Vector3 velocity{};
    RBX::PlayerInstance current_player;
    RBX::PlayerInstance localplayer = globals::localplayer;
  //  StartHealthMonitor();
    HWND rblx = FindWindowA(0, ("roblox"));
    while (true) {
        if (!secondthreadoptset) {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
            secondthreadoptset = true;
        }
      //  
        //  CFrameFly();
        POINT cursor_point;
        GetCursorPos(&cursor_point);

        if (GetForegroundWindow() != rblx) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        globals::aimbot_bind.update();

        if (!globals::aimbot_bind.enabled || !globals::aimbot) {
            is_aimboting = false;
            aimbot_target.address = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        current_player = (globals::aimbot_sticky && is_aimboting && saved_player.address != 0) ? saved_player : getClosestPlayerFromCursor();
        if (current_player.address == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            is_aimboting = false;
            aimbot_target.address = 0;
            continue;
        }
        
    //    std::this_thread::sleep_for(std::chrono::milliseconds(15));
     //   std::this_thread::sleep_for(std::chrono::milliseconds(50));
        aimbot_target = current_player;
        if (globals::closest_part) InitializePlayerParts(current_player);

        run(current_player, velocity);
        saved_player = current_player;
        is_aimboting = true;

        if (globals::resolver) {
            std::thread([&velocity, &current_player]() { velocity = Recalculate_Velocity(current_player); }).detach();
        }
        if (globals::spectate_target) {
            aimbot_target.head.Spectate(aimbot_target.head);
        }
      /*  if (aimbot_target.address != 0) {
            aimbot_target.head.Spectate(aimbot_target.head);
        }
        else {
            aimbot_target.head.UnSpectate();
        }*/ // camera desync
     //   std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}