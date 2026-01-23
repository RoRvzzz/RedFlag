#pragma once
#include <cstdint>
#include <vector>
#include <utility>
#include <string>
#include "math/math.h"

namespace rbx {
	struct instance_t;

	struct addressable_t
	{
		std::uint64_t address;

		addressable_t() : address(0) {}
		addressable_t(std::uint64_t address) : address(address) {}
	};

	struct nameable_t : addressable_t
	{
		using addressable_t::addressable_t;
		std::string get_name() const;
		std::string get_class_name() const;

	};

	struct treeinterface_t
	{
		std::vector<instance_t> get_children();

		template<typename T>
		std::vector<T> get_children();

		template<typename T, typename R>
		std::pair<std::vector<T>, std::vector<R>> get_children();

		std::uint64_t find_first_child(const std::string& child_name);

		std::uint64_t find_first_child_by_class(const std::string& serviceName);

		std::uint64_t get_parent();


	};

	struct instance_t : public nameable_t, public treeinterface_t
	{
		using nameable_t::nameable_t;
		template<typename T>
		void set_value(T value);
	};

	struct model_instance_t final : public addressable_t, public treeinterface_t
	{

	};



	struct primitive_t final : public addressable_t
	{
		math::vector3_t get_velocity() const;
		math::vector3_t get_position();
		math::vector3_t get_size();
		math::matrix3_t get_rotation();
	};

	struct part_t : public nameable_t, public treeinterface_t
	{
		primitive_t get_primitive();
	};


	struct player_t final : public instance_t
	{
		using instance_t::instance_t;

		std::uint64_t get_userid();
		std::uint64_t get_team();
		model_instance_t get_model_instance();
	};

	struct workspace_t final : public instance_t
	{
		using instance_t::instance_t;

		// << other stuff for workspace >>

	};


	struct replicated_storage_t final : public instance_t
	{
		using instance_t::instance_t;

		// << other stuff for replicated_storage >>

	};

	struct datamodel_t final : public instance_t
	{
		using instance_t::instance_t;
		std::uint64_t get_place_id();
		std::uint64_t get_game_id();
		std::uint64_t get_creator_id();
		workspace_t get_workspace();
		replicated_storage_t get_replicated_storage();
		player_t get_local_player();
	};

	struct visualengine_t final : public addressable_t
	{
		math::vector2_t get_dimensions();
		math::matrix4_t get_viewmatrix();
		math::vector2_t world_to_screen(const math::vector3_t& world);
	};

	struct camera_t final : public addressable_t
	{
		math::vector3_t get_camera_position();
		math::matrix3_t get_camera_rotation();

		void set_camera_position(const math::vector3_t& pos);
		void set_camera_rotation(const math::matrix3_t& rot);
	};

	struct humanoid_t final : public addressable_t
	{
		humanoid_t() : addressable_t(0) {}
		humanoid_t(std::uint64_t addr) : addressable_t(addr) {}

		int get_humanoid_state() const;

		float get_health() const;
		float get_max_health() const;
	};


}

#include <sdk/offsets/offsets.h>
#include <memory/memory.h>

extern std::unique_ptr<memory_t> memory;

template<typename T>
std::vector<T> rbx::treeinterface_t::get_children()
{
	T* self = static_cast<T*>(this);

	std::vector<T> children;

	std::uint64_t start = memory->read<std::uint64_t>(self->address + Offsets::Instance::ChildrenStart);
	std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);
	for (auto instance = memory->read<std::uint64_t>(start); instance != end; instance += 0x10) {
		children.emplace_back(T(memory->read<std::uint64_t>(instance)));
	}
	return children;
}

template <typename T>
void rbx::instance_t::set_value(T value)
{
	rbx::instance_t* self = static_cast<rbx::instance_t*>(this);
	memory->write<T>(self->address + Offsets::Misc::Value, value);
}



//template<typename T, typename R>
//std::pair<std::vector<T>, std::vector<R>> rbx::treeinterface_t::get_children() {
//
//}