#include <thread>
#include "thread.h"


void rage::hook() {
	std::thread(utils::fly).detach();
	std::thread(utils::antistomp).detach();
	std::thread(utils::walkspeed).detach();
	std::thread(utils::jump).detach();
	std::thread(utils::noclip).detach();
	std::thread(utils::autoreload).detach();
	std::thread(utils::rapidfire).detach();
	std::thread(utils::cframe).detach();
	std::thread(utils::waypoint_loop).detach();
	std::thread(utils::desync_loop_thread).detach();

}