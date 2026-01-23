#include "../thread.h"
#include "../../../../../misc/Umodule/Umodule.hpp"
#include "../../../../engine/Classes/offsets/offsets.hpp"
void sethum(float value)
{
	auto hm = globals::players.FindFirstChild("humaniod");
	if (hm.address)
	{
		Umodule::write<float>(hm.address + Offsets::HipHeight, value);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void utils::hipheight() {
	while (true) {
		if (globals::HipHeight) {
			globals::HipHeight_Bind.update();
			if (globals::HipHeight_Bind.enabled) {
				sethum(globals::HipHeight);
			}
		}
	}
}