

#include <windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <wininet.h>

#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include "../../main.h"
#include "../../../globals/globals.hpp"
#include "../../../misc/Umodule/Umodule.hpp"
#include "offsets/offsets.hpp"

RBX::Instance RBX::Instance::GetLighting() const {
    return globals::game.FindFirstChildOfClass(("Lighting"));
}

void RBX::Instance::Setbrightness(float brightness) {
    Umodule::write<float>(this->address + Offsets::Lighting::brightness, brightness);
}

void RBX::Instance::SetFogEnd(float FogEnd) {
    Umodule::write<float>(this->address + Offsets::Lighting::fogend, FogEnd);
}

void RBX::Instance::SetFogStart(float FogStart) {
    Umodule::write<float>(this->address + Offsets::Lighting::fogstart, FogStart);
}

void RBX::Instance::SetAmbience(RBX::Vector3 Color) {
    Umodule::write<RBX::Vector3>(this->address + Offsets::Lighting::ambient, Color);
}

void RBX::Instance::SetColorShiftTop(RBX::Vector3 Color) {
    Umodule::write<RBX::Vector3>(this->address + Offsets::Lighting::colorshift_top, Color);
}

void RBX::Instance::SetColorShiftBottom(RBX::Vector3 Color) {
    Umodule::write<RBX::Vector3>(this->address + Offsets::Lighting::colorshift_bottom, Color);
}

void RBX::Instance::SetBloomIntensity(float Power) {
    Umodule::write<int>(
        globals::lighting.FindFirstChild("Bloom").address + Offsets::Lighting::intensity, Power);
}

void RBX::Instance::SetBloomSize(float Size) {
    Umodule::write<int>(
        globals::lighting.FindFirstChild("Bloom").address + Offsets::Lighting::size, Size);
}

void RBX::Instance::SetBloomThreshHold(float ThreshHold) {
    Umodule::write<int>(
        globals::lighting.FindFirstChild("Bloom").address + Offsets::Lighting::threshold, ThreshHold);
}

void RBX::Instance::SetSunRayIntensity(float Intensity) {
    Umodule::write<int>(
        globals::lighting.FindFirstChild("Sunrays").address + Offsets::Lighting::intensity, Intensity);
}

void RBX::Instance::SetSunRaySpread(float Spread) {
    Umodule::write<float>(
        globals::lighting.FindFirstChild("Sunrays").address + Offsets::Lighting::spread, Spread);
}

int RBX::Instance::GetColor3() {
    return Umodule::read<int>(this->address + Offsets::Lighting::color);
}

void RBX::Instance::setColor3(RBX::Vector3 color) {
    Umodule::write<RBX::Vector3>(this->address + Offsets::Lighting::color, (color));
}
// Env.cpp