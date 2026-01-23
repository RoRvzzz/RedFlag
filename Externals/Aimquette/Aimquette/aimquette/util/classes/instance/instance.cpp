#include "../classes.h"
#include "../../offsets.h"
#include "../../globals.h"

math::Vector2 roblox::instance::GetDimensins()
{
    return read<math::Vector2>(this->address + offsets::Dimensions);
}

math::Matrix4 roblox::instance::GetViewMatrix()
{
    return read<math::Matrix4>(this->address + offsets::viewmatrix);
}

void roblox::instance::setAnchored(bool booz)
{
    write<bool>(this->primitive() + offsets::Anchored, booz);
}

uintptr_t roblox::instance::primitive() const {
    if (!is_valid_address(address)) return 0;
    return read<uintptr_t>(this->address + offsets::Primitive);
}

struct stringed {
    char data[200];
};
std::string read_string(std::uint64_t address)
{
    auto character = read<stringed>(address);
    return character.data;
}

std::string fetchstring(std::uint64_t address)
{
    int length = read<int>(address + 0x18);

    if (length >= 16u) {
        std::uintptr_t padding = read<std::uintptr_t>(address);
        return read_string(padding);
    }
    std::string name = read_string(address);
    return name;
}

std::string roblox::instance::get_name() const {
    return fetchstring(read<uintptr_t>(this->address + offsets::Name));
}
float roblox::instance::read_health() const {
    return read<float>(this->address + offsets::Health);
}
void roblox::instance::write_health(float health) {
    write<float>(this->address + offsets::Health, health);
}

CFrame roblox::instance::read_cframe() const {
    return read<CFrame>(this->primitive() + offsets::CFrame);
}
Matrix3 roblox::instance::read_part_cframe() const {
    return read<Matrix3>(this->primitive() + offsets::CFrame);
}
void roblox::instance::write_cframe(CFrame cframe) {
    write<CFrame>(this->primitive() + offsets::CFrame, cframe);
}
void roblox::instance::write_part_cframe(Matrix3 ma3) {
     write<Matrix3>(this->primitive() + offsets::CFrame, ma3);
}
float roblox::instance::read_maxhealth() const {
    return read<float>(this->address + offsets::MaxHealth);
}
std::string roblox::instance::get_class_name() const {
    return fetchstring(read<uintptr_t>(read<uintptr_t>(this->address + offsets::ClassDescriptor) + offsets::ChildrenEnd));
}
std::string roblox::instance::get_displayname() const {
    const std::uint64_t name_pointer = read<std::uint64_t>(this->address + offsets::DisplayName);

    std::string ptr_result;

    if (name_pointer)
    {
        ptr_result = fetchstring(name_pointer);

        if (ptr_result == (""))
        {
            ptr_result = fetchstring(this->address + offsets::DisplayName);
            return ptr_result;
        }
    }

    return ("Unable to read displayname");
}

std::string roblox::instance::get_humdisplayname() const {
    const std::uint64_t name_pointer = read<std::uint64_t>(this->address + offsets::HumanoidDisplayName);

    std::string ptr_result;

    if (name_pointer)
    {
        ptr_result = fetchstring(name_pointer);

        if (ptr_result == (""))
        {
            ptr_result = fetchstring(this->address + offsets::HumanoidDisplayName);
            return ptr_result;
        }
    }

    return ("Unable to read displayname");
}

roblox::instance roblox::instance::findfirstchild(std::string arg) const {
    roblox::instance returned;
    for (auto children : this->get_children()) {
        if (children.get_name() == arg) returned = children;
    }
    return returned;
}
roblox::instance roblox::instance::read_service(std::string arg) const {
    roblox::instance returned{};
    for (auto children : this->get_children()) {
        if (children.get_class_name() == arg) returned = children;
    }
    return returned;
}

roblox::instance roblox::instance::read_parent() const {
    return read<roblox::instance>(this->address + offsets::Parent);

}

Vector3 roblox::instance::get_pos() const {
    return read<Vector3>(this->primitive() + offsets::Position);
}
Vector3 roblox::instance::get_velocity() const {
    if (!is_valid_address(address)) return {};
    return read<Vector3>(this->primitive() + offsets::Velocity);
}

void roblox::instance::write_position(Vector3 arg) {
    write<Vector3>(this->primitive() + offsets::Position, arg);
}
void roblox::instance::write_velocity(Vector3 arg) {
    write<Vector3>(this->primitive() + offsets::Velocity, arg);
}

Vector3 roblox::instance::get_cam_pos() const {
    return read<Vector3>(this->address + offsets::CameraPos);
}
Matrix3 roblox::instance::get_cam_rot() const {
    return read<Matrix3>(this->address + offsets::CameraRotation);
}

void roblox::instance::write_cam_pos(Vector3 pos) {
    write<Vector3>(this->address + offsets::CameraPos, pos);
}
void roblox::instance::write_cam_rot(Matrix3 rot) {
    write<Matrix3>(this->address + offsets::CameraRotation, rot);
}

roblox::instance roblox::instance::model_instance() const {
    return read<roblox::instance>(this->address + offsets::ModelInstance);
}
roblox::instance roblox::instance::local_player() const {
    return read<roblox::instance>(this->address + offsets::LocalPlayer);
}

roblox::instance roblox::instance::read_workspace() const
{
    return read<roblox::instance>(this->address + offsets::Workspace);
}

Vector2 roblox::instance::get_dimensions() const {
    return read<Vector2>(this->address + offsets::Dimensions);
}

Vector3 roblox::instance::read_vector3_value() const {
    return read<Vector3>(this->address + offsets::Value);
}
int roblox::instance::read_int_value() const {
    return read<int>(this->address + offsets::Value);
}
bool roblox::instance::read_bool_value() const {
    return read<bool>(this->address + offsets::Value);
}

void roblox::instance::write_bool_value(bool val) {
    write<bool>(this->address + offsets::Value, val);
}
void roblox::instance::write_int_value(int val) {
    write<int>(this->address + offsets::Value, val);
}
void roblox::instance::write_vector3_value(Vector3 val) {
    write<Vector3>(this->address + offsets::Value, val);
}
void roblox::instance::write_float_value(float val) {
    write<float>(this->address + offsets::Value, val);
}
double roblox::instance::read_double_value() const {
    return read<double>(this->address + offsets::Value);
}
void roblox::instance::write_double_value(double val) {
    write<double>(this->address + offsets::Value, val);
}
float roblox::instance::read_float_value() const {
    return read<float>(this->address + offsets::Value);
}

std::vector <roblox::instance> roblox::instance::get_children() const {
    std::vector<roblox::instance> children;
    std::uint64_t begin = read<std::uint64_t>(this->address + offsets::Children);
    std::uint64_t end = read<std::uint64_t>(begin + offsets::ChildrenEnd);
    if (!begin) return children; if (!end) return children;
    for (auto instances = read<std::uint64_t>(begin); instances != end; instances += 16u)
    {
        children.emplace_back(read<roblox::instance>(instances));
    }
    return children;
}
struct Quaternion final { float x, y, z, w; };

roblox::instance roblox::instance::Team() const {
    if (!is_valid_address(address)) return roblox::instance();
    return read<roblox::instance>(this->address + offsets::Team);
}

Vector2 roblox::worldtoscreen(Vector3 world) {
    Vector2 dimensions = globals::instances::visualengine.get_dimensions();
    Matrix4 viewmatrix = read<Matrix4>(globals::instances::visualengine.address + offsets::viewmatrix);
    
    // Transform world position to clip space
    float clip_x = (world.x * viewmatrix.data[0]) + (world.y * viewmatrix.data[1]) + (world.z * viewmatrix.data[2]) + viewmatrix.data[3];
    float clip_y = (world.x * viewmatrix.data[4]) + (world.y * viewmatrix.data[5]) + (world.z * viewmatrix.data[6]) + viewmatrix.data[7];
    float clip_w = (world.x * viewmatrix.data[12]) + (world.y * viewmatrix.data[13]) + (world.z * viewmatrix.data[14]) + viewmatrix.data[15];

    // Check if behind camera
    if (clip_w < 0.1f)
        return { -1, -1 };

    // Convert to NDC (Normalized Device Coordinates)
    float ndc_x = clip_x / clip_w;
    float ndc_y = clip_y / clip_w;

    // Convert NDC to screen coordinates
    Vector2 screenPos = {
        (ndc_x + 1.0f) * (dimensions.x / 2.0f),
        (1.0f - ndc_y) * (dimensions.y / 2.0f)
    };

    // Check if on screen
    if (screenPos.x < 0 || screenPos.x > dimensions.x || screenPos.y < 0 || screenPos.y > dimensions.y)
        return { -1, -1 };

    return screenPos;
}