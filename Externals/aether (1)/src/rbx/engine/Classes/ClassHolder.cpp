#include "../../main.h"

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
#include "../../../misc/Umodule/Umodule.hpp"
#include "../../../misc/output_system/output/output.hpp"
#include "../math/types/Math/Math.h"
#include "../../../globals/globals.hpp"
#include "../Classes/offsets/offsets.hpp"
std::string RBX::Instance::ReadString(std::uint64_t Address)
{
    constexpr int MaxSafeLength = 200;

    int Length = 0;
    if (!Umodule::safe_read(Address + 0x18, &Length, sizeof(Length)) || Length <= 0 || Length > MaxSafeLength) {
        return "";
    }

    std::uint64_t StringAddress = Address;
    if (Length >= 16) {
        if (!Umodule::safe_read(Address, &StringAddress, sizeof(StringAddress))) {     
            return "";
        }
    }

    std::vector<char> Buffer(static_cast<size_t>(Length) + 1, '\0');
    SIZE_T BytesRead = 0;
    if (!ReadProcessMemory(Umodule::hProcess, reinterpret_cast<LPCVOID>(StringAddress), Buffer.data(), Length, &BytesRead) || BytesRead != static_cast<SIZE_T>(Length)) {
        BytesRead = 0;
        for (int I = 0; I < Length; ++I) {
            char C;
            if (Umodule::safe_read(StringAddress + I, &C, sizeof(char))) {
                Buffer[I] = C;
                ++BytesRead;
            }
            else {
                utils::output::printint("Byte read failed at offset: %d", I);
                break;
            }
        }
        Buffer[BytesRead] = '\0';
    }

    return std::string(Buffer.data());
}


using namespace RBX;




RBX::Instance RBX::Instance::GetVisualEngine() {
    RBX::Instance Instance;
    return static_cast<RBX::Instance>(Umodule::read<std::uint64_t>(Instance.GetRenderView() + Offsets::Visual_Engine));
}

RBX::Vector2 RBX::Instance::GetDimensions()
{
    return Umodule::read<RBX::Vector2>(this->address + Offsets::Dimensions);
}

RBX::Matrix4x4 RBX::Instance::GetViewMatrix()
{
    return Umodule::read<RBX::Matrix4x4>(this->address + Offsets::View_Matrix);
}

float RBX::Instance::GetHealth() const {
    auto one = Umodule::read<std::uint64_t>(this->address + Offsets::Health::Current);
    auto two = Umodule::read<std::uint64_t>(Umodule::read<std::uint64_t>(this->address + Offsets::Health::Current));
    std::uint64_t normalizedHealth = one ^ two;
    float transformedHealth;
    std::memcpy(&transformedHealth, &normalizedHealth, sizeof(transformedHealth));
    return transformedHealth;
}

std::string  RBX::Instance::getPlayerDisplayName() {
    RBX::Instance name = Umodule::read<RBX::Instance>(this->address + 0xC8);
    return name.GetName();
}
void RBX::Instance::SetHumanoidWalkSpeed(float WalkSpeed)
{
    Umodule::write<float>(this->address + Offsets::WalkSpeed1, WalkSpeed);
    Umodule::write<float>(this->address + Offsets::WalkSpeed2, WalkSpeed);
    return;
}


void RBX::Instance::write_double(double value) {
    Umodule::write<double>(this->address + Offsets::Value, value);
}

void RBX::Instance::write_health(float health) {
    Umodule::write<float>(this->address + Offsets::Health::Current, health);
}

void RBX::Instance::write_velocity(Vector3 velo) {
    auto primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Primitive);

    if (primitive) {
        int loopgod = 0;
        while (true) {
            loopgod += 1;
            Umodule::write<Vector3>(primitive + Offsets::Part_Velocity, velo);

            if (loopgod == 250) {
                break;
            }
        }

        Umodule::write<Vector3>(primitive + Offsets::Part_Velocity, velo);
    }
}

void RBX::Instance::write_extra(Vector3 velo) {
    auto primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Primitive);
    for (int lawwtf_bestcoder = 0; lawwtf_bestcoder < 1; lawwtf_bestcoder++) {
        Umodule::write<Vector3>(primitive + Offsets::Part_Velocity, velo);
    }
}

void RBX::Instance::write_cframe(RBX::CFrame newCFrame)
{
    auto primitive = Umodule::read< std::uint64_t >(this->address + Offsets::Primitive);

    if (primitive) {
        int count = 0;
        while (true) {
            count += 1;
            Umodule::write<RBX::CFrame>(primitive + Offsets::CFrame, newCFrame);

            if (count == 250) {
                break;
            }
        }

        Umodule::write<RBX::CFrame>(primitive + Offsets::CFrame, newCFrame);
    }
}



void RBX::write_walkspeed(float value)
{
    auto humanoid_instance = globals::players.GetModelInstance().FindFirstChild("Humanoid");

    if (humanoid_instance.address) {
        Umodule::write<float>(humanoid_instance.address + Offsets::WalkSpeed1, float(value));
        Umodule::write<float>(humanoid_instance.address + Offsets::WalkSpeedCheck, float(value));
    }
    //	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void RBX::walkspeedloop(float value)
{
    auto humanoid_instance = globals::players.GetModelInstance().FindFirstChild("Humanoid");
    for (int i = 0; i < 12500; i++) {
        if (humanoid_instance.address) {
            Umodule::write<float>(humanoid_instance.address + Offsets::WalkSpeed1, float(value));
            Umodule::write<float>(humanoid_instance.address + Offsets::WalkSpeedCheck, float(value));
        }
    }

    //	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}


float RBX::read_walkspeed()
{
    auto humanoid_instance = globals::players.GetModelInstance().FindFirstChild("Humanoid");
    if (humanoid_instance.address) {
        return Umodule::read<float>(humanoid_instance.address + Offsets::WalkSpeed1);
    }
    //	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void RBX::Instance::write_jumppower(float jumppower) {
    std::cout << jumppower << std::endl;
    Umodule::write<float>(this->address + Offsets::JumpPower1, jumppower);
}

float RBX::Instance::read_jumppower() {
    return  Umodule::read<float>(this->address + Offsets::JumpPower1);

}


void RBX::Instance::GetMouseService() {
    while (!(globals::mouse_service = globals::game.FindFirstChild("MouseService").address)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

#include <iostream>
#include <cstdint>
#include <stdexcept>
#include "../math/enums/enum.h"

std::uint64_t RBX::Instance::GetInputObject(std::uint64_t base_address) {
    try {

        std::uint64_t target_address = base_address + Offsets::Input::InputObject;

        std::uint64_t input_object = Umodule::read<std::uint64_t>(target_address);

        if (input_object == 0) {
            std::cerr << "Error: Failed to read valid input object. Address may be invalid." << std::endl;
            return 0;
        }

        return input_object;
    }
    catch (const std::exception& e) {

        std::cerr << "Error: Exception caught in GetInputObject: " << e.what() << std::endl;
        return 0;
    }
    catch (...) {

        std::cerr << "Error: Unknown exception caught in GetInputObject" << std::endl;
        return 0;
    }
}

std::uint64_t FindPart(const std::string& part_name) {
    auto character = globals::players.GetLocalPlayer().GetModelInstance();
    auto part = character.FindFirstChild(part_name);
    return part.address ? part.GetPart() : 0;
}

std::vector<Instance> GetCharacterParts() {
    auto character = globals::players.GetLocalPlayer().GetModelInstance();
    return character.GetChildren();
}
void Instance::SetColor(const std::string& part_name, RBX::Vector3 color) {
    auto part = FindPart(part_name);
    if (part) Umodule::write<RBX::Vector3>(part + Offsets::Part::Part_Color, color);
}

Instance Instance::SetAllColor(RBX::Vector3 color) {
    for (auto& part : GetCharacterParts()) {
        SetColor(part.GetName(), color);
    }
    return *this;
}

void Instance::SetMaterial(const std::string& part_name) {
    auto part = FindPart(part_name);
    if (part) Umodule::write<int16_t>(part + Offsets::Part::Part_Material, static_cast<int16_t>(utils::enums::material::ForceField));
}

Instance Instance::SetAllMaterial() {
    for (auto& part : GetCharacterParts()) {
        SetMaterial(part.GetName());
    }
    return *this;
}
int Instance::getPlayerArmor() {
    for (const auto& player : globals::cached_players) {
        return Umodule::read<int>(player.armor_obj.address + Offsets::Value);
    }
    return 0;
}

RBX::Instance Instance::GetPlayersService() {

    RBX::Instance playersService = globals::game.FindFirstChildOfClass("Players");


    return playersService;
}

RBX::Instance Instance::GetLocalPlayer() const {

    RBX::Instance localPlayer = Umodule::read<RBX::Instance>(this->address + Offsets::Local_Entity);



    return localPlayer;
}

RBX::Instance Instance::GetModelInstance() const {

    RBX::Instance modelInstance = Umodule::read<RBX::Instance>(this->address + Offsets::Model_Instance);

 

    return modelInstance;
}

std::int32_t Instance::GetRigType() {

    std::uint8_t rigType = Umodule::read<std::uint8_t>(this->address + Offsets::Humanoid_RigType);



    return static_cast<std::int32_t>(rigType);
}

RBX::Instance Instance::GetModelPrimaryPart() {

    RBX::Instance primaryPart = Umodule::read<RBX::Instance>(this->address + Offsets::Primary_Part);



    return primaryPart;
}

RBX::Instance Instance::GetTeam() const {

    RBX::Instance team = Umodule::read<RBX::Instance>(this->address + Offsets::Team);

 

    return team;
}

void Instance::SetPartPos(RBX::Vector3 position) {
    static Vector3 cached_position = { 0.0f, 0.0f, 0.0f };

    const float threshold = 0.001f;

    if ((position - cached_position).magnitude() > threshold) {
        auto primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Part_Primitive);

        for (int i = 0; i < 10000; i++) {
            Umodule::write<Vector3>(primitive + Offsets::Part_Position, position);
        }

        cached_position = position;
    }
}
RBX::Vector3 Instance::GetMoveDirection() {
    return Umodule::read<RBX::Vector3>(this->address + Offsets::utils::MoveDirection);
}

void Instance::SetPartCframe(RBX::CFrame NewCframe) {
    if (this->address == 0)
        return;

    static std::unordered_map<std::uint64_t, RBX::CFrame> CachedCframes;
    const float Threshold = 0.001f;

    RBX::CFrame& Cached = CachedCframes[this->address];

    if ((NewCframe.position - Cached.position).magnitude() > Threshold) {
        std::uint64_t Primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Part_Primitive);
        if (Primitive == 0)
            return;

        for (int i = 0; i < 10000; i++) {
            Umodule::write<RBX::CFrame>(Primitive + Offsets::Part_Position, NewCframe);
        }

        Cached = NewCframe;
    }
}


RBX::CFrame Instance::GetPartCframe() {


    std::uint64_t Primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Part_Primitive);
    if (Primitive == 0)
        return RBX::CFrame();

    return Umodule::read<RBX::CFrame>(Primitive + Offsets::Part_Position);
}



void Instance::writePositionMultipleTimes(std::uint64_t primitive, RBX::Vector3 position) {

    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Position, position);
}

uint64_t Instance::getPlayerUserId() {

    std::uint64_t userIdAddress = Umodule::read<std::uint64_t>(this->address + Offsets::Identity::UserId);



    return Umodule::read<uint64_t>(userIdAddress);
}
std::string Instance::GetName() const {

    std::uint64_t nameAddress = Umodule::read<std::uint64_t>(this->address + Offsets::Name);

 

    return RBX::Instance::ReadString(nameAddress);
}

std::string Instance::GetClass() const {

    std::uint64_t classAddress = Umodule::read<std::uint64_t>(this->address + Offsets::Class_Name);

 

    std::uint64_t sizeAddress = classAddress + Offsets::Size;
    std::uint64_t classNameSize = Umodule::read<std::uint64_t>(sizeAddress);
    return RBX::Instance::ReadString(classNameSize);
}

std::vector<Instance> Instance::GetChildren() const {
    std::vector<Instance> children;



    std::uint64_t start = Umodule::read<std::uint64_t>(this->address + Offsets::Children);
 

    std::uint64_t end = Umodule::read<std::uint64_t>(start + Offsets::ChildrenEnd);
 

    for (auto instance = Umodule::read<std::uint64_t>(start); instance != end; instance += Offsets::HierarchyInterPolation) {
   
        children.emplace_back(Umodule::read<Instance>(instance));
    }
    return children;
}

Instance Instance::FindFirstChild(std::string childName) const {

   

    auto children = this->GetChildren();
 

    for (const auto& child : children) {
        if (child.GetName() == childName) {
            return child;
        }
    }

    return Instance();
}
bool Instance::IsA(const std::string& className) const {

    return this->GetClass() == className;
}

bool Instance::IsDescendantOf(const Instance& ancestor) const {


    Instance current = *this;
    while (current.address) {
        if (current.address == ancestor.address) return true;
        current = current.GetParent();
        if (!current.address) {

            break;
        }
    }
    return false;
}

std::vector<Instance> Instance::GetDescendants() const {
    std::vector<Instance> descendants;

    if (!this->address) {

        return descendants;
    }

    std::vector<Instance> stack = this->GetChildren();
    while (!stack.empty()) {
        Instance instance = stack.back();
        stack.pop_back();
        if (instance.address == 0) {

            continue;
        }

        descendants.push_back(instance);
        auto children = instance.GetChildren();
        stack.insert(stack.end(), children.begin(), children.end());
    }

    return descendants;
}

Instance Instance::FindFirstDescendant(const std::string& name) const {
    if (name.empty()) {

        return Instance();
    }

    auto descendants = this->GetDescendants();
    for (const auto& descendant : descendants) {
        if (descendant.GetName() == name) return descendant;
    }
    return Instance();
}
Instance Instance::WaitForChild(std::string childName, std::chrono::milliseconds timeout) const {
    if (childName.empty()) {

        return Instance();
    }

    std::uint64_t startTime = std::chrono::steady_clock::now().time_since_epoch().count();

    while (true) {

        std::uint64_t currentTime = std::chrono::steady_clock::now().time_since_epoch().count();
        if (std::chrono::milliseconds(currentTime - startTime) >= timeout) {

            return Instance();
        }

        auto children = this->GetChildren();
        for (const auto& child : children) {
            if (child.GetName() == childName) {

                return child;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
Instance Instance::FindFirstChildOfClass(std::string serviceName) {

   for (const auto& child : this->GetChildren()) {
        if (child.GetClass() == serviceName) 
            return child;
    }
    return Instance();
}
Instance Instance::GetService(std::string serviceName) {
    if (serviceName == "Workspace") {
        return  RBX::Instance{}.GetWorkspaceByOffset();
    }
    else if (serviceName == "Players") {
        return RBX::Instance{}.GetPlayersService();
    }
    else if (serviceName == "Lighting") {
        return RBX::Instance{}.GetLighting();

    }
    return Instance();
}
bool Instance::SetProperty(const std::string& propertyName, std::uint64_t value) {
    std::uint64_t propertyAddr = globals::game.FindFirstChild(propertyName).address;
    if (!propertyAddr) return false;
    Umodule::write<std::uint64_t>(propertyAddr, value);
    return true;
}

std::uint64_t Instance::GetProperty(const std::string& propertyName) const {
    std::uint64_t propertyAddr = this->address + globals::game.FindFirstChild(propertyName).address;
    if (!propertyAddr) return 0;
    return Umodule::read<std::uint64_t>(propertyAddr);
}

bool Instance::Destroy() {
    if (!this->address) return false;
    Umodule::write<std::uint64_t>(this->address + Offsets::Parent, 0);
    return true;
}

bool Instance::Clone(Instance& newInstance) const {
    if (!this->address) return false;
    newInstance = Umodule::read<Instance>(this->address);
    return newInstance.address != 0;
}

bool Instance::MoveTo(Instance& parent) {
    if (!this->address || !parent.address) return false;
    Umodule::write<std::uint64_t>(this->address + Offsets::Parent, parent.address);
    return true;
}
Instance Instance::GetParent() {
    return Umodule::read<Instance>(this->address + Offsets::Parent);
}
std::vector<Instance> Instance::FindAllChildrenByName(const std::string& childName) const {
    std::vector<Instance> matchingInstances;
    auto children = this->GetChildren();
    for (const auto& child : children) {
        if (child.GetName() == childName) matchingInstances.push_back(child);
    }
    return matchingInstances;
}
RBX::Instance RBX::Instance::GetWorkspace() {
    RBX::Instance holder{};

    if (!globals::game.address)
        return holder;

    holder = globals::game.FindFirstChildOfClass("Workspace");
    return holder; // when debug mode errors here it means cheats outdated
}
void RBX::Instance::SetWalkSpeed(float walkspeed) {
    Umodule::write<float>(this->address + Offsets::Entity::WalkSpeed1, walkspeed);
}
void RBX::Instance::SetJumpPower(float jumppower) {
    Umodule::write<float>(this->address + Offsets::Entity::JumpPower1, jumppower);
}

void RBX::Instance::SetHipHeight(float HipHeight) {
    Umodule::write<float>(this->address + Offsets::Entity::HipHeight, HipHeight);
}
void RBX::Instance::SetWalkSpeedCompensator(float walkspeed) {
    Umodule::write<float>(this->address + Offsets::WalkSpeed2, walkspeed);
} // call this before calling SetWalkspeed and make sure their the same value
float RBX::Instance::GetWalkSpeed() {
    return  Umodule::read<float>(this->address + Offsets::Entity::WalkSpeed1);
}
float RBX::Instance::GetJumpPower() {
    return  Umodule::read<float>(this->address + Offsets::Entity::JumpPower1);
}
float RBX::Instance::GetHipHeight() {
  return  Umodule::read<float>(this->address + Offsets::Entity::HipHeight);
}
void RBX::Instance::SetUseJumpPower(bool usejumppower) {
    Umodule::write<bool>(this->address + Offsets::Entity::UseJumpPower, usejumppower);
}
float RBX::Instance::GetPing() {
    auto pingstat = globals::game.FindFirstChild("Stats");
    auto ps = pingstat.FindFirstChild("PerformanceStats");
    auto ping = ps.FindFirstChild("Ping");
    RBX::Instance pingvalue = Umodule::read<RBX::Instance>(ping.address + 0xC8);
    return ping.GetFloatFromValue();
}
RBX::Instance RBX::Instance::GetWorkspaceByOffset() {
    auto workspace = Umodule::read<RBX::Instance>(globals::game.address + Offsets::General::Workspace);
    if (workspace.address) {
        return workspace;
    }
    else {
        GetWorkspace();
    }
}
RBX::Instance RBX::Instance::GetCurrentCamera() {
    return Umodule::read<RBX::Instance>(globals::camera.address + Offsets::Camera::Camera_Offset);
}
uint64_t RBX::Instance::GetScriptContext()
{
    auto HyrbidScriptsJob = RBX::TaskScheduler{}.GetJobByName("WaitingHybridScriptsJob");
    return Umodule::read<uint64_t>(HyrbidScriptsJob + Offsets::General::ScriptContext);
}

uint64_t RBX::Instance::GetDataModelViaScriptContext()
{
    return Umodule::read<uint64_t>(GetScriptContext() + Offsets::Hierarchy::Parent);
}
std::vector<Instance> Instance::FindAllDescendantsByName(const std::string& name) const {
    std::vector<Instance> matchingInstances;
    auto descendants = this->GetDescendants();
    for (const auto& descendant : descendants) {
        if (descendant.GetName() == name) matchingInstances.push_back(descendant);
    }
    return matchingInstances;
}
void Instance::SetPartVelocity(RBX::Vector3 velocity) {
    static RBX::Vector3 cached_velocity = { 0.0f, 0.0f, 0.0f };
    std::uint64_t primitive = Umodule::read<std::uint64_t>(this->address + Offsets::Part_Primitive);

    if ((velocity - cached_velocity).magnitude() > 0.001f) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 1000 && std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count() < 1; i++) {
            Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Velocity, velocity);
        }

        cached_velocity = velocity;
    }

    Umodule::write<RBX::Vector3>(primitive + Offsets::Part_Velocity, velocity);
}
void RBX::Instance::SetSize2(float size) {

    Umodule::write<float>(this->address + Offsets::Part_Size, size);
    Umodule::write<float>(this->address + Offsets::Size, size);
    Umodule::write<float>(GetPart() + Offsets::Size, size);
    Umodule::write<float>(GetPart() + Offsets::Part_Size, size);

}

std::uint64_t Instance::GetPart() const {

    return Umodule::read<std::uint64_t>(this->address + Offsets::Part_Primitive);
}

int Instance::GetPartMaterial() const {
    return Umodule::read<int>(this->address + Offsets::Part::Part_Material);
}

bool Instance::SetCanCollide(bool value) {
    return Umodule::write<bool>(this->address + Offsets::CanCollide, value);
}

RBX::Vector3 Instance::GetPosition() const {

    return Umodule::read<RBX::Vector3>(this->GetPart() + Offsets::Part_Position);
}

RBX::Vector3 Instance::GetVelocity() const {
    return Umodule::read<RBX::Vector3>(this->GetPart() + Offsets::Part_Velocity);
}

RBX::Matrix3x3 Instance::GetRotation() const {
    return Umodule::read<RBX::Matrix3x3>(this->GetPart() + Offsets::Part_Rotation);
}

RBX::Vector3 Instance::GetSize() const {

    return Umodule::read<RBX::Vector3>(this->GetPart() + Offsets::Part_Size);
}

RBX::Vector3 Instance::SetSize(RBX::Vector3 size) const {
    printf("errrr");
}

RBX::CFrame Instance::GetCframe() {
    std::uint64_t primitive = GetPart();
    return primitive ? Umodule::read<RBX::CFrame>(primitive + Offsets::CFrame) : RBX::CFrame{};
}

#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <immintrin.h>  

void RBX::Instance::CallCachedMouseService(std::uint64_t address) {
    try {

        std::uint64_t input_object = GetInputObject(address);

        if (input_object && input_object != 0xFFFFFFFFFFFFFFFF) {

            _mm_prefetch(reinterpret_cast<const char*>(input_object) + Offsets::Input::MousePosition, _MM_HINT_T0);
            _mm_prefetch(reinterpret_cast<const char*>(input_object) + Offsets::Input::MousePosition + sizeof(RBX::Vector2), _MM_HINT_T0);
        }
        else {
            std::cerr << "Error: Invalid input_object value (0 or 0xFFFFFFFFFFFFFFFF)" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Exception caught in CallCachedMouseService: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Error: Unknown exception caught in CallCachedMouseService" << std::endl;
    }
}

void RBX::Instance::WriteMousePosition(std::uint64_t address, float x, float y) {
    try {

        std::uint64_t input_object = GetInputObject(address);

        if (input_object && input_object != 0xFFFFFFFFFFFFFFFF) {

            Umodule::write<RBX::Vector2>(input_object + Offsets::Input::MousePosition, { x, y });
        }
        else {
            std::cerr << "Error: Invalid input_object value (0 or 0xFFFFFFFFFFFFFFFF)" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Exception caught in WriteMousePosition: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Error: Unknown exception caught in WriteMousePosition" << std::endl;
    }
}

void RBX::Instance::SetHumanoidJumpPower(float JumpPower)
{
    Umodule::write<float>(this->address + Offsets::JumpPower1, JumpPower);
    return;
}
float RBX::Instance::GetMaxHealth() const {
    auto one = Umodule::read<std::uint64_t>(this->address + Offsets::Health::Max);
    auto two = Umodule::read<std::uint64_t>(Umodule::read<std::uint64_t>(this->address + Offsets::Health::Max));

    std::uint64_t normalizedHealth = one ^ two;
    float transformer;
    std::memcpy(&transformer, &normalizedHealth, sizeof(transformer));
    return transformer;
}

int RBX::Instance::getHumanoidState() const {
    return Umodule::read<int>(this->address + Offsets::Entity::Humanoid_CurrentData);
}

RBX::Vector2 RBX::WorldToScreen(RBX::Vector3 world, RBX::Vector2 dimensions, RBX::Matrix4x4 viewmatrix) {
    RBX::Vector4 clipCoords = {
        world.x * viewmatrix.data[0] + world.y * viewmatrix.data[1] + world.z * viewmatrix.data[2] + viewmatrix.data[3],
        world.x * viewmatrix.data[4] + world.y * viewmatrix.data[5] + world.z * viewmatrix.data[6] + viewmatrix.data[7],
        world.x * viewmatrix.data[8] + world.y * viewmatrix.data[9] + world.z * viewmatrix.data[10] + viewmatrix.data[11],
        world.x * viewmatrix.data[12] + world.y * viewmatrix.data[13] + world.z * viewmatrix.data[14] + viewmatrix.data[15]
    };

    if (globals::non_zero_check) {
        if (clipCoords.w <= 1e-6f) {
            return { -1.0f, -1.0f };
        }
    }

    float inv_w = 1.0f / clipCoords.w;
    RBX::Vector3 ndc = { clipCoords.x * inv_w, clipCoords.y * inv_w, clipCoords.z * inv_w };

    return {
        (dimensions.x / 2.0f) * (ndc.x + 1.0f),
        (dimensions.y / 2.0f) * (1.0f - ndc.y)
    };
}

int Instance::getPlayerTeamColor() {
    return Umodule::read<int>(this->address + Offsets::Identity::TeamColor);
}

bool Instance::isPlayerVisible(Matrix4x4 viewMatrix) {
    Instance Instance;
    Vector3 position = Instance.GetPosition();
    Vector2 screenPosition = WorldToScreen(position, GetDimensions(), viewMatrix);
    return !(screenPosition.x == -1 && screenPosition.y == -1);
}
void RBX::Instance::SetCameraMaxZoom(int zoom) {
    Umodule::write<int>(globals::camera.address + Offsets::Camera::CameraMaxZoomOut, zoom);
}
void RBX::Instance::SetCameraMinZoom(int zoom) {
    Umodule::write<int>(globals::camera.address + Offsets::Camera::CameraMinZoomOut, zoom);
}
uint64_t RBX::Instance::SetFramePositionX(uint64_t position) {
    return Umodule::write<uint64_t>(address + Offsets::Frame::Xoffset, position);
}

uint64_t RBX::Instance::SetFramePositionY(uint64_t position) {
    return Umodule::write<uint64_t>(address + Offsets::Frame::Yoffset, position);
}
uint64_t RBX::Instance::SetFrameRotation(uint64_t rotation) {
    return Umodule::write<uint64_t>(address + Offsets::Frame::Rotation, rotation);
}

int_fast64_t RBX::Instance::GetGameId() {
    int_fast64_t game_id = Umodule::read<int_fast64_t>(this->address + Offsets::GameID);
    return (game_id > 0 && game_id <= 0x2540BE3FF) ? std::stoll(std::to_string(game_id)) : 0;
}

RBX::Instance RBX::Instance::GetDataModelPTR() {
    RBX::Instance Instance2;
    return static_cast<RBX::Instance>(Umodule::read<uintptr_t>(Instance2.GetRenderView() + Offsets::DataModel));
}

RBX::Instance RBX::Instance::GetDataModel() {
    RBX::TaskScheduler Instance;
    RBX::Instance Instance2;
    std::uint64_t datamodel = Umodule::read<uintptr_t>(Instance2.GetRenderView() + Offsets::DataModel);
    return static_cast<RBX::Instance>(Umodule::read<std::uint64_t>(datamodel + Offsets::Game));
}

int_fast64_t RBX::Instance::GetCurrentPlaceId() {
    return Umodule::read<int_fast64_t>(this->address + Offsets::General::GameID);
}

std::vector<RBX::Instance> RBX::Instance::GetPlayerList() {
    std::vector<RBX::Instance> players;
    RBX::Instance playerService = this->FindFirstChild("Players");
    if (playerService.address == 0) return players;

    return playerService.GetChildren();
}

bool RBX::Instance::IsInGame() {
    if (this->GetPlayersService().GetChildren().size() > 1) {
        return false;
    }
    else {
        return true;
    }
}

int RBX::Instance::GetPlayerCount() {
    std::vector<RBX::Instance> players = this->GetPlayerList();
    return players.size();
}

std::string RBX::Instance::GetJobId() /* globals::game.address.GetJobId()*/ {
    return Umodule::read<std::string>(this->address + Offsets::TaskScheduler::JobId);
}
int RBX::Instance::SetAfkTime(int time) {
    Umodule::write<int>(this->address + Offsets::General::ForceNewAFKDuration, time);
}
std::uint64_t Instance::GetRenderView() {
    return Umodule::read<uintptr_t>(TaskScheduler{}.GetJobByName("RenderJob") + 0x218);
}
uintptr_t Instance::GetHumanoidEnumState() {
    uintptr_t state = Umodule::read<uintptr_t>(this->address + Offsets::Humanoid_CurrentData);
   return state;

}
std::uint64_t Instance::GetCameraMatrix() {
    auto renderView = GetRenderView();
    return renderView == 0 ? 0 : Umodule::read<uintptr_t>(renderView + 0x58);
}

std::pair<int, int> Instance::GetViewportSize() {
    auto renderView = GetRenderView();
    if (renderView == 0) return { 0, 0 };

    return { Umodule::read<int>(renderView + 0x2D0), Umodule::read<int>(renderView + 0x2D0) };
}

bool Instance::IsRenderViewValid() {
    return GetRenderView() != 0;
}

void Instance::SetRenderFeature(std::string featureName, bool enable) {
    auto renderView = GetRenderView();
    if (renderView == 0) return;

    uintptr_t featureAddress = Umodule::read<uintptr_t>(renderView + 0x1DA);
    if (featureName == "Bloom") {
        Umodule::write<bool>(featureAddress + 0x14C, enable);
    }
    else if (featureName == "Voids") {
        Umodule::write<bool>(featureAddress + 0x2A0, enable);
    }
}





using namespace RBX;

RBX::Instance RBX::Instance::GetCameraInstance()
{
    return Umodule::read<RBX::Instance>(this->address + Offsets::Camera::Camera_Offset);
}


RBX::Vector3 RBX::Instance::GetCameraPosition()
{
    return Umodule::read<RBX::Vector3>(this->address + Offsets::Camera_Position);
}
RBX::Matrix3x3 RBX::Instance::GetCameraRotation()
{
    return Umodule::read<RBX::Matrix3x3>(this->address + Offsets::Camera_Rotation);
}

float RBX::Instance::GetFov()
{
    return Umodule::read<float>(this->address + Offsets::FieldOfView);
}

void RBX::Instance::SetCameraRotation(RBX::Matrix3x3 Rotation)
{
    Umodule::write<RBX::Matrix3x3>(this->address + Offsets::Camera_Rotation, Rotation);
}

RBX::Instance RBX::Instance::Spectate(RBX::Instance stringhere) {
    RBX::Instance placeholder;
    Umodule::write<std::uint64_t>(globals::game.FindFirstChild("Workspace").FindFirstChild("Camera").address + Offsets::Value, stringhere.address);
    return placeholder;
}

RBX::Instance RBX::Instance::UnSpectate() {
    RBX::Instance placeholder;
    Umodule::write<std::uint64_t>(globals::game.FindFirstChild("Workspace").FindFirstChild("Camera").address + Offsets::Value,
        globals::localplayer.humanoid.address);
    return placeholder;
}