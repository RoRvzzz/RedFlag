
#include <windows.h> 
#include <TlHelp32.h> 
#include <string> 
#include <iostream> 

#include <windows.h>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <wininet.h>
#include <string>
#include <ShlObj.h>
#include <functional>
#include "../../main.h"
#include "../../../misc/Umodule/Umodule.hpp"
#include "../../../globals/globals.hpp"

void syncList22(std::vector<RBX::WorkSpaceInstance>& oldList, const std::vector<RBX::WorkSpaceInstance>& newList) {
    ::SetThreadPriority(::GetCurrentThread(), 0x80);
    if (newList.empty()) return;
    std::unordered_map<uintptr_t, RBX::WorkSpaceInstance> newEntitiesMap;
    for (const auto& entity : newList)
        newEntitiesMap[entity.address] = entity;
    oldList.erase(
        std::remove_if(oldList.begin(), oldList.end(),
            [&newEntitiesMap](const RBX::WorkSpaceInstance& item) {
                return newEntitiesMap.find(item.address) == newEntitiesMap.end();
            }),
        oldList.end()
    );
    for (const auto& entity : newList) {
        auto it = std::find_if(oldList.begin(), oldList.end(),
            [&entity](const RBX::WorkSpaceInstance& item) {
                if (item.address == entity.address)
                return  item.address == entity.address;
            });
        if (it != oldList.end())
            *it = entity;
        else
            oldList.push_back(entity);
    }
}

namespace RBX {

    int Instance::fetchPlayer(std::uint64_t address) const {
        return Umodule::read<int>(address);
    }

    std::atomic<bool> running{ true };
    std::mutex cachedPlayersMutex;

    void syncList(std::vector<RBX::PlayerInstance>& oldList, const std::vector<RBX::PlayerInstance>& newList) {
        ::SetThreadPriority(::GetCurrentThread(), 0x80);
        if (newList.empty()) return;
        std::unordered_map<uintptr_t, RBX::PlayerInstance> newEntitiesMap;
        for (const auto& entity : newList)
            newEntitiesMap[entity.address] = entity;
        oldList.erase(
            std::remove_if(oldList.begin(), oldList.end(),
                [&newEntitiesMap](const RBX::PlayerInstance& item) {
                    return newEntitiesMap.find(item.address) == newEntitiesMap.end();
                }),
            oldList.end()
        );
        for (const auto& entity : newList) {
            auto it = std::find_if(oldList.begin(), oldList.end(),
                [&entity](const RBX::PlayerInstance& item) {
                    return item.address == entity.address;
                });
            if (it != oldList.end())
                *it = entity;
            else
                oldList.push_back(entity);
        }
    }
    bool isOnScreen(const RBX::PlayerInstance& player) {
        auto dimensions = globals::visualengine.GetDimensions();
        auto playerPosition = player.character.GetPosition();
        auto screenPosition = RBX::WorldToScreen(playerPosition, dimensions, globals::visualengine.GetViewMatrix());

        return (screenPosition.x >= 0 && screenPosition.x <= dimensions.x) &&
            (screenPosition.y >= 0 && screenPosition.y <= dimensions.y);
    }

    void Instance::updatePlayers() {

        std::unordered_set<std::uintptr_t> knownAddresses;
        auto& entityPool = globals::players;
        std::mutex updateMutex;

        std::vector<RBX::PlayerInstance> tempCache;
        tempCache.reserve(entityPool.GetChildren().size());
        globals::camera = globals::workspace.FindFirstChild("Camera");
        globals::mouse_service = globals::game.FindFirstChild("MouseService").address;
   
     
        std::this_thread::sleep_for(std::chrono::microseconds(935));

        while (true) {

            if (entityPool.fetchPlayer(entityPool.address) != globals::mostFreq) {
                std::this_thread::sleep_for(std::chrono::microseconds(935));
                continue;
            }
            RBX::PlayerInstance LocalPlayer;
        
            LocalPlayer.aim = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("MainScreenGui").FindFirstChild("Aim");
            LocalPlayer.jail_aim = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("CrossHairGui").FindFirstChild("CrossHair");;
            LocalPlayer.Crosshair2 = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("WeaponGUI").FindFirstChild("Crosshair");;
            LocalPlayer.hc_aim = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("Main Screen").FindFirstChild("Aim");
            LocalPlayer.Hood_Game = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("MainGui").FindFirstChild("Aim");
            LocalPlayer.weapon_aim = globals::players.GetLocalPlayer().FindFirstChild("PlayerGui").FindFirstChild("Crosshair").FindFirstChild("Main");
            LocalPlayer.team = globals::players.GetLocalPlayer().GetTeam();

            LocalPlayer.name = globals::players.GetLocalPlayer().GetName();
            //  std::this_thread::sleep_for(std::chrono::microseconds(935));
            LocalPlayer.character = globals::players.GetLocalPlayer().GetModelInstance();
            std::this_thread::sleep_for(std::chrono::microseconds(globals::threadrestarttime));
            auto humanoidData = LocalPlayer.character.FindFirstChild("Humanoid");
            if (LocalPlayer.character.address == 0 || humanoidData.address == 0) {
                continue;
            }

            LocalPlayer.humanoid = humanoidData;
            LocalPlayer.team = globals::players.GetLocalPlayer().GetTeam();
            LocalPlayer.address = globals::players.GetLocalPlayer().address;
            LocalPlayer.children = LocalPlayer.character.GetChildren();
            LocalPlayer.r15 = humanoidData.GetRigType();
            LocalPlayer.health = humanoidData.GetHealth();
            LocalPlayer.maxhealth = humanoidData.GetMaxHealth();
            LocalPlayer.currentToolName = LocalPlayer.character.FindFirstChildOfClass("Tool").GetName();
            globals::localplayer = LocalPlayer;
            knownAddresses.clear();
            tempCache.clear();

            auto entityList = entityPool.GetChildren();

            std::unordered_map<std::uintptr_t, RBX::PlayerInstance> partCache;
        //    std::this_thread::sleep_for(std::chrono::microseconds(935));
            for (const auto& entityInstance : entityList) {
                if (!globals::localplayercheck) {
                    if (entityInstance.address == globals::players.GetLocalPlayer().address)
                        continue;
      }
              
                RBX::PlayerInstance currentEntity;
                currentEntity.name = entityInstance.GetName();
              //  std::this_thread::sleep_for(std::chrono::microseconds(935));
                currentEntity.character = entityInstance.GetModelInstance();
                std::this_thread::sleep_for(std::chrono::microseconds(globals::threadrestarttime));
                auto humanoidData = currentEntity.character.FindFirstChild("Humanoid");
                if (currentEntity.character.address == 0 || humanoidData.address == 0) {
                    continue;
                }

                currentEntity.humanoid = humanoidData;
                currentEntity.team = entityInstance.GetTeam();
                currentEntity.address = entityInstance.address;
                currentEntity.children = currentEntity.character.GetChildren();
                currentEntity.r15 = humanoidData.GetRigType();
                currentEntity.health = humanoidData.GetHealth();
                currentEntity.maxhealth = humanoidData.GetMaxHealth();

                currentEntity.bodyEffects = currentEntity.character.FindFirstChild(("BodyEffects"));
              
                if (globals::shield_bar) {
                    currentEntity.armor_obj = currentEntity.bodyEffects.FindFirstChild("Armor");
                currentEntity.shield = currentEntity.armor_obj.getIntFromValue();
                }
                if (globals::tool_esp) {
                    currentEntity.currentTool = currentEntity.character.FindFirstChildOfClass("Tool");
                    currentEntity.currentToolName = currentEntity.character.FindFirstChildOfClass("Tool").GetName();
                }
                if ((!globals::aimbot_checks.empty() || !globals::esp_checks.empty() || !globals::silent_aim_checks.empty() && globals::aimbot_checks[0] && globals::esp_checks[0] && globals::silent_aim_checks[0])) {
                    if (!globals::aimbot_checks.empty() && globals::aimbot_checks[0]) {
                        currentEntity.knockedOut = currentEntity.bodyEffects.FindFirstChild(("K.O"));
                        currentEntity.ifGrabbed = currentEntity.bodyEffects.FindFirstChild(("GRABBING_CONSTRAINT"));
                    }
                }
           //     std::this_thread::sleep_for(std::chrono::microseconds(935));
                if (globals::cframe && !isOnScreen(currentEntity)) {
                    continue;
                }

                std::unordered_map<std::string, RBX::Instance&> partMap;

                if (currentEntity.r15 == 0) {
                    partMap = {
                        {"Torso", currentEntity.upperTorso},
                        {"Head", currentEntity.head},
                        {"HumanoidRootPart", currentEntity.rootPart},
                        {"Right Arm", currentEntity.rightUpperArm},
                        {"Left Arm", currentEntity.leftUpperArm},
                        {"Left Leg", currentEntity.leftUpperLeg},
                        {"Right Leg", currentEntity.rightUpperLeg}
                    };
                }
                else {
                    partMap = {
                        {"Head", currentEntity.head},
                        {"UpperTorso", currentEntity.upperTorso},
                        {"LowerTorso", currentEntity.lowerTorso},
                        {"HumanoidRootPart", currentEntity.rootPart},
                        {"LeftUpperLeg", currentEntity.leftUpperLeg},
                        {"RightUpperLeg", currentEntity.rightUpperLeg},
                        {"LeftLowerLeg", currentEntity.leftLowerLeg},
                        {"RightLowerLeg", currentEntity.rightLowerLeg},
                        {"LeftFoot", currentEntity.leftFoot},
                        {"RightFoot", currentEntity.rightFoot},
                        {"LeftUpperArm", currentEntity.leftUpperArm},
                        {"RightUpperArm", currentEntity.rightUpperArm},
                        {"LeftLowerArm", currentEntity.leftLowerArm},
                        {"RightLowerArm", currentEntity.rightLowerArm},
                        {"LeftHand", currentEntity.leftHand},
                        {"RightHand", currentEntity.rightHand}
                    };
                }
            //    std::this_thread::sleep_for(std::chrono::microseconds(935));
                for (auto& child : currentEntity.character.GetChildren()) {

                    auto it = partMap.find(child.GetName());
                    if (it != partMap.end()) {
                        it->second = child;
                        partCache[child.address] = currentEntity;
                    }
                }

                tempCache.push_back(currentEntity);
                knownAddresses.insert(currentEntity.address);
            }

            try {

                std::lock_guard<std::mutex> lock(updateMutex);

                globals::cached_players.erase(
                    std::remove_if(globals::cached_players.begin(), globals::cached_players.end(),
                        [&knownAddresses](const RBX::PlayerInstance& player) {
                            return knownAddresses.find(player.address) == knownAddresses.end();
                        }),
                    globals::cached_players.end());

                for (const auto& playerInstance : tempCache) {
                    if (knownAddresses.find(playerInstance.address) == knownAddresses.end()) {
                        globals::cached_players.push_back(playerInstance);
                    }
                }

                syncList(globals::cached_players, tempCache);

            }
            catch (const std::exception& e) {
                std::cerr << "Exception caught while updating players: " << e.what() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(globals::threadtime));
        }
    }
}

void RBX::Instance::updateWorkspace() {
    std::unordered_set<std::uintptr_t> knownAddresses;
    auto& workspacePool = globals::workspace;
    std::mutex updateMutex;

    std::vector<RBX::WorkSpaceInstance> tempCache;
    tempCache.reserve(100);
    globals::camera = globals::workspace.FindFirstChild("Camera");
    if (!globals::camera.address)
        globals::camera = globals::workspace.FindFirstChildOfClass("Camera");
    std::this_thread::sleep_for(std::chrono::microseconds(935));

    while (RBX::running.load()) {
        RBX::WorkSpaceInstance currentWorkspace;
        currentWorkspace.address = globals::workspace.address;
        currentWorkspace.children = globals::workspace.GetChildren();
        currentWorkspace.name = RBX::Instance{}.GetWorkspaceByOffset().GetName();

        knownAddresses.clear();
        tempCache.clear();

        std::unordered_map<std::uintptr_t, RBX::WorkSpaceInstance> partCache;

        std::function<void(RBX::WorkSpaceInstance&)> cacheRecursive = [&](RBX::WorkSpaceInstance& workspaceInstance) {
            for (const auto& child : workspaceInstance.children) {
                RBX::WorkSpaceInstance childInstance;
                childInstance.name = child.GetName();
                childInstance.address = child.address;
                childInstance.children = child.GetChildren();
                partCache[childInstance.address] = childInstance;

                cacheRecursive(childInstance);

                tempCache.push_back(childInstance);
                knownAddresses.insert(childInstance.address);
            }
            };

        cacheRecursive(currentWorkspace);

        try {
            std::lock_guard<std::mutex> lock(updateMutex);

            globals::cachedInstances.erase(
                std::remove_if(globals::cachedInstances.begin(), globals::cachedInstances.end(),
                    [&knownAddresses](const RBX::WorkSpaceInstance& workspace) {
                        return knownAddresses.find(workspace.address) == knownAddresses.end();
                    }),
                globals::cachedInstances.end());

            for (const auto& workspaceInstance : tempCache) {
                if (knownAddresses.find(workspaceInstance.address) == knownAddresses.end()) {
                    globals::cachedInstances.push_back(workspaceInstance);
                }
            }

            syncList22(globals::cachedInstances, tempCache);

        }
        catch (const std::exception& e) {
            std::cerr << "Exception caught while updating workspace: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(globals::threadtime));
    }
}