#include "../thread.h"



static int down_count = 0;
static bool zzzzzzzzz = false;


void utils::rapidfire() {
	while (true) {
		if (globals::rapidfire) {
	
			// first method
            if (globals::rapidfire && down_count <= 40) {
                down_count++;

                INPUT input{};
                input.type = INPUT_MOUSE;
                input.mi.time = 0;
                input.mi.mouseData = 0;
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                SendInput(2, &input, sizeof(input));
            }
        }
        else {
            while (down_count > 0) {
                INPUT input{};
                input.type = INPUT_MOUSE;
                input.mi.time = 0;
                input.mi.mouseData = 0;
                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(2, &input, sizeof(input));
                down_count--;
            }
        }

		// second method 

        RBX::Instance backpack = globals::players.GetLocalPlayer().FindFirstChild("Backpack");
        for (RBX::Instance tool : backpack.GetChildren()) {
            if (tool.GetClass() != "Tool" && tool.GetName() != "Combat") continue;
            RBX::Instance shooting = tool.FindFirstChild("ShootingCooldown");
            RBX::Instance tyolerance = tool.FindFirstChild("ToleranceCooldown");
            shooting.write_double(0.000000001);
            tyolerance.write_double(0.000000001);
        }


		// third method 


	}
}