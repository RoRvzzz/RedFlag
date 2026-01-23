
#include <windows.h> 
#include <TlHelp32.h> 
#include <string> 
#include <iostream> 


#include <Psapi.h>
#include "../../cheat/features/features.h"
#include "../../overlay/overlay.hpp"
std::unordered_map<std::string, std::atomic<std::chrono::steady_clock::time_point>> thread_heartbeats;

int threadfails = 0;
std::mutex heartbeat_mutex;
void update_heartbeat(const std::string& thread_name) {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);
    thread_heartbeats[thread_name] = std::chrono::steady_clock::now();
}



bool check_thread_health(const std::string& thread_name, int timeout_seconds = 5) {
    auto it = thread_heartbeats.find(thread_name);
    if (it == thread_heartbeats.end()) return false;
    auto last_beat = it->second.load();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - last_beat).count() < timeout_seconds;
}
void ThreadSafetyCheck() {
    std::vector<std::thread> threads;

    threads.emplace_back(std::thread([]() {
        while (true) {
            update_heartbeat("Aimbot");
            RBX::InitializeAimbot();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }));

    threads.emplace_back(std::thread([]() {
        while (true) {
            update_heartbeat("Silent Aim");
            RBX::initsilent();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }));

    threads.emplace_back(std::thread([]() {
        while (true) {
            update_heartbeat("Player");
            RBX::Instance::updatePlayers();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }));

    threads.emplace_back(std::thread([]() {
        while (true) {
            update_heartbeat("Overlay");
            overlay::render();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }));


    // **Thread Management**
    for (auto& t : threads) {
        t.detach();
    }
}
void debug() {

    if (globals::debug_info) {
        RBX::TaskScheduler Instance;
        RBX::Instance Instance2;

      
        auto Viewp = Instance2.GetViewportSize();
        uintptr_t renderjob = Instance.GetJobByName("RenderJob");
        uintptr_t renderaddress = Umodule::read<uintptr_t>(renderjob);
        uintptr_t renderview = Instance2.GetRenderView();
        uintptr_t ScriptContext = Instance2.GetScriptContext();
        uintptr_t ScriptContextDM = Instance2.GetDataModelViaScriptContext();

        if (Instance.GetScheduler() == 0) {
            utils::output::printaddress("Scheduler: ", (void*)Instance.GetScheduler());
        }
        if (renderaddress == 0) {
            utils::output::printaddress("Render_Job: ", (void*)renderaddress);
        }
       
       
        std::pair<int, int> viewportSize = Instance2.GetViewportSize();
        utils::output::info("Viewport Size: " + std::to_string(viewportSize.first) + ", " + std::to_string(viewportSize.second));

        utils::output::printaddress("Render_View: ", (void*)renderview);
        utils::output::printaddress("Visual_Engine: ", (void*)globals::visualengine.address);
        utils::output::printaddress("Script_Context: ", (void*)ScriptContext);
        utils::output::printaddress("Script_Context To Data_Model: ", (void*)ScriptContextDM);
        utils::output::printaddress("Data_Model: ", (void*)globals::game.address);

        utils::output::info("Lplr Name: " + globals::players.GetLocalPlayer().GetName());
    }
}
