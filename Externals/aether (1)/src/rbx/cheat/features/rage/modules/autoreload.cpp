#include "../thread.h"



void reload() {
	keybd_event(0, MapVirtualKey('R', 0), KEYEVENTF_SCANCODE, 0);
	keybd_event(0, MapVirtualKey('R', 0), KEYEVENTF_KEYUP, 0);
}
inline std::chrono::milliseconds mike(250);
	
void utils::autoreload() {
	while (true) {
		if (!globals::autoreload) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}
		if (globals::autoreload) {
			std::this_thread::sleep_for(mike);
			auto tool = globals::players.GetLocalPlayer().GetModelInstance().FindFirstChildOfClass("Tool");
			if (tool.GetClass() != "Tool") continue;
			auto ammo = tool.FindFirstChild("Ammo");
			auto ammo_value = ammo.getIntFromValue();
			if (ammo_value == 0) reload();
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

	}
}