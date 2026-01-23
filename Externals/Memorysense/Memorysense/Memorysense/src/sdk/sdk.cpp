#include "sdk.h"
#include <vector>
#include <main.h>
#include "offsets/offsets.h"

std::string rbx::nameable_t::get_name() const {
	std::uint64_t ptr = memory->read<std::uint64_t>(this->address + Offsets::Instance::Name);
	if (ptr != 0) {
		return memory->read_string(ptr);
	}
	return "unknown";
}

std::string rbx::nameable_t::get_class_name() const
{
	// Resolve ClassDescriptor using offsets, then read the ClassName pointer from the descriptor
	std::uint64_t class_descriptor = memory->read<std::uint64_t>(this->address + Offsets::Instance::ClassDescriptor);
	if (!class_descriptor)
		return "unknown";

	std::uint64_t class_name_ptr = memory->read<std::uint64_t>(class_descriptor + Offsets::Instance::ClassName);
	if (!class_name_ptr)
		return "unknown";

	return memory->read_string(class_name_ptr);
}

std::vector<rbx::instance_t> rbx::treeinterface_t::get_children()
{
	rbx::instance_t* self = static_cast<rbx::instance_t*>(this);

	std::vector<instance_t> children;

	std::uint64_t start = memory->read<std::uint64_t>(self->address + Offsets::Instance::ChildrenStart);
	std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);
	for (auto instance = memory->read<std::uint64_t>(start); instance != end; instance += 0x10) {
		children.emplace_back(instance_t(memory->read<std::uint64_t>(instance)));
	}
	return children;
}

std::uint64_t rbx::treeinterface_t::find_first_child(const std::string& child_name)
{
	rbx::instance_t* self = static_cast<rbx::instance_t*>(this);

	std::vector<rbx::instance_t> children = this->get_children();
	for (const auto& child : children) {
		if (child.get_name() == child_name) {
			return child.address;
		}
	}

	return 0;
}

std::uint64_t rbx::treeinterface_t::find_first_child_by_class(const std::string& class_name)
{
	for (const auto& child : this->get_children()) {
		if (child.get_class_name() == class_name) {
			return child.address;  // return the uint64_t address
		}
	}
	return 0;  // or some invalid value if no child matches
}

std::uint64_t rbx::treeinterface_t::get_parent()
{
	rbx::instance_t* self = static_cast<rbx::instance_t*>(this);

	instance_t parent = instance_t(memory->read<std::uint64_t>(self->address + Offsets::Instance::Parent));
	return parent.address;
}

int rbx::humanoid_t::get_humanoid_state() const
{
	uint64_t humanoid_state = memory->read<uint64_t>(this->address + Offsets::Humanoid::HumanoidState);
	int16_t humanoid_state_id = memory->read<int16_t>(humanoid_state + Offsets::Humanoid::HumanoidStateID);
	return humanoid_state_id;
}

rbx::player_t rbx::datamodel_t::get_local_player()
{
	std::uint64_t local_addr = memory->read<std::uint64_t>(this->find_first_child("Players") + Offsets::Player::LocalPlayer);
	if (!local_addr)
		return rbx::player_t{};

	return rbx::player_t(local_addr);
}

std::uint64_t rbx::player_t::get_userid()
{
	std::uint64_t userIdAddress = memory->read<std::uint64_t>(this->address + Offsets::Player::UserId);
	return memory->read<uint64_t>(userIdAddress);
}

std::uint64_t rbx::player_t::get_team() // go nova go!
{
	return memory->read<uint64_t>(this->address + Offsets::Player::Team);
}

rbx::model_instance_t rbx::player_t::get_model_instance()
{
	rbx::instance_t modelInstance = memory->read<rbx::instance_t>(this->address + Offsets::Player::ModelInstance);
	return rbx::model_instance_t(modelInstance.address);
}

std::uint64_t rbx::datamodel_t::get_place_id()
{
	return memory->read<uint64_t>(this->address + Offsets::DataModel::PlaceId);
}

std::uint64_t rbx::datamodel_t::get_game_id()
{
	return memory->read<uint64_t>(this->address + Offsets::DataModel::GameId);
}

std::uint64_t rbx::datamodel_t::get_creator_id()
{
	return memory->read<uint64_t>(this->address + Offsets::DataModel::CreatorId);
}

rbx::workspace_t rbx::datamodel_t::get_workspace()
{
	rbx::workspace_t holder{};

	if (!g_datamodel->address)
		return holder;

	holder = g_datamodel->find_first_child(("Workspace"));
	return holder; // when debug mode errors here it means cheats outdated < nova
}

rbx::replicated_storage_t rbx::datamodel_t::get_replicated_storage()
{
	rbx::replicated_storage_t holder{};

	if (!g_datamodel->address)
		return holder;

	holder = g_datamodel->find_first_child(("ReplicatedStorage"));
	return holder; // when debug mode errors here it means cheats outdated < nova
}

rbx::primitive_t rbx::part_t::get_primitive()
{
	std::uint64_t primitiveAddress = memory->read<std::uint64_t>(this->address + Offsets::BasePart::Primitive);
	return rbx::primitive_t(primitiveAddress);
}

math::vector2_t rbx::visualengine_t::get_dimensions()
{
	return memory->read<math::vector2_t>(this->address + Offsets::VisualEngine::Dimensions);
}

math::matrix4_t rbx::visualengine_t::get_viewmatrix()
{
	return memory->read<math::matrix4_t>(this->address + Offsets::VisualEngine::ViewMatrix);
}

math::vector2_t rbx::visualengine_t::world_to_screen(const math::vector3_t& world) {
	math::matrix4_t view = this->get_viewmatrix();
	math::vector2_t dims = this->get_dimensions();

	math::vector4_t clip{};
	clip.x = world.x * view(0, 0) + world.y * view(0, 1) + world.z * view(0, 2) + view(0, 3);
	clip.y = world.x * view(1, 0) + world.y * view(1, 1) + world.z * view(1, 2) + view(1, 3);
	clip.z = world.x * view(2, 0) + world.y * view(2, 1) + world.z * view(2, 2) + view(2, 3);
	clip.w = world.x * view(3, 0) + world.y * view(3, 1) + world.z * view(3, 2) + view(3, 3);

	if (clip.w < 0.1f)
		return { -1.0f, -1.0f };

	clip.x /= clip.w;
	clip.y /= clip.w;

	math::vector2_t screen;
	screen.x = (dims.x * 0.5f * clip.x) + (dims.x * 0.5f);
	screen.y = -(dims.y * 0.5f * clip.y) + (dims.y * 0.5f);

	return screen;
}


math::vector3_t rbx::primitive_t::get_velocity() const {
	return memory->read<math::vector3_t>(this->address + Offsets::BasePart::AssemblyLinearVelocity);
}

math::vector3_t rbx::primitive_t::get_position()
{
	return memory->read<math::vector3_t>(this->address + Offsets::BasePart::Position);
}

math::matrix3_t rbx::primitive_t::get_rotation()
{
	return memory->read<math::matrix3_t>(this->address + Offsets::BasePart::Rotation);
}

math::vector3_t rbx::primitive_t::get_size()
{
	return memory->read<math::vector3_t>(this->address + Offsets::BasePart::Size);
}

math::vector3_t rbx::camera_t::get_camera_position()
{
	return memory->read<math::vector3_t>(this->address + Offsets::Camera::Position);
}

math::matrix3_t rbx::camera_t::get_camera_rotation()
{
	return memory->read<math::matrix3_t>(this->address + Offsets::Camera::Rotation);
}

void rbx::camera_t::set_camera_position(const math::vector3_t& pos)
{
	memory->write<math::vector3_t>(this->address + Offsets::Camera::Position, pos);
}

void rbx::camera_t::set_camera_rotation(const math::matrix3_t& rot)
{
	memory->write<math::matrix3_t>(this->address + Offsets::Camera::Rotation, rot);
}

float rbx::humanoid_t::get_health() const
{
	auto one = memory->read<std::uint64_t>(this->address + Offsets::Humanoid::Health);
	auto two = memory->read<std::uint64_t>(memory->read<std::uint64_t>(this->address + Offsets::Humanoid::Health));

	union { std::uint64_t hex; float f; } conv;
	conv.hex = one ^ two;
	return conv.f;
}

float rbx::humanoid_t::get_max_health() const
{
	auto one = memory->read<std::uint64_t>(this->address + Offsets::Humanoid::MaxHealth);
	auto two = memory->read<std::uint64_t>(memory->read<std::uint64_t>(this->address + Offsets::Humanoid::MaxHealth));

	union { std::uint64_t hex; float f; } conv;
	conv.hex = one ^ two;
	return conv.f;
}