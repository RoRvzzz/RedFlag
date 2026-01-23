#pragma once
#define NOMINMAX
#include "../classes/classes.h"
#include "../../drawing/imgui/imgui.h"
#include <vector>
#include <string>
#include <unordered_map>

struct WeaponTexture {
    std::string weapon_name;
    std::string texture_id;
};

struct SkinData {
    std::string name;
    std::vector<WeaponTexture> weapons;
};

class SkinChanger {
public:
    SkinChanger();
    ~SkinChanger();

    void Initialize();
    void Update();
    
    // Get skin list for dropdown
    std::vector<std::string> GetSkinList() const;
    int GetSelectedSkinIndex() const { return selected_skin_index; }
    void SetSelectedSkinIndex(int index);
    
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool enable) { enabled = enable; }

private:
    bool enabled = false;
    int selected_skin_index = 0; // 0=Galaxy, 1=Inferno, 2=Matrix, 3=RedDeath, 4=GoldGlory
    
    std::vector<SkinData> skins;
    
    // Helper functions
    void InitializeSkins();
    void ApplySkin(int skin_index);
    roblox::instance FindWeaponInCharacter(const std::string& weapon_name);
    roblox::instance FindWeaponInBackpack(const std::string& weapon_name);
    roblox::instance FindWeaponInStarterGear(const std::string& weapon_name);
    void SetWeaponTexture(roblox::instance weapon, const std::string& texture_id);
    bool GetCharacter();
    bool GetBackpack();
    bool GetStarterGear();
    
    roblox::instance character;
    roblox::instance backpack;
    roblox::instance starter_gear;
};

