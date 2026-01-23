#include "../classes.h"

roblox::instance roblox::instance::read_userid() const {
    return read<roblox::instance>(this->address + offsets::UserId);
}
// Gravity manipulation method commented out - method is outdated
// void roblox::instance::writeprimgrav(float gravity) {
//     write<float>(read<uintptr_t>(this->address + offsets::Primitive) + offsets::PrimitiveGravity, gravity);
// }

float roblox::instance::read_primgrav() const {
    return read<float>(read<uintptr_t>(this->address + offsets::Primitive) + offsets::PrimitiveGravity);
}
float roblox::instance::read_walkspeed() const {
    if (!is_valid_address(address)) return 0;
    return read<float>(this->address + offsets::WalkSpeed);
}
void roblox::instance::write_walkspeed(float value) {
     write<float>(this->address + offsets::WalkSpeed, value);
     write<float>(this->address + offsets::WalkSpeedCheck, value);
}
float roblox::instance::read_jumppower() const {
    return read<float>(this->address + offsets::JumpPower);
}
void roblox::instance::write_jumppower(float value) {
    write<float>(this->address + offsets::JumpPower, value);
}

float roblox::instance::read_hipheight() const {
    return read<float>(this->address + offsets::HipHeight);
}
std::string roblox::instance::read_animationid() const {
    return read<std::string>(this->address + offsets::AnimationId);
}
void roblox::instance::write_animationid(std::string animid) {
    if (!this->address) return;
    write<std::string>(this->address + offsets::AnimationId, animid);
}
void roblox::instance::write_hipheight(float value) {
    write<float>(this->address + offsets::HipHeight, value);
}
bool roblox::instance::read_sitting() const {
    return read<bool>(this->address + offsets::Sit);
}
void roblox::instance::write_sitting(bool value) {
    write<bool>(this->address + offsets::Sit, value);
}

// Team checking method implementation
bool roblox::instance::is_same_team(const roblox::instance& other_player) const {
    if (!is_valid_address(address) || !is_valid_address(other_player.address)) return false;
    
    // Read team names and compare
    std::string our_team = this->read_team_name();
    std::string their_team = other_player.read_team_name();
    
    return our_team == their_team && !our_team.empty();
}

// Helper method to read team name
std::string roblox::instance::read_team_name() const {
    if (!is_valid_address(address)) return "";
    
    try {
        auto team_instance = this->Team();
        if (is_valid_address(team_instance.address)) {
            return team_instance.get_name();
        }
    }
    catch (...) {
        // Ignore any exceptions
    }
    
    return "";
}
