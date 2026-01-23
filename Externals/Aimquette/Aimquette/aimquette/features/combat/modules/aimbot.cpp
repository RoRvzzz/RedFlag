#include "../combat.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <random>
#include <unordered_map>
#include "../../../util/driver/driver.h"
#include "../../hook.h"
#include <windows.h>
#include "../../wallcheck/wallcheck.h"

#define max
#undef max
#define min
#undef min

using namespace roblox;
static bool foundTarget = false;

// Helper function to check if target is jumping (in air) - from layuh22
static bool isTargetJumping(const roblox::player& target) {
    if (!is_valid_address(target.hrp.address)) return false;
    
    Vector3 velocity = target.hrp.get_velocity();
    float vertical_velocity = velocity.y;
    
    // Consider jumping if vertical velocity is positive and significant
    return vertical_velocity > 5.0f;
}

// Helper function to check if player is in same crew/group (checks team membership)
static bool isSameCrew(const roblox::player& target) {
    if (!globals::combat::crew_check) return false;
    
    try {
        // Check if local player and target have valid teams
        if (!is_valid_address(globals::instances::lp.main.address) || !is_valid_address(target.main.address)) {
            return false;
        }
        
        // Get teams
        roblox::instance localTeam = globals::instances::lp.team;
        roblox::instance targetTeam = target.team;
        
        // If both have valid teams and they're the same, they're in the same crew
        if (localTeam.is_valid() && targetTeam.is_valid()) {
            return localTeam.address == targetTeam.address;
        }
        
        // Alternative: Check if they have the same team name (crew/group indicator)
        if (localTeam.is_valid() && targetTeam.is_valid()) {
            std::string localTeamName = localTeam.get_name();
            std::string targetTeamName = targetTeam.get_name();
            if (!localTeamName.empty() && !targetTeamName.empty()) {
                return localTeamName == targetTeamName;
            }
        }
    } catch (...) {
        // Silently handle errors
    }
    
    return false;
}

struct MouseSettings {
    float baseDPI = 800.0f;
    float currentDPI = 800.0f;
    float dpiScaleFactor = 1.0f;
    bool dpiAutoDetected = false;

    void updateDPIScale() {
        dpiScaleFactor = baseDPI / currentDPI;
    }

    float getDPIAdjustedSensitivity() const {
        return dpiScaleFactor;
    }
} mouseSettings;

float CalculateDistance(Vector2 first, Vector2 sec) {
    return sqrt(pow(first.x - sec.x, 2) + pow(first.y - sec.y, 2));
}

float CalculateDistance1(Vector3 first, Vector3 sec) {
    return sqrt(pow(first.x - sec.x, 2) + pow(first.y - sec.y, 2) + pow(first.z - sec.z, 2));
}


// Helper function to check if a part should be ignored
bool isPartIgnored(int partIndex) {
    // Get the appropriate ignore parts vector based on aimbot type
    std::vector<int>* ignoreParts = nullptr;
    if (globals::combat::aimbottype == 0) {
        // Camera aimbot
        if (globals::combat::ignore_parts_camera != nullptr && globals::combat::ignore_parts_camera->size() > partIndex) {
            ignoreParts = globals::combat::ignore_parts_camera;
        }
    } else if (globals::combat::aimbottype == 1) {
        // Mouse aimbot
        if (globals::combat::ignore_parts_mouse != nullptr && globals::combat::ignore_parts_mouse->size() > partIndex) {
            ignoreParts = globals::combat::ignore_parts_mouse;
        }
    }
    
    // If ignore parts is set and this part is marked as ignored, return true
    if (ignoreParts != nullptr && (*ignoreParts)[partIndex] != 0) {
        return true;
    }
    return false;
}

// Helper function to get target part based on airpart settings
roblox::instance getTargetPart(const roblox::player& target, int partIndex) {
    // Helper function to get fallback part (first non-ignored part)
    auto getFallbackPart = [&target]() -> roblox::instance {
        if (!isPartIgnored(0) && is_valid_address(target.head.address)) return target.head;
        if (!isPartIgnored(1) && is_valid_address(target.uppertorso.address)) return target.uppertorso;
        if (!isPartIgnored(2) && is_valid_address(target.lowertorso.address)) return target.lowertorso;
        if (!isPartIgnored(3) && is_valid_address(target.hrp.address)) return target.hrp;
        if (!isPartIgnored(4) && is_valid_address(target.lefthand.address)) return target.lefthand;
        if (!isPartIgnored(5) && is_valid_address(target.righthand.address)) return target.righthand;
        if (!isPartIgnored(6) && is_valid_address(target.leftfoot.address)) return target.leftfoot;
        if (!isPartIgnored(7) && is_valid_address(target.rightfoot.address)) return target.rightfoot;
        // If all parts are ignored, return HRP as ultimate fallback
        return target.hrp;
    };
    
    switch (partIndex) {
        case 0: {
            if (isPartIgnored(0)) return getFallbackPart();
            return target.head;
        }
        case 1: {
            if (isPartIgnored(1)) return getFallbackPart();
            return target.uppertorso;
        }
        case 2: {
            if (isPartIgnored(2)) return getFallbackPart();
            return target.lowertorso;
        }
        case 3: {
            if (isPartIgnored(3)) return getFallbackPart();
            return target.hrp;
        }
        case 4: {
            if (isPartIgnored(4)) return getFallbackPart();
            return target.lefthand;
        }
        case 5: {
            if (isPartIgnored(5)) return getFallbackPart();
            return target.righthand;
        }
        case 6: {
            if (isPartIgnored(6)) return getFallbackPart();
            return target.leftfoot;
        }
        case 7: {
            if (isPartIgnored(7)) return getFallbackPart();
            return target.rightfoot;
        }
        case 8: {
            // Closest Part logic - skip ignored parts
            struct Candidate { roblox::instance inst; int index; };
            std::vector<Candidate> candidates;
            if (!isPartIgnored(0)) candidates.push_back({ target.head, 0 });
            if (!isPartIgnored(1)) candidates.push_back({ target.uppertorso, 1 });
            if (!isPartIgnored(2)) candidates.push_back({ target.lowertorso, 2 });
            if (!isPartIgnored(3)) candidates.push_back({ target.hrp, 3 });
            if (!isPartIgnored(4)) candidates.push_back({ target.lefthand, 4 });
            if (!isPartIgnored(5)) candidates.push_back({ target.righthand, 5 });
            if (!isPartIgnored(6)) candidates.push_back({ target.leftfoot, 6 });
            if (!isPartIgnored(7)) candidates.push_back({ target.rightfoot, 7 });
            
            // If all parts are ignored, return fallback
            if (candidates.empty()) {
                return getFallbackPart();
            }
            
            POINT p; HWND rw = FindWindowA(nullptr, "Roblox"); 
            if (rw) { 
                GetCursorPos(&p); 
                ScreenToClient(rw, &p); 
            }
            Vector2 mouse = { (float)p.x, (float)p.y };
            float bestDist = 9e18f;
            roblox::instance bestPart = candidates[0].inst;
            
            for (auto& c : candidates) {
                if (!is_valid_address(c.inst.address)) continue;
                Vector2 scr = roblox::worldtoscreen(c.inst.get_pos());
                if (scr.x == -1.0f || scr.y == -1.0f) continue;
                float dx = mouse.x - scr.x; 
                float dy = mouse.y - scr.y; 
                float d2 = dx * dx + dy * dy;
                if (d2 < bestDist) { 
                    bestDist = d2; 
                    bestPart = c.inst; 
                }
            }
            return bestPart;
        }
        case 9: {
            // Random Part logic - skip ignored parts
            std::vector<roblox::instance> candidates;
            if (!isPartIgnored(0) && is_valid_address(target.head.address)) candidates.push_back(target.head);
            if (!isPartIgnored(1) && is_valid_address(target.uppertorso.address)) candidates.push_back(target.uppertorso);
            if (!isPartIgnored(2) && is_valid_address(target.lowertorso.address)) candidates.push_back(target.lowertorso);
            if (!isPartIgnored(3) && is_valid_address(target.hrp.address)) candidates.push_back(target.hrp);
            if (!isPartIgnored(4) && is_valid_address(target.lefthand.address)) candidates.push_back(target.lefthand);
            if (!isPartIgnored(5) && is_valid_address(target.righthand.address)) candidates.push_back(target.righthand);
            if (!isPartIgnored(6) && is_valid_address(target.leftfoot.address)) candidates.push_back(target.leftfoot);
            if (!isPartIgnored(7) && is_valid_address(target.rightfoot.address)) candidates.push_back(target.rightfoot);
            
            if (!candidates.empty()) {
                int randomIndex = rand() % candidates.size();
                return candidates[randomIndex];
            } else {
                return getFallbackPart(); // fallback
            }
        }
        case 10: {
            // Closest Point logic - find closest point on target's bounding box to camera
            HWND robloxWindow = FindWindowA(nullptr, "Roblox");
            if (!robloxWindow) return getFallbackPart();
            
            POINT cursorPoint;
            if (!GetCursorPos(&cursorPoint) || !ScreenToClient(robloxWindow, &cursorPoint)) {
                return getFallbackPart();
            }
            
            Vector2 mouse = { static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y) };
            
            // Get all body parts and their screen positions
            struct PartCandidate {
                roblox::instance inst;
                Vector2 screenPos;
            };
            
            std::vector<PartCandidate> candidates;
            std::vector<roblox::instance> bodyParts = {
                target.head, target.uppertorso, target.lowertorso, target.hrp,
                target.lefthand, target.righthand, target.leftfoot, target.rightfoot
            };
            
            int partIndex = 0;
            for (auto& bp : bodyParts) {
                if (is_valid_address(bp.address)) {
                    // Skip ignored parts for closest point mode
                    if (partIndex < 8 && isPartIgnored(partIndex)) {
                        partIndex++;
                        continue;
                    }
                    Vector2 screenPos = roblox::worldtoscreen(bp.get_pos());
                    if (screenPos.x != -1.0f && screenPos.y != -1.0f) {
                        candidates.push_back({ bp, screenPos });
                    }
                }
                partIndex++;
            }
            
            if (candidates.empty()) {
                return getFallbackPart();
            }
            
            // Calculate bounding box from visible parts
            float minX = candidates[0].screenPos.x, maxX = candidates[0].screenPos.x;
            float minY = candidates[0].screenPos.y, maxY = candidates[0].screenPos.y;
            
            for (const auto& c : candidates) {
                minX = std::min(minX, c.screenPos.x);
                maxX = std::max(maxX, c.screenPos.x);
                minY = std::min(minY, c.screenPos.y);
                maxY = std::max(maxY, c.screenPos.y);
            }
            
            // Add padding
            float padding = 10.0f;
            minX -= padding; maxX += padding;
            minY -= padding; maxY += padding;
            
            // Find closest point on bounding box to mouse
            float closestX = std::max(minX, std::min(maxX, mouse.x));
            float closestY = std::max(minY, std::min(maxY, mouse.y));
            
            // Find the body part closest to this screen point
            float bestDist = 9e18f;
            roblox::instance bestPart = candidates[0].inst;
            
            for (const auto& c : candidates) {
                float dx = c.screenPos.x - closestX;
                float dy = c.screenPos.y - closestY;
                float dist = dx * dx + dy * dy;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestPart = c.inst;
                }
            }
            
            return bestPart;
        }
        default: 
            return target.head;
    }
}

Vector3 lerp_vector3(const Vector3& a, const Vector3& b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

Vector3 AddVector3(const Vector3& first, const Vector3& sec) {
    return { first.x + sec.x, first.y + sec.y, first.z + sec.z };
}

Vector3 DivVector3(const Vector3& first, const Vector3& sec) {
    return { first.x / sec.x, first.y / sec.y, first.z / sec.z };
}

Vector3 normalize(const Vector3& vec) {
    float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return (length != 0) ? Vector3{ vec.x / length, vec.y / length, vec.z / length } : vec;
}

Vector3 crossProduct(const Vector3& vec1, const Vector3& vec2) {
    return {
        vec1.y * vec2.z - vec1.z * vec2.y,
        vec1.z * vec2.x - vec1.x * vec2.z,
        vec1.x * vec2.y - vec1.y * vec2.x
    };
}

Matrix3 LookAtToMatrix(const Vector3& cameraPosition, const Vector3& targetPosition) {
    Vector3 forward = normalize(Vector3{ (targetPosition.x - cameraPosition.x), (targetPosition.y - cameraPosition.y), (targetPosition.z - cameraPosition.z) });
    Vector3 right = normalize(crossProduct({ 0, 1, 0 }, forward));
    Vector3 up = crossProduct(forward, right);

    Matrix3 lookAtMatrix{};
    lookAtMatrix.data[0] = -right.x;  lookAtMatrix.data[1] = up.x;  lookAtMatrix.data[2] = -forward.x;
    lookAtMatrix.data[3] = right.y;  lookAtMatrix.data[4] = up.y;  lookAtMatrix.data[5] = -forward.y;
    lookAtMatrix.data[6] = -right.z;  lookAtMatrix.data[7] = up.z;  lookAtMatrix.data[8] = -forward.z;

    return lookAtMatrix;
}

bool detectMouseDPI() {
    HWND robloxWindow = FindWindowA(0, "Roblox");
    if (!robloxWindow) return false;

    HDC hdc = GetDC(robloxWindow);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(robloxWindow, hdc);

    HKEY hkey;
    DWORD sensitivity = 10;
    DWORD size = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Mouse", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        RegQueryValueEx(hkey, "MouseSensitivity", NULL, NULL, (LPBYTE)&sensitivity, &size);
        RegCloseKey(hkey);
    }

    float estimatedDPI = 800.0f * (sensitivity / 10.0f) * (dpiX / 96.0f);
    mouseSettings.currentDPI = std::max(400.0f, std::min(3200.0f, estimatedDPI));
    mouseSettings.updateDPIScale();
    mouseSettings.dpiAutoDetected = true;

    return true;
}

// Custom easing function using Bezier curve control points from graph
float evaluateCustomEasing(float t) {
    // Clamp t to [0, 1]
    t = std::max(0.0f, std::min(1.0f, t));
    
    // Bezier curve: start at (0,0), end at (1,1), control points from globals
    // Cubic Bezier formula: B(t) = (1-t)^3 * P0 + 3(1-t)^2 * t * P1 + 3(1-t) * t^2 * P2 + t^3 * P3
    // Where P0 = (0,0), P1 = (easing_point1_x, easing_point1_y), P2 = (easing_point2_x, easing_point2_y), P3 = (1,1)
    
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    // Calculate Y value of the curve at point t
    float y = uuu * 0.0f +  // P0.y = 0
              3 * uu * t * globals::combat::easing_point1_y +  // P1.y
              3 * u * tt * globals::combat::easing_point2_y +  // P2.y
              ttt * 1.0f;  // P3.y = 1
    
    return y;
}

static float applyEasing(float t, int style) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (style) {
    case 0: // None (raw)
        return t; // factor already set externally; treat as linear
    case 1: // Linear
        return t;
    case 2: // EaseInQuad
        return t * t;
    case 3: // EaseOutQuad
        return t * (2.0f - t);
    case 4: // EaseInOutQuad
        return (t < 0.5f) ? (2.0f * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f);
    case 5: // EaseInCubic
        return t * t * t;
    case 6: // EaseOutCubic
        return 1.0f - std::pow(1.0f - t, 3.0f);
    case 7: // EaseInOutCubic
        return (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f);
    case 8: // EaseInSine
        return 1.0f - std::cos((t * 3.14159265f) / 2.0f);
    case 9: // EaseOutSine
        return std::sin((t * 3.14159265f) / 2.0f);
    case 10: // EaseInOutSine
        return -(std::cos(3.14159265f * t) - 1.0f) / 2.0f;
    default:
        return t;
    }
}

void performCameraAimbot(const Vector3& targetPos, const Vector3& cameraPos) {
    roblox::camera camera = globals::instances::camera;
    Matrix3 currentRotation = camera.getRot();
    Matrix3 targetMatrix = LookAtToMatrix(cameraPos, targetPos);

    // Apply shake if enabled (improved version with X, Y, Z modification)
    if (globals::combat::camlock_shake) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);
        
        // Generate shake offsets for X, Y, and Z axes
        float shakeOffsetX = shakeDist(gen) * globals::combat::camlock_shake_x;
        float shakeOffsetY = shakeDist(gen) * globals::combat::camlock_shake_y;
        float shakeOffsetZ = shakeDist(gen) * globals::combat::camlock_shake_z;
        
        // Apply shake to the target matrix by slightly adjusting the rotation
        // This creates a subtle shaking effect while maintaining the aim direction
        Matrix3 shakenMatrix = targetMatrix;
        
        // Apply shake to the rotation matrix (affecting pitch, yaw, and roll)
        // Adjust the forward vector (first row) for X shake
        shakenMatrix.data[0] += shakeOffsetX * 0.01f;
        shakenMatrix.data[1] += shakeOffsetY * 0.01f;
        shakenMatrix.data[2] += shakeOffsetZ * 0.01f;
        
        // Adjust the up vector (second row) for additional Y shake
        shakenMatrix.data[4] += shakeOffsetY * 0.01f;
        
        // Adjust the right vector (third row) for Z shake (roll effect)
        shakenMatrix.data[6] += shakeOffsetZ * 0.01f;
        shakenMatrix.data[7] += shakeOffsetZ * 0.01f;
        
        // Normalize the matrix to prevent distortion
        Vector3 forward = { shakenMatrix.data[0], shakenMatrix.data[1], shakenMatrix.data[2] };
        Vector3 up = { shakenMatrix.data[3], shakenMatrix.data[4], shakenMatrix.data[5] };
        Vector3 right = { shakenMatrix.data[6], shakenMatrix.data[7], shakenMatrix.data[8] };
        
        forward = normalize(forward);
        up = normalize(up);
        right = normalize(right);
        
        // Reconstruct the matrix with normalized vectors
        shakenMatrix.data[0] = forward.x; shakenMatrix.data[1] = forward.y; shakenMatrix.data[2] = forward.z;
        shakenMatrix.data[3] = up.x; shakenMatrix.data[4] = up.y; shakenMatrix.data[5] = up.z;
        shakenMatrix.data[6] = right.x; shakenMatrix.data[7] = right.y; shakenMatrix.data[8] = right.z;
        
        targetMatrix = shakenMatrix;
    }

    if (globals::combat::smoothing) {
        float baseX = std::clamp(1.0f / globals::combat::smoothingx, 0.01f, 1.0f);
        float baseY = std::clamp(1.0f / globals::combat::smoothingy, 0.01f, 1.0f);
        float easedX = applyEasing(baseX, globals::combat::smoothingstyle);
        float easedY = applyEasing(baseY, globals::combat::smoothingstyle);
        // Apply camlock sensitivity as a multiplier to the easing factor (only if sensitivity is enabled)
        // Ensure sensitivity doesn't make movement too slow (minimum 0.1f to prevent excessive smoothness)
        float sens = globals::combat::sensitivity_enabled ? std::clamp(globals::combat::cam_sensitivity, 0.1f, 5.0f) : 1.0f;
        easedX = std::clamp(easedX * sens, 0.0f, 1.0f);
        easedY = std::clamp(easedY * sens, 0.0f, 1.0f);

        // Prevent shaking when X and Y smoothing differ significantly
        // Use the minimum factor (slower movement) to ensure both axes move at similar rates
        float minFactor = std::min(easedX, easedY);
        // If one axis is much faster than the other, limit the faster one to prevent shaking
        // Allow up to 2x difference, beyond that use the slower rate
        float maxFactor = std::max(easedX, easedY);
        if (maxFactor > minFactor * 2.0f) {
            // If difference is too large, use the slower rate for both to prevent shaking
            easedX = minFactor;
            easedY = minFactor;
        }

        Matrix3 smoothedRotation{};
        for (int i = 0; i < 9; ++i) {
            const float factor = (i % 3 == 0) ? easedX : easedY;
            smoothedRotation.data[i] = currentRotation.data[i] + (targetMatrix.data[i] - currentRotation.data[i]) * factor;
        }
        camera.writeRot(smoothedRotation);
    }
    else {
        // Without smoothing, completely ignore all sensitivity and apply instant lock
        // This ensures no partial movement that could feel like smoothing
        // Force instant lock by directly setting the target rotation
        camera.writeRot(targetMatrix);
    }
}
void performMouseAimbot(const Vector2& screenCoords, HWND robloxWindow) {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(robloxWindow, &cursorPos);

    float deltaX = screenCoords.x - cursorPos.x;
    float deltaY = screenCoords.y - cursorPos.y;

    if (globals::combat::smoothing) {
        // Inverted: higher smoothingx = more smoothing (lower factor)
        // Use 1/smoothingx like cem, but with 0.05f scaling for mouse
        float baseX = std::clamp(1.0f / (globals::combat::smoothingx * 0.05f), 0.01f, 1.0f);
        float baseY = std::clamp(1.0f / (globals::combat::smoothingy * 0.05f), 0.01f, 1.0f);
        float easedX = applyEasing(baseX, globals::combat::smoothingstyle);
        float easedY = applyEasing(baseY, globals::combat::smoothingstyle);

        float dpiAdjustedSensitivity = mouseSettings.getDPIAdjustedSensitivity();
        // Apply DPI and user mouselock sensitivity (only if sensitivity is enabled)
        float userSens = globals::combat::sensitivity_enabled ? std::clamp(globals::combat::mouse_sensitivity, 0.01f, 5.0f) : 1.0f;
        easedX = std::clamp(easedX * dpiAdjustedSensitivity * userSens, 0.0f, 1.0f);
        easedY = std::clamp(easedY * dpiAdjustedSensitivity * userSens, 0.0f, 1.0f);

        deltaX *= easedX;
        deltaY *= easedY;
    }
    else {
        // Without smoothing, completely ignore all sensitivity and DPI adjustments for instant lock
        // deltaX and deltaY remain unchanged for truly instant movement
    }

    // Apply minimum movement threshold to prevent micro-jitters
    const float minMovementThreshold = 0.5f;
    if (std::isfinite(deltaX) && std::isfinite(deltaY) && (abs(deltaX) > minMovementThreshold || abs(deltaY) > minMovementThreshold)) {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dx = static_cast<LONG>(deltaX);
        input.mi.dy = static_cast<LONG>(deltaY);
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(input));
    }
}

// Helper function to get target closest to camera
roblox::player gettargetclosesttocamera() {
    // Add bots to the player list
    std::vector<roblox::player> bots = GetBots(base_address);
    std::vector<roblox::player> all_players = globals::instances::cachedplayers;
    all_players.insert(all_players.end(), bots.begin(), bots.end());
    const auto& players = all_players;
    
    if (players.empty()) return {};

    const bool useKnockCheck = (*globals::combat::flags)[1] != 0;
    const bool useHealthCheck = (*globals::combat::flags)[3] != 0;
    const bool useWallCheck = (*globals::combat::flags)[4] != 0;
    const bool useTeamCheck = (*globals::combat::flags)[0] != 0;
    const bool useRangeCheck = (*globals::combat::flags)[2] != 0;
    const bool useFov = globals::combat::usefov;
    const float fovSize = globals::combat::fovsize;
    const float fovSizeSquared = fovSize * fovSize;
    const float healthThreshold = globals::combat::healththreshhold;
    const bool isArsenal = (globals::instances::gamename == "Arsenal");
    const std::string& localPlayerName = globals::instances::localplayer.get_name();
    const Vector3 cameraPos = globals::instances::camera.getPos();

    roblox::player closest = {};
    float closestDistanceSquared = 9e18f;

    for (auto player : players) {
        if (!is_valid_address(player.main.address) || !is_valid_address(player.head.address)) continue;
        if (player.name == localPlayerName) continue;

        // Arsenal-specific checks (from seizure)
        if (isArsenal) {
            auto nrpbs = player.main.findfirstchild("NRPBS");
            if (nrpbs.address) {
                auto health = nrpbs.findfirstchild("Health");
                if (health.address && (health.read_double_value() == 0.0 || player.hrp.get_pos().y < 0.0f)) {
                    continue;
                }
            }
        }

        // BodyEffects check (from seizure)
        auto bodyEffects = player.instance.findfirstchild("BodyEffects");
        if (bodyEffects.address) {
            if (bodyEffects.findfirstchild("Dead").read_bool_value()) continue;
            if (useKnockCheck) {
                // Check for knocked (K.O flag)
                if (bodyEffects.findfirstchild("K.O").read_bool_value()) continue;
                // Check for dead (Dead flag)
                if (bodyEffects.findfirstchild("Dead").read_bool_value()) continue;
            }
        }

        // Health check (from seizure) - works with custom models
        if (useHealthCheck) {
            int playerHealth = player.health;
            // Fallback: read from humanoid if health is 0 or not set (for custom models)
            if (playerHealth == 0 && player.humanoid.address) {
                playerHealth = player.humanoid.read_health();
            }
            if (playerHealth <= static_cast<int>(healthThreshold)) continue;
        }

        // Range check (from seizure) - works with custom models
        if (useRangeCheck) {
            Vector3 targetPos = player.hrp.get_pos();
            float distance = CalculateDistance1(cameraPos, targetPos);
            if (distance > globals::combat::range) continue;
        }

        // Crew check - skip players in same roblox group/crew
        if (globals::combat::crew_check) {
            if (isSameCrew(player)) {
                continue; // Same crew/group, skip
            }
        }
        
        // Team check (from seizure) - works with custom models
        if (useTeamCheck) {
            roblox::instance localPlayerTeam = globals::instances::lp.team;
            if (localPlayerTeam.address != 0 && player.team.address != 0) {
                if (localPlayerTeam.address == player.team.address) {
                    continue; // Same team, skip
                }
            }
        }

        // Forcefield check (from seizure)
        const bool useforcefieldcheck = (*globals::combat::flags)[6];
        bool hasaforcefield = false;
        if (useforcefieldcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "ForceField") {
                    hasaforcefield = true;
                    break;
                }
            }
            if (hasaforcefield) {
                continue;
            }
        }

        // Grabbed check (from seizure - uses GRABBING_CONSTRAINT)
        const bool usegrabbedcheck = (*globals::combat::flags)[5];
        bool isgrabbed = false;
        if (usegrabbedcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "GRABBING_CONSTRAINT") {
                    isgrabbed = true;
                    break;
                }
            }
            if (isgrabbed) {
                continue;
            }
        }

        // Wall check (from seizure)
        if (useWallCheck && !wallcheck::can_see(player.head.get_pos(), cameraPos)) {
            continue;
        }

        Vector3 targetPos = player.hrp.get_pos();
        float distanceSquared = CalculateDistance1(cameraPos, targetPos);

        // FOV check
        if (useFov) {
            Vector2 screenPos = roblox::worldtoscreen(targetPos);
            if (screenPos.x == -1.0f || screenPos.y == -1.0f) continue;
            
            Vector2 screenCenter = { 960.0f, 540.0f }; // Assuming 1920x1080, adjust as needed
            float screenDistanceSquared = (screenPos.x - screenCenter.x) * (screenPos.x - screenCenter.x) + 
                                        (screenPos.y - screenCenter.y) * (screenPos.y - screenCenter.y);
            if (screenDistanceSquared > fovSizeSquared) continue;
        }

        // No wall check - treat all targets as visible
        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closest = player;
        }
    }

    // Return closest target
    return closest;
}

roblox::player gettargetclosesttomouse() {
    static HWND robloxWindow = nullptr;
    static auto lastWindowCheck = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    if (!robloxWindow || std::chrono::duration_cast<std::chrono::seconds>(now - lastWindowCheck).count() > 5) {
        robloxWindow = FindWindowA(nullptr, "Roblox");
        lastWindowCheck = now;
    }

    if (!robloxWindow) return {};

    POINT point;
    if (!GetCursorPos(&point) || !ScreenToClient(robloxWindow, &point)) {
        return {};
    }

    const Vector2 curpos = { static_cast<float>(point.x), static_cast<float>(point.y) };
    
    // Add bots to the player list
    std::vector<roblox::player> bots = GetBots(base_address);
    std::vector<roblox::player> all_players = globals::instances::cachedplayers;
    all_players.insert(all_players.end(), bots.begin(), bots.end());
    const auto& players = all_players;

    if (players.empty()) return {};

    const bool useKnockCheck = (*globals::combat::flags)[1] != 0;
    const bool useHealthCheck = (*globals::combat::flags)[3] != 0;
    const bool useWallCheck = (*globals::combat::flags)[4] != 0;
    const bool useTeamCheck = (*globals::combat::flags)[0] != 0;
    const bool useRangeCheck = (*globals::combat::flags)[2] != 0;
    const bool allowWallFallback = true; // fallback built-in; UI toggle optional
    const bool useFov = globals::combat::usefov;
    const float fovSize = globals::combat::fovsize;
    const float fovSizeSquared = fovSize * fovSize;
    const float healthThreshold = globals::combat::healththreshhold;
    const bool isArsenal = (globals::instances::gamename == "Arsenal");
    const std::string& localPlayerName = globals::instances::localplayer.get_name();
    const Vector3 cameraPos = globals::instances::camera.getPos();

    roblox::player closest = {};
    float closestDistanceSquared = 9e18f;

    for (auto player : players) {
        if (!is_valid_address(player.head.address) ||
            player.name == localPlayerName ||
            player.head.address == 0) {
            continue;
        }

        // Skip whitelisted players (user-chosen: aimbot should not target them)
        if (std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), player.name) != globals::instances::whitelist.end()) {
            continue;
        }

        const Vector2 screenCoords = roblox::worldtoscreen(player.head.get_pos());
        if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) continue;

        const float dx = curpos.x - screenCoords.x;
        const float dy = curpos.y - screenCoords.y;
        const float distanceSquared = dx * dx + dy * dy;

        if (useFov && distanceSquared > fovSizeSquared) continue;

        // Arsenal-specific checks (from seizure)
        if (isArsenal) {
            auto nrpbs = player.main.findfirstchild("NRPBS");
            if (nrpbs.address) {
                auto health = nrpbs.findfirstchild("Health");
                if (health.address && (health.read_double_value() == 0.0 || player.hrp.get_pos().y < 0.0f)) {
                    continue;
                }
            }
        }

        // BodyEffects check (from seizure)
        auto bodyEffects = player.instance.findfirstchild("BodyEffects");
        if (bodyEffects.address) {
            if (bodyEffects.findfirstchild("Dead").read_bool_value()) continue;
            if (useKnockCheck) {
                // Check for knocked (K.O flag)
                if (bodyEffects.findfirstchild("K.O").read_bool_value()) continue;
                // Check for dead (Dead flag)
                if (bodyEffects.findfirstchild("Dead").read_bool_value()) continue;
            }
        }

        // Health check (from seizure) - works with custom models
        if (useHealthCheck) {
            int playerHealth = player.health;
            // Fallback: read from humanoid if health is 0 or not set (for custom models)
            if (playerHealth == 0 && player.humanoid.address) {
                playerHealth = player.humanoid.read_health();
            }
            if (playerHealth <= static_cast<int>(healthThreshold)) continue;
        }

        // Crew check - skip players in same roblox group/crew
        if (globals::combat::crew_check) {
            if (isSameCrew(player)) {
                continue; // Same crew/group, skip
            }
        }
        
        // Forcefield check (from seizure)
        const bool useforcefieldcheck = (*globals::combat::flags)[6];
        bool hasaforcefield = false;
        if (useforcefieldcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "ForceField") {
                    hasaforcefield = true;
                    break;
                }
            }
            if (hasaforcefield) {
                continue;
            }
        }

        // Grabbed check (from seizure - uses GRABBING_CONSTRAINT)
        const bool usegrabbedcheck = (*globals::combat::flags)[5];
        bool isgrabbed = false;
        if (usegrabbedcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "GRABBING_CONSTRAINT") {
                    isgrabbed = true;
                    break;
                }
            }
            if (isgrabbed) {
                continue;
            }
        }

        // Wall check (from seizure)
        if (useWallCheck && !wallcheck::can_see(player.head.get_pos(), cameraPos)) {
            continue;
        }

        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closest = player;
        }
    }

    return closest;
}

void hooks::aimbot() {
    if (!mouseSettings.dpiAutoDetected) {
        detectMouseDPI();
    }

    HWND robloxWindow = FindWindowA(0, "Roblox");
    roblox::player target;

    while (true) {
        globals::combat::aimbotkeybind.update();

        if (!globals::focused || !globals::combat::aimbot || !globals::combat::aimbotkeybind.enabled) {
            foundTarget = false;
            target = {};
            // Clear aimbot lock state when aimbot is disabled
            globals::combat::aimbot_locked = false;
            globals::combat::aimbot_current_target = {};
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Strong sticky aim: when enabled, keep target locked and skip all validation checks
        if (!globals::combat::stickyaim || !foundTarget) {
            // Use target method to determine which function to call
            if (globals::combat::target_method == 0) {
                target = gettargetclosesttomouse();
            } else {
                target = gettargetclosesttocamera();
            }
            
            if (is_valid_address(target.main.address) && target.head.address != 0) {
                foundTarget = true;
            }
            else {
                // No valid target found, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // If sticky aim is enabled and we have a target, skip ALL validation checks for stronger lock
        if (globals::combat::stickyaim && foundTarget) {
            // Just validate that the target part still exists (basic check)
            roblox::instance targetPart;
            
            // Check if airpart is enabled and target is jumping
            if (globals::combat::airpart_enabled && isTargetJumping(target)) {
                targetPart = getTargetPart(target, globals::combat::airpart);
            } else {
                targetPart = getTargetPart(target, globals::combat::aimpart);
            }
            
            if (targetPart.address == 0) {
                // Target part is invalid, but with sticky aim we keep trying - use HRP as fallback
                if (target.hrp.address != 0) {
                    targetPart = target.hrp;
                } else {
                    // No valid part at all, skip this iteration but keep target locked
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }
            
            Vector3 targetPos = targetPart.get_pos();
            Vector3 predictedPos = targetPos;
            
            // Apply prediction if enabled
            if (globals::combat::predictions) {
                Vector3 velocity = targetPart.get_velocity();
                Vector3 veloVector = DivVector3(velocity, { globals::combat::predictionsx, globals::combat::predictionsy, globals::combat::predictionsx });
                predictedPos = AddVector3(targetPos, veloVector);
            }
            
            Vector2 screenCoords = roblox::worldtoscreen(predictedPos);
            
            // With sticky aim, aim at target even if off-screen (stronger lock)
            if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) {
                // Target is off screen, but with sticky aim we still aim at it
                screenCoords = roblox::worldtoscreen(targetPos);
                if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) {
                    // Still invalid, skip this iteration but keep target locked
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }
            
            // Aim at the target (skip FOV, wall, range, health, team checks with sticky aim)
            if (globals::combat::aimbottype == 0) {
                roblox::camera camera = globals::instances::camera;
                Vector3 cameraPos = camera.getPos();
                performCameraAimbot(predictedPos, cameraPos);
                if (globals::combat::smoothing) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            else if (globals::combat::aimbottype == 1) {
                performMouseAimbot(screenCoords, robloxWindow);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else if (globals::combat::aimbottype == 2) {
                static Vector3 lastMemoryTargetPos = {0, 0, 0};
                static bool hasMemoryTarget = false;
                
                if (!hasMemoryTarget) {
                    lastMemoryTargetPos = predictedPos;
                    hasMemoryTarget = true;
                }
                
                roblox::camera camera = globals::instances::camera;
                Vector3 cameraPos = camera.getPos();
                performCameraAimbot(lastMemoryTargetPos, cameraPos);
                
                if (globals::combat::smoothing || globals::combat::predictions) {
                    float memoryLerpSpeed = 0.1f;
                    lastMemoryTargetPos = lerp_vector3(lastMemoryTargetPos, predictedPos, memoryLerpSpeed);
                } else {
                    lastMemoryTargetPos = predictedPos;
                }
                
                if (globals::combat::smoothing) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            
            if (target.head.address) {
                globals::instances::cachedtarget = target;
                globals::instances::cachedlasttarget = target;
                globals::combat::aimbot_locked = true;
                globals::combat::aimbot_current_target = target;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue; // Skip all validation checks when sticky aim is enabled
        }

        // Determine target part with airpart support
        roblox::instance targetPart;
        
        // Check if airpart is enabled and target is jumping
        if (globals::combat::airpart_enabled && isTargetJumping(target)) {
            // Use airpart when target is jumping
            targetPart = getTargetPart(target, globals::combat::airpart);
        } else {
            // Use regular aimpart when target is not jumping or airpart is disabled
            targetPart = getTargetPart(target, globals::combat::aimpart);
        }

        if (targetPart.address == 0) {
            foundTarget = false;
            continue;
        }

        Vector3 targetPos = targetPart.get_pos();

        // Arsenal-specific death check - from layuh22
        if (globals::instances::gamename == "Arsenal" && globals::combat::unlockondeath) {
            if (target.main.findfirstchild("NRPBS").findfirstchild("Health").read_double_value() == 0 || target.hrp.get_pos().y < 0) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                continue; // Continue to re-acquire a new target
            }
        }

        // Generic death checks (BodyEffects.Dead or Humanoid health <= 0)
        // Only unlock from dead targets if unlockondeath is enabled
        // Works for both regular players and custom models
        if (globals::combat::unlockondeath) {
            bool targetDead = false;
            // Use target.bodyeffects if available (works for both regular players and custom models)
            auto bodyEffects = target.bodyeffects;
            if (!bodyEffects.address) {
                // Fallback: try to find BodyEffects from instance (for custom models)
                bodyEffects = target.instance.findfirstchild("BodyEffects");
            }
            if (bodyEffects.address) {
                try {
                    auto deadFlag = bodyEffects.findfirstchild("Dead");
                    if (deadFlag.address && deadFlag.read_bool_value()) {
                        targetDead = true;
                    }
                } catch (...) {
                    // If we can't read BodyEffects, continue
                }
            }
            if (target.humanoid.address) {
                try {
                    int targetHealth = target.humanoid.read_health();
                    if (targetHealth <= 0) {
                        targetDead = true;
                    }
                } catch (...) {
                    // If we can't read health, continue
                }
            }
            if (targetDead) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                continue; // Continue to re-acquire a new target
            }
        }

        // Knock check (from seizure) - unlock when knocked or dead (don't auto-switch)
        if ((*globals::combat::flags)[1] != 0) {
            auto bodyEffects = target.instance.findfirstchild("BodyEffects");
            if (bodyEffects.address) {
                // Check for knocked (K.O flag)
                if (bodyEffects.findfirstchild("K.O").read_bool_value()) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                // Check for dead (Dead flag)
                if (bodyEffects.findfirstchild("Dead").read_bool_value()) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
            // Also check health <= 0 as death indicator
            int targetHealth = target.health;
            if (targetHealth == 0 && target.humanoid.address) {
                targetHealth = target.humanoid.read_health();
            }
            if (targetHealth == 0) {
                auto bodyEffects = target.instance.findfirstchild("BodyEffects");
                if (bodyEffects.address) {
                    auto health = bodyEffects.findfirstchild("Health");
                    if (health.address) {
                        targetHealth = static_cast<int>(health.read_double_value());
                    }
                }
            }
            if (targetHealth <= 0) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            // Check if HRP is below ground (death indicator)
            Vector3 hrpPos = target.hrp.get_pos();
            if (hrpPos.y < -50.0f) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Range check (from seizure) - unlock if target is too far (don't auto-switch)
        if ((*globals::combat::flags)[2] != 0) {
            Vector3 cameraPos = globals::instances::camera.getPos();
            float distance = CalculateDistance1(cameraPos, targetPos);
            if (distance > globals::combat::range) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Crew check - unlock if same crew/group (don't auto-switch)
        if (globals::combat::crew_check) {
            if (isSameCrew(target)) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }
        
        // Team check (from seizure) - unlock if same team (don't auto-switch)
        if ((*globals::combat::flags)[0] != 0) {
            roblox::instance localPlayerTeam = globals::instances::lp.team;
            if (localPlayerTeam.address != 0 && target.team.address != 0) {
                if (localPlayerTeam.address == target.team.address) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
        }

        // Wall check (from seizure) - unlock if wall is blocking (don't auto-switch)
        if ((*globals::combat::flags)[4] != 0) {
            if (!wallcheck::can_see(target.head.get_pos(), globals::instances::camera.getPos())) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Health check - MUST run right before camera lock to break it immediately (don't auto-switch)
        // Read fresh health every frame to catch real-time changes
        if ((*globals::combat::flags)[3] != 0) {
            int targetHealth = 0;
            // Try reading from humanoid first (most reliable)
            if (target.humanoid.address) {
                targetHealth = target.humanoid.read_health();
            }
            // Fallback: try reading from BodyEffects for custom models
            if (targetHealth == 0) {
                auto bodyEffects = target.instance.findfirstchild("BodyEffects");
                if (bodyEffects.address) {
                    auto health = bodyEffects.findfirstchild("Health");
                    if (health.address) {
                        targetHealth = static_cast<int>(health.read_double_value());
                    }
                }
            }
            // Fallback: use cached health if available
            if (targetHealth == 0) {
                targetHealth = target.health;
            }
            // Unlock immediately if health is at or below threshold - BREAKS CAMERA LOCK
            if (targetHealth > 0 && targetHealth <= static_cast<int>(globals::combat::healththreshhold)) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue; // Immediately break camera lock and skip aiming
            }
        }

        Vector3 predictedPos = targetPos;
        if (globals::combat::predictions) {
            Vector3 velocity = targetPart.get_velocity();
            Vector3 veloVector = DivVector3(velocity, { globals::combat::predictionsx, globals::combat::predictionsy, globals::combat::predictionsx });
            predictedPos = AddVector3(targetPos, veloVector);
        } else {
            // Ensure no prediction is applied when disabled
            predictedPos = targetPos;
        }

        Vector2 screenCoords = roblox::worldtoscreen(predictedPos);
        if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) {
            // With sticky aim, keep target locked even if off-screen
            if (globals::combat::stickyaim) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            } else {
                foundTarget = false;
                continue;
            }
        }

        if (globals::combat::aimbottype == 0) {
            roblox::camera camera = globals::instances::camera;
            Vector3 cameraPos = camera.getPos();
            performCameraAimbot(predictedPos, cameraPos);
            if (globals::combat::smoothing) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        else if (globals::combat::aimbottype == 1) {
            performMouseAimbot(screenCoords, robloxWindow);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else if (globals::combat::aimbottype == 2) {
            // Memory Aimbot: Store target position in memory and use it for aimbot
            static Vector3 lastMemoryTargetPos = {0, 0, 0};
            static bool hasMemoryTarget = false;
            
            if (!hasMemoryTarget) {
                lastMemoryTargetPos = predictedPos;
                hasMemoryTarget = true;
            }
            
            // Use stored memory position for aiming
            roblox::camera camera = globals::instances::camera;
            Vector3 cameraPos = camera.getPos();
            performCameraAimbot(lastMemoryTargetPos, cameraPos);
            
            // Update memory position - only apply lerp when smoothing/prediction is enabled
            if (globals::combat::smoothing || globals::combat::predictions) {
                float memoryLerpSpeed = 0.1f; // Adjust this value for memory persistence
                lastMemoryTargetPos = lerp_vector3(lastMemoryTargetPos, predictedPos, memoryLerpSpeed);
            } else {
                // Direct update when no smoothing or prediction
                lastMemoryTargetPos = predictedPos;
            }
            
            if (globals::combat::smoothing) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (target.head.address) {
            globals::instances::cachedtarget = target;
            globals::instances::cachedlasttarget = target;
            // Update global aimbot state for silent aim connection
            globals::combat::aimbot_locked = true;
            globals::combat::aimbot_current_target = target;
        } else {
            // No target, clear aimbot lock state
            globals::combat::aimbot_locked = false;
            globals::combat::aimbot_current_target = {};
        }
    }
}