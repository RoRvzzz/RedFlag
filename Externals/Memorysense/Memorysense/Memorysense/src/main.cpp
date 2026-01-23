#include <main.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <sdk/sdk.h>
#include <features/cache/cache.h>
#include <features/cheats/aimbot/aimbot.h>
#include <features/cheats/console/console.h>

std::atomic<bool> g_is_running = true;

struct ProcessHandler {
    static void WaitForProcess(const char* process_name) {// lalala i real
        while (memory->find_process_id(process_name) == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        memory->attach_to_process(process_name);
        memory->find_module_address(process_name);
    }
};

struct MemoryInitializer {
    static void InitPointers() {
        auto module = memory->get_module_address();
        auto fake_datamodel = memory->read<std::uint64_t>(module + Offsets::FakeDataModel::Pointer);
        g_datamodel = std::make_unique<rbx::datamodel_t>(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
        g_visualengine = std::make_unique<rbx::visualengine_t>(memory->read<std::uint64_t>(module + Offsets::VisualEngine::Pointer));
        g_cache_players = rbx::instance_t(g_datamodel->find_first_child("Players")).get_children<rbx::player_t>();
    }
};

struct RenderInitializer {
    static bool Init() {
        render = std::make_unique<render_t>();
        if (!render->create_window() || !render->create_device() || !render->create_imgui()) {
            CONSOLE_ERROR("Render system initialization failed!");
            return false;
        }
        CONSOLE_INFO("Render system initialized successfully!");
        return true;
    }

    static void Launch() {
        std::thread render_thread([]() {
            render->start_render();
            });
        render_thread.detach();
    }
};

struct PlayersLoop {
    static void Start() {
        std::thread([] {
            while (g_is_running.load()) {
                if (g_datamodel) {
                    if (auto players_inst = g_datamodel->find_first_child("Players")) {
                        std::scoped_lock lock(g_players_mutex);
                        g_cache_players = rbx::instance_t(players_inst).get_children<rbx::player_t>();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }).detach();
    }
};

struct CacheSystem {
    static void Start() {
        std::thread(player_cache::start_cache).detach();
    }
};

std::int32_t main() {
    // Set console title
    SetConsoleTitleA("Memorysense");
    
    CONSOLE_INFO("waiting for roblox...");
    ProcessHandler::WaitForProcess("RobloxPlayerBeta.exe");
    CONSOLE_INFO("found roblox with pid %d", memory->find_process_id("RobloxPlayerBeta.exe"));
    CONSOLE_INFO("attached to roblox, handle %d", memory->get_process_handle());

    MemoryInitializer::InitPointers();
    CONSOLE_DEBUG("found main module address 0x%llx", memory->get_module_address());
    CONSOLE_DEBUG("datamodel @ 0x%llx", g_datamodel->address);
    CONSOLE_DEBUG("visengine @ 0x%llx", g_visualengine->address);

    PlayersLoop::Start();
    CacheSystem::Start();

    if (!RenderInitializer::Init()) {
        CONSOLE_ERROR("Failed to initialize render system!");
        return -1;
    }

    RenderInitializer::Launch();
    
    // Initialize aimbot threads
    memorysense::aimbot::initialize_threads();

    MSG msg;
    while (g_is_running.load() && GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}