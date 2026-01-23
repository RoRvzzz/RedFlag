#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sdk/sdk.h>
#include <unordered_map>

namespace cache {
	struct r15_parts {
		rbx::instance_t head;
		rbx::instance_t humanoidRootPart;
		rbx::instance_t humanoid;

		rbx::instance_t upperTorso;
		rbx::instance_t lowerTorso;

		rbx::instance_t leftHand;
		rbx::instance_t rightHand;

		rbx::instance_t leftLowerArm;
		rbx::instance_t rightLowerArm;
		rbx::instance_t leftUpperArm;
		rbx::instance_t rightUpperArm;

		rbx::instance_t leftFoot;
		rbx::instance_t rightFoot;

		rbx::instance_t leftLowerLeg;
		rbx::instance_t rightLowerLeg;
		rbx::instance_t leftUpperLeg;
		rbx::instance_t rightUpperLeg;

		std::unordered_map<std::string, rbx::part_t> r15parts;
	};

	struct r6_parts {
		rbx::instance_t head;
		rbx::instance_t torso;
		rbx::instance_t leftArm;
		rbx::instance_t rightArm;
		rbx::instance_t leftLeg;
		rbx::instance_t rightLeg;
		rbx::instance_t humanoid;
		rbx::instance_t humanoidRootPart;

		std::unordered_map<std::string, rbx::part_t> r6parts;
	};

	struct tool_info {
		rbx::instance_t tool_instance;
		std::string tool_name;
	};

	struct cached_entity {
		std::uint64_t address = 0;

		bool localplayer = false;

		float health;
		float max_health;
		float distance;

		std::string name;

		rbx::instance_t team;
		rbx::instance_t player;
		rbx::model_instance_t character;
		std::vector<rbx::instance_t> children;

		r15_parts r15;
		r6_parts r6;

		tool_info tool;

		bool operator==(cache::cached_entity& other) {
			return character.address == other.character.address;
		}
	};
}

namespace player_cache {
	void start_cache();
	void load();
}
