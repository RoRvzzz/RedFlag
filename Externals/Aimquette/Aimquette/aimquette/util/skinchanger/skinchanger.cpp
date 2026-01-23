#define NOMINMAX
#include "skinchanger.h"
#include "../globals.h"
#include "../driver/driver.h"
#include "../offsets.h"
#include <Windows.h>
#include <algorithm>

SkinChanger::SkinChanger() {
    Initialize();
}

SkinChanger::~SkinChanger() {
}

void SkinChanger::Initialize() {
    InitializeSkins();
}

void SkinChanger::InitializeSkins() {
    skins.clear();
    
    // Galaxy Skin
    SkinData galaxy;
    galaxy.name = "Galaxy";
    galaxy.weapons = {
        {"[SilencerAR]", "rbxassetid://9402007158"},
        {"[SMG]", "rbxassetid://9387614760"},
        {"[TacticalShotgun]", "rbxassetid://9402279010"},
        {"[AK47]", "rbxassetid://9402132929"},
        {"[AUG]", "rbxassetid://9401832956"},
        {"[AR]", "rbxassetid://9402007158"},
        {"[Glock]", "rbxassetid://9401709916"},
        {"[Shotgun]", "rbxassetid://9387933478"},
        {"[Silencer]", "rbxassetid://9401709916"},
        {"[P90]", "rbxassetid://9399887933"},
        {"[Revolver]", "rbxassetid://9370936730"},
        {"[RPG]", "rbxassetid://9399842353"},
        {"[LMG]", "rbxassetid://9400170566"},
        {"[Flamethrower]", "rbxassetid://9400558000"},
        {"[DrumGun]", "rbxassetid://9381577172"},
        {"[Double-Barrel SG]", "rbxassetid://9401441647"}
    };
    skins.push_back(galaxy);
    
    // Inferno Skin
    SkinData inferno;
    inferno.name = "Inferno";
    inferno.weapons = {
        {"[SilencerAR]", "rbxassetid://9401972413"},
        {"[SMG]", "rbxassetid://9387593777"},
        {"[TacticalShotgun]", "rbxassetid://9402244359"},
        {"[AK47]", "rbxassetid://9402094255"},
        {"[AUG]", "rbxassetid://9401802930"},
        {"[AR]", "rbxassetid://9401972413"},
        {"[Glock]", "rbxassetid://9401670081"},
        {"[Shotgun]", "rbxassetid://9387831940"},
        {"[Silencer]", "rbxassetid://9401670081"},
        {"[P90]", "rbxassetid://9399878713"},
        {"[Revolver]", "rbxassetid://9370404463"},
        {"[RPG]", "rbxassetid://9399831924"},
        {"[LMG]", "rbxassetid://9400160302"},
        {"[Flamethrower]", "rbxassetid://9400503673"},
        {"[DrumGun]", "rbxassetid://9381496666"},
        {"[Double-Barrel SG]", "rbxassetid://9401416743"}
    };
    skins.push_back(inferno);
    
    // Matrix Skin
    SkinData matrix;
    matrix.name = "Matrix";
    matrix.weapons = {
        {"[SilencerAR]", "rbxassetid://9402023983"},
        {"[SMG]", "rbxassetid://9387681455"},
        {"[TacticalShotgun]", "rbxassetid://9402295362"},
        {"[AK47]", "rbxassetid://9402147406"},
        {"[AUG]", "rbxassetid://9401855319"},
        {"[AR]", "rbxassetid://9402023983"},
        {"[Glock]", "rbxassetid://9401727978"},
        {"[Shotgun]", "rbxassetid://9387945198"},
        {"[Silencer]", "rbxassetid://9401727978"},
        {"[P90]", "rbxassetid://9399894480"},
        {"[Revolver]", "rbxassetid://9380928144"},
        {"[RPG]", "rbxassetid://9399850204"},
        {"[LMG]", "rbxassetid://9400178599"},
        {"[Flamethrower]", "rbxassetid://9400582867"},
        {"[DrumGun]", "rbxassetid://9381601709"},
        {"[Double-Barrel SG]", "rbxassetid://9401457713"}
    };
    skins.push_back(matrix);
    
    // RedDeath Skin
    SkinData reddeath;
    reddeath.name = "RedDeath";
    reddeath.weapons = {
        {"[SilencerAR]", "rbxassetid://8213168054"},
        {"[SMG]", "rbxassetid://8199875638"},
        {"[TacticalShotgun]", "rbxassetid://9203641766"},
        {"[AK47]", "rbxassetid://8213572965"},
        {"[AUG]", "rbxassetid://8212802637"},
        {"[AR]", "rbxassetid://8213168054"},
        {"[Glock]", "rbxassetid://8212637463"},
        {"[Shotgun]", "rbxassetid://8200647420"},
        {"[Silencer]", "rbxassetid://8212637463"},
        {"[P90]", "rbxassetid://8205381104"},
        {"[Revolver]", "rbxassetid://8173928665"},
        {"[RPG]", "rbxassetid://8201055935"},
        {"[LMG]", "rbxassetid://8205713344"},
        {"[Flamethrower]", "rbxassetid://8206707126"},
        {"[DrumGun]", "rbxassetid://8186385983"},
        {"[Double-Barrel SG]", "rbxassetid://8212384179"}
    };
    skins.push_back(reddeath);
    
    // GoldGlory Skin
    SkinData goldglory;
    goldglory.name = "GoldGlory";
    goldglory.weapons = {
        {"[SilencerAR]", "rbxassetid://8213175568"},
        {"[SMG]", "rbxassetid://8199883519"},
        {"[TacticalShotgun]", "rbxassetid://9203647967"},
        {"[AK47]", "rbxassetid://8213606202"},
        {"[AUG]", "rbxassetid://8212809463"},
        {"[AR]", "rbxassetid://8213175568"},
        {"[Glock]", "rbxassetid://8212667115"},
        {"[Shotgun]", "rbxassetid://8200657428"},
        {"[Silencer]", "rbxassetid://8212667115"},
        {"[P90]", "rbxassetid://8205397990"},
        {"[Revolver]", "rbxassetid://8173955378"},
        {"[RPG]", "rbxassetid://8201059812"},
        {"[LMG]", "rbxassetid://8205719479"},
        {"[Flamethrower]", "rbxassetid://8208010648"},
        {"[DrumGun]", "rbxassetid://8186168230"},
        {"[Double-Barrel SG]", "rbxassetid://8212394280"}
    };
    skins.push_back(goldglory);
}

std::vector<std::string> SkinChanger::GetSkinList() const {
    std::vector<std::string> skin_names;
    for (const auto& skin : skins) {
        skin_names.push_back(skin.name);
    }
    return skin_names;
}

void SkinChanger::SetSelectedSkinIndex(int index) {
    if (index >= 0 && index < static_cast<int>(skins.size())) {
        selected_skin_index = index;
    }
}

bool SkinChanger::GetCharacter() {
    if (globals::instances::localplayer.address == 0) {
        return false;
    }
    
    try {
        character = globals::instances::localplayer.findfirstchild("Character");
        return character.address != 0;
    } catch (...) {
        return false;
    }
}

roblox::instance SkinChanger::FindWeaponInCharacter(const std::string& weapon_name) {
    if (character.address == 0) {
        if (!GetCharacter()) {
            return roblox::instance{0};
        }
    }
    
    try {
        return character.findfirstchild(weapon_name);
    } catch (...) {
        return roblox::instance{0};
    }
}

bool SkinChanger::GetBackpack() {
    if (globals::instances::localplayer.address == 0) {
        return false;
    }
    
    try {
        backpack = globals::instances::localplayer.findfirstchild("Backpack");
        return backpack.address != 0;
    } catch (...) {
        return false;
    }
}

bool SkinChanger::GetStarterGear() {
    if (globals::instances::localplayer.address == 0) {
        return false;
    }
    
    try {
        starter_gear = globals::instances::localplayer.findfirstchild("StarterGear");
        return starter_gear.address != 0;
    } catch (...) {
        return false;
    }
}

roblox::instance SkinChanger::FindWeaponInBackpack(const std::string& weapon_name) {
    if (backpack.address == 0) {
        if (!GetBackpack()) {
            return roblox::instance{0};
        }
    }
    
    try {
        return backpack.findfirstchild(weapon_name);
    } catch (...) {
        return roblox::instance{0};
    }
}

roblox::instance SkinChanger::FindWeaponInStarterGear(const std::string& weapon_name) {
    if (starter_gear.address == 0) {
        if (!GetStarterGear()) {
            return roblox::instance{0};
        }
    }
    
    try {
        return starter_gear.findfirstchild(weapon_name);
    } catch (...) {
        return roblox::instance{0};
    }
}

void SkinChanger::SetWeaponTexture(roblox::instance weapon, const std::string& texture_id) {
    if (weapon.address == 0) {
        return;
    }
    
    try {
        // Find "Default" part in weapon
        roblox::instance default_part = weapon.findfirstchild("Default");
        if (default_part.address == 0) {
            // Try without "Default" - maybe the weapon itself is the part
            default_part = weapon;
        }
        
        // TextureID property - try multiple approaches
        // In Roblox, TextureID on Part/MeshPart is typically at the same offset as AnimationId (0xD0)
        // Read the pointer first (same pattern as reading strings)
        uintptr_t texture_ptr_address = default_part.address + offsets::AnimationId; // 0xD0
        uintptr_t texture_ptr = read<uintptr_t>(texture_ptr_address);
        
        if (texture_ptr != 0) {
            // Write string to the pointer location (matches reading pattern)
            bool success = write_string(texture_ptr, texture_id);
            if (success) {
                return;
            }
        }
        
        // Fallback 1: Try DecalTexture offset (0x198)
        texture_ptr_address = default_part.address + offsets::DecalTexture; // 0x198
        texture_ptr = read<uintptr_t>(texture_ptr_address);
        if (texture_ptr != 0) {
            bool success = write_string(texture_ptr, texture_id);
            if (success) {
                return;
            }
        }
        
        // Fallback 2: Try MeshPartTexture offset (0x310) if it's a MeshPart
        texture_ptr_address = default_part.address + offsets::MeshPartTexture; // 0x310
        texture_ptr = read<uintptr_t>(texture_ptr_address);
        if (texture_ptr != 0) {
            bool success = write_string(texture_ptr, texture_id);
            if (success) {
                return;
            }
        }
        
        // Fallback 3: Try direct writes (in case it's not a pointer structure)
        write_string(default_part.address + offsets::AnimationId, texture_id);
        write_string(default_part.address + offsets::DecalTexture, texture_id);
        write_string(default_part.address + offsets::MeshPartTexture, texture_id);
        
        // Fallback 4: Try on primitive if it exists
        uintptr_t primitive = default_part.primitive();
        if (primitive != 0) {
            texture_ptr = read<uintptr_t>(primitive + offsets::AnimationId);
            if (texture_ptr != 0) {
                write_string(texture_ptr, texture_id);
            } else {
                write_string(primitive + offsets::AnimationId, texture_id);
            }
        }
    } catch (...) {
        // Error writing
    }
}

void SkinChanger::ApplySkin(int skin_index) {
    if (skin_index < 0 || skin_index >= static_cast<int>(skins.size())) {
        return;
    }
    
    const SkinData& skin = skins[skin_index];
    
    // Refresh character, backpack, and starter gear
    GetCharacter();
    GetBackpack();
    GetStarterGear();
    
    // Apply textures to all weapons (check character, backpack, and starter gear)
    // Apply to ALL locations where the weapon exists, not just one
    for (const auto& weapon_texture : skin.weapons) {
        // Try character
        roblox::instance weapon = FindWeaponInCharacter(weapon_texture.weapon_name);
        if (weapon.address != 0) {
            SetWeaponTexture(weapon, weapon_texture.texture_id);
        }
        
        // Try backpack (don't continue, apply to all)
        weapon = FindWeaponInBackpack(weapon_texture.weapon_name);
        if (weapon.address != 0) {
            SetWeaponTexture(weapon, weapon_texture.texture_id);
        }
        
        // Try starter gear
        weapon = FindWeaponInStarterGear(weapon_texture.weapon_name);
        if (weapon.address != 0) {
            SetWeaponTexture(weapon, weapon_texture.texture_id);
        }
    }
}

void SkinChanger::Update() {
    if (!enabled) {
        return;
    }
    
    // Apply skin every frame (like the Lua script's render step)
    // Only apply if a valid skin is selected
    if (selected_skin_index >= 0 && selected_skin_index < static_cast<int>(skins.size())) {
        ApplySkin(selected_skin_index);
    }
}

