#include "../../main.h"
#include "../../../globals/globals.hpp"

#include "../../engine/Classes/offsets/offsets.hpp"
#include "../../../misc/Umodule/Umodule.hpp"

#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <cmath>

#include <chrono>
namespace RBX {
	void CrosshairLoop();
	void SnaplineLoop();
	void initVisuals();
	void InitializeAimbot();
	VOID TriggerBotLoop();
	void initsilent();

}
extern RBX::PlayerInstance aimbot_target;
extern RBX::PlayerInstance aimbot_target2;