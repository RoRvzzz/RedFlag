#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <vector>
#include <future>
#include <memory>
#include <sstream>

#include "../hook.h"
#include "../../util/globals.h"
#include "../../util/classes/classes.h"
#include "../../util/driver/driver.h"
#include "../visuals/visuals.h"
#include "../../util/avatarmanager/avatarmanager.h"
#include "../misc/misc.h"

// Bad Business support structures and functions
struct stringed {
    char data[200];
};

struct PlayerStaticData {
    uintptr_t playerPtr;
    uintptr_t character;
    bool isValid;
    float textColorR;
    float textColorG;
    float textColorB;
    bool hasTextColor;
    std::string name;
    uintptr_t humanoid;
    uintptr_t hrpPtr;
    uintptr_t headPtr;
    uintptr_t hrpPrimitive;
    uintptr_t headPrimitive;
    bool isR6;
    std::vector<std::pair<std::string, uintptr_t>> bodyPartPrimitives;
};

// Helper functions for Bad Business
uintptr_t GetChildren(uintptr_t parent, const std::string& childName) {
    if (!parent) return 0;
    
    try {
        roblox::instance parentInstance(parent);
        auto children = parentInstance.get_children();
        for (const auto& child : children) {
            if (child.get_name() == childName) {
                return child.address;
            }
        }
    } catch (...) {
        return 0;
    }
    return 0;
}

std::vector<uintptr_t> GetAllChildrenNames(uintptr_t parent) {
    std::vector<uintptr_t> result;
    if (!parent) return result;
    
    try {
        roblox::instance parentInstance(parent);
        auto children = parentInstance.get_children();
        for (const auto& child : children) {
            result.push_back(child.address);
        }
    } catch (...) {
        return result;
    }
    return result;
}

std::string ReadString(uintptr_t address) {
    if (!address) return "";
    
    try {
        // Use the existing fetchstring function from instance.cpp
        int length = read<int>(address + 0x18);
        if (length >= 16u) {
            std::uintptr_t padding = read<std::uintptr_t>(address);
            auto character = read<stringed>(padding);
            return character.data;
        }
        auto character = read<stringed>(address);
        return character.data;
    } catch (...) {
        return "";
    }
}

std::string GetClassName(uintptr_t address) {
    if (!address) return "";
    
    try {
        roblox::instance instance(address);
        return instance.get_class_name();
    } catch (...) {
        return "";
    }
}


template<typename T>
std::string to_string_compat(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> pending_count;

public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) : stop(false), pending_count(0) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop.load() || !this->tasks.empty(); });

                        if (this->stop.load() && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        this->pending_count--;
                    }
                    task();
                }
                });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop.load())
                return;

            tasks.emplace(std::forward<F>(f));
            pending_count++;
        }
        condition.notify_one();
    }

    size_t pending_tasks() {
        return pending_count.load();
    }

    ~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

class ThreadSafeGlobals {
private:
    mutable std::mutex players_mutex;
    mutable std::mutex lp_mutex;
    mutable std::mutex instances_mutex;

public:
    void updateCachedPlayers(const std::vector<roblox::player>& new_players) {
        std::lock_guard<std::mutex> lock(players_mutex);
        globals::instances::cachedplayers = new_players;
    }

    std::vector<roblox::player> getCachedPlayers() const {
        std::lock_guard<std::mutex> lock(players_mutex);
        return globals::instances::cachedplayers;
    }

    void updateLocalPlayer(const roblox::player& lp) {
        std::lock_guard<std::mutex> lock(lp_mutex);
        globals::instances::lp = lp;
    }

    roblox::player getLocalPlayer() const {
        std::lock_guard<std::mutex> lock(lp_mutex);
        return globals::instances::lp;
    }

    template<typename T>
    void updateInstance(T& instance, const T& new_value) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        instance = new_value;
    }

    template<typename T>
    T getInstance(const T& instance) const {
        std::lock_guard<std::mutex> lock(instances_mutex);
        return instance;
    }
};

static std::unique_ptr<ThreadPool> g_thread_pool;
static std::unique_ptr<ThreadSafeGlobals> g_safe_globals;

extern class Logger {
public:
    static void CustomLog(int color, const std::string& start, const std::string& message);
} logger;

class ThreadSafePlayerCache {
private:
    std::atomic<bool> is_running;
    std::atomic<bool> should_stop;
    mutable std::mutex update_mutex;
    
    // Performance optimizations
    static constexpr size_t MAX_CACHE_SIZE = 50;
    static constexpr int UPDATE_INTERVAL_MS = 1; // Very fast default cadence (was 50ms, then 10ms)
    static constexpr int PF_UPDATE_INTERVAL_MS = 1; // instant for Phantom Forces
    static constexpr int MICRO_SLEEP_US = 1; // Minimal micro sleeps (was 100us, then 10us)
    static constexpr int PF_MICRO_SLEEP_US = 1; // instant sleeps for Phantom Forces
    
    std::chrono::steady_clock::time_point last_update;
    std::vector<roblox::player> cached_players;
    std::unordered_map<uint64_t, roblox::player> player_map; // Fast lookup by address

    static std::unique_ptr<AvatarManager> avatar_manager;

public:
    ThreadSafePlayerCache() : is_running(false), should_stop(false) {
        cached_players.reserve(MAX_CACHE_SIZE);
    }

    void Initialize() {
        if (!g_safe_globals) {
            g_safe_globals = std::unique_ptr<ThreadSafeGlobals>(new ThreadSafeGlobals());
        }

        if (is_valid_address(globals::instances::datamodel.address)) {
            auto players_service = globals::instances::datamodel.read_service("Players");
            g_safe_globals->updateInstance(globals::instances::players, players_service);
        }
        
        // Ensure the first UpdateCache() call runs immediately (no initial delay)
        last_update = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
    }

    void ForceImmediateUpdate() {
        // Force immediate update by resetting last_update
        last_update = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
        // Clear caches to force fresh scan
        cached_players.clear();
        player_map.clear();
        globals::instances::cachedplayers.clear();
    }

    void UpdateCache() {
        if (!g_safe_globals) {
            g_safe_globals = std::unique_ptr<ThreadSafeGlobals>(new ThreadSafeGlobals());
        }

        // Game ID detection - only scan once per game session
        static uint64_t last_detected_game_id = 0;
        static bool game_structure_cached = false;
        static std::chrono::steady_clock::time_point last_game_scan = std::chrono::steady_clock::now();
        
        uint64_t current_game_id = 0;
        bool isPhantomForcesByPlaceId = false;
        bool isArsenalByPlaceId = false;
        bool isBadBusinessByPlaceId = false;
        
        if (is_valid_address(globals::instances::datamodel.address)) {
            try {
                current_game_id = read<uint64_t>(globals::instances::datamodel.address + offsets::PlaceId);
                isPhantomForcesByPlaceId = (current_game_id == 292439477ULL); // Phantom Forces
                isArsenalByPlaceId = (current_game_id == 286090429ULL); // Arsenal
                isBadBusinessByPlaceId = (current_game_id == 3233893879ULL); // Bad Business
                
                // Debug game detection
                printf("Current game PlaceId: %llu\n", current_game_id);
                if (current_game_id == 3233893879ULL) {
                    printf("Bad Business detected! PlaceId: %llu\n", current_game_id);
                } else if (current_game_id != 0) {
                    printf("Unknown game detected! PlaceId: %llu (might be Bad Business with different ID)\n", current_game_id);
                }
                printf("Game detection: PF=%s, Arsenal=%s, BadBusiness=%s\n", 
                       isPhantomForcesByPlaceId ? "true" : "false",
                       isArsenalByPlaceId ? "true" : "false", 
                       isBadBusinessByPlaceId ? "true" : "false");
            } catch (...) { 
                isPhantomForcesByPlaceId = false;
                isArsenalByPlaceId = false;
                isBadBusinessByPlaceId = false;
            }
        }
        
        // Only rescan if game changed or first time
        bool game_changed = (current_game_id != last_detected_game_id);
        bool needs_initial_scan = !game_structure_cached;
        
        if (game_changed || needs_initial_scan) {
            // New game detected - reset cache and scan immediately
            last_detected_game_id = current_game_id;
            game_structure_cached = true;
            last_game_scan = std::chrono::steady_clock::now();
            
            // Clear old cache for new game
            cached_players.clear();
            player_map.clear();
            globals::instances::cachedplayers.clear();
            
            // Force immediate scan - no delay
            last_update = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
        } else {
            // Game structure already cached - scan very frequently for new players
            int interval_ms = isPhantomForcesByPlaceId ? PF_UPDATE_INTERVAL_MS : UPDATE_INTERVAL_MS;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
            
            // Scan immediately if enough time has passed (very short interval now)
            if (elapsed.count() < interval_ms) {
                // Minimal sleep - just yield to other threads
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                return;
            }
        }

        std::unique_lock<std::mutex> lock(update_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return;
        }

        if (should_stop.load()) {
            return;
        }

        std::vector<roblox::player> tempplayers;
        tempplayers.reserve(MAX_CACHE_SIZE); // Pre-allocate to avoid reallocations

        try {


                    // Standard Roblox player collection
        if (!is_valid_address(globals::instances::players.address)) {
            return;
        }

        auto players_service = globals::instances::players;
        auto children = players_service.get_children();

            // Clear old cache if too many players
            if (children.size() > MAX_CACHE_SIZE) {
                player_map.clear();
            }

            for (roblox::instance& player : children) {
                if (should_stop.load()) break;

                // Throttle scan: 20ms for Phantom Forces workspace reading, else keep prior micro-sleep
                if (isPhantomForcesByPlaceId) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(MICRO_SLEEP_US));
                }

                if (!is_valid_address(player.address))
                    continue;

                if (player.get_class_name() != "Player")
                    continue;

                auto character = player.model_instance();
                if (!is_valid_address(character.address))
                    continue;

                // Check if player is already cached and valid
                auto it = player_map.find(player.address);
                if (it != player_map.end()) {
                    // Use cached player if still valid
                    tempplayers.push_back(it->second);
                    continue;
                }

                roblox::player entity = buildPlayerEntity(player, character);

                // Cache the player
                player_map[player.address] = entity;

                if (is_valid_address(globals::instances::localplayer.address)) {
                    if (entity.name == globals::instances::localplayer.get_name()) {
                        g_safe_globals->updateLocalPlayer(entity);
                    }
                }

                // Minimal sleep for faster rescan - just yield to other threads
                if (isPhantomForcesByPlaceId) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100)); // Minimal sleep
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(50)); // Very minimal sleep
                }
                tempplayers.push_back(entity);
            }

            // Add bots to the cache if enabled
            if (globals::misc::npc_check) {
                std::vector<roblox::player> bots = GetBots(::base_address);
                tempplayers.insert(tempplayers.end(), bots.begin(), bots.end());
            }

            // Update cached players
            cached_players = std::move(tempplayers);
            g_safe_globals->updateCachedPlayers(cached_players);
            last_update = std::chrono::steady_clock::now();

        }
        catch (const std::exception& e) {
            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "CACHE_ERROR",
                "Player cache update failed: " + std::string(e.what()));
        }
        catch (...) {
            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "CACHE_ERROR",
                "Player cache update failed with unknown exception");
        }
    }

private:
    roblox::player buildPlayerEntity(roblox::instance& player, roblox::instance& character) {
        std::unordered_map<std::string, roblox::instance> cachedchildren;

        for (auto& child : character.get_children()) {
            cachedchildren[child.get_name()] = child;
        }

        roblox::player entity;
        entity.name = player.get_name();
        entity.displayname = player.get_humdisplayname();
        entity.head = cachedchildren["Head"];
        entity.hrp = cachedchildren["HumanoidRootPart"];
        entity.humanoid = cachedchildren["Humanoid"];

        if (cachedchildren["LeftUpperLeg"].address) {
            entity.rigtype = 1;
            entity.uppertorso = cachedchildren["UpperTorso"];
            entity.lowertorso = cachedchildren["LowerTorso"];
            entity.lefthand = cachedchildren["LeftHand"];
            entity.righthand = cachedchildren["RightHand"];
            entity.leftlowerarm = cachedchildren["LeftLowerArm"];
            entity.rightlowerarm = cachedchildren["RightLowerArm"];
            entity.leftupperarm = cachedchildren["LeftUpperArm"];
            entity.rightupperarm = cachedchildren["RightUpperArm"];
            entity.leftfoot = cachedchildren["LeftFoot"];
            entity.leftlowerleg = cachedchildren["LeftLowerLeg"];
            entity.leftupperleg = cachedchildren["LeftUpperLeg"];
            entity.rightlowerleg = cachedchildren["RightLowerLeg"];
            entity.rightfoot = cachedchildren["RightFoot"];
            entity.rightupperleg = cachedchildren["RightUpperLeg"];
        }
        else {
            entity.rigtype = 0;
            entity.uppertorso = cachedchildren["Torso"];
            entity.lefthand = cachedchildren["Left Arm"];
            entity.righthand = cachedchildren["Right Arm"];
            entity.rightfoot = cachedchildren["Left Leg"];
            entity.leftfoot = cachedchildren["Right Leg"];
        }

        if (globals::instances::gamename != "Arsenal") {
            entity.health = entity.humanoid.read_health();
            entity.maxhealth = entity.humanoid.read_maxhealth();
        }
        else {
            try {
                entity.health = player.findfirstchild("NRPBS").findfirstchild("Health").read_double_value();
                entity.maxhealth = player.findfirstchild("NRPBS").findfirstchild("MaxHealth").read_double_value();
            }
            catch (...) {
                entity.health = 0;
                entity.maxhealth = 100;
            }
        }

        entity.userid = player.read_userid();
        entity.instance = character;
        entity.main = player;
        entity.bodyeffects = character.findfirstchild("BodyEffects");
        entity.displayname = player.get_displayname();

        if (entity.displayname.empty()) {
            entity.displayname = entity.humanoid.get_humdisplayname();
        }

        auto tool = character.read_service("Tool");
        entity.tool = tool;
        entity.toolname = tool.address ? tool.get_name() : "NONE";

        // Assign team information for team checking
        entity.team = player.Team();

        return entity;
    }

public:
    void Cleanup() {
        should_stop = true;
        std::lock_guard<std::mutex> lock(update_mutex);

        if (g_safe_globals) {
            auto empty_players = std::vector<roblox::player>();
            g_safe_globals->updateCachedPlayers(empty_players);

            auto players_service = roblox::instance();
            g_safe_globals->updateInstance(globals::instances::players, players_service);
        }
    }

    void Refresh() {
        Cleanup();
        should_stop = false;
        Initialize();
    }

    void stop() {
        should_stop = true;
    }
};

std::unique_ptr<AvatarManager> ThreadSafePlayerCache::avatar_manager;

class ThreadSafeHookManager {
private:
    std::atomic<bool> hooks_running;
    std::vector<std::thread> hook_threads;

public:
    ThreadSafeHookManager() : hooks_running(false) {}

    void launchThreads() {
        bool expected = false;
        if (!globals::firstreceived)return;
        if (!hooks_running.compare_exchange_strong(expected, true)) {
            return;
        }

        logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            "HOOKS", "Launching individual hook threads...");

        try {
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting cached_input_objectzz thread");
                roblox::cached_input_objectzz();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting cache thread");
                hooks::cache();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting silent thread");
                hooks::silent();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting speed thread");
                hooks::speed();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting jumppower thread");
                hooks::jumppower();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting max_fps thread");
                hooks::max_fps();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting antistomp thread");
                hooks::antistomp();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting flight thread");
                hooks::flight();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting spin360 thread");
                hooks::spinbot();
            });



            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting rapidfire thread");
                hooks::rapidfire();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "COMBAT", "Starting triggerbot thread");
                hooks::triggerbot();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting autoreload thread");
                hooks::autoreload();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting autoarmor thread");
                hooks::autoarmor();
                });

            

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting autostomp thread");
                hooks::autostomp();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting voidhide thread");
                hooks::voidhide();
                });
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting headless thread");
                hooks::headless();
                });
            // Hip Height thread removed
            // hook_threads.emplace_back([]() {
            //     logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting hipheight thread");
            //     hooks::hipheight();
            //     });
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting listener thread");
                hooks::listener();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting antisit thread");
                hooks::antisit();
                });
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "COMBAT", "Starting AIMBOT (combat) thread");
                hooks::aimbot();
                });

            // Animation and enhanced movement hooks
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "THREAD", "Starting animation thread");
                hooks::animation();
                });

            // CFrame thread removed
            // hook_threads.emplace_back([]() {
            //     logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "MOVEMENT", "Starting cframe movement thread");
            //     hooks::cframe();
            //     });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "COMBAT", "Starting anti-aim thread");
                hooks::antiaim();
                });

            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "MISC", "Starting desync thread");
                hooks::desync();
                });
            
            hook_threads.emplace_back([]() {
                logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "MISC", "Starting spam tp thread");
                hooks::spam_tp();
                });
            


            for (auto& thread : hook_threads) {
                thread.detach();
            }

            logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                "HOOKS", "Successfully launched " + to_string_compat(hook_threads.size()) + " individual hook threads");
            logger.CustomLog(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                "COMBAT", "Aimbot and Triggerbot threads are running independently");

        }
        catch (const std::exception& e) {
            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY,
                "HOOK_ERROR", "Failed to launch hooks: " + std::string(e.what()));
            hooks_running = false;
        }
    }

    void stopHooks() {
        hooks_running = false;

        globals::unattach = true;

        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
            "HOOKS", "Signaling all hook threads to stop...");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        hook_threads.clear();

        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            "HOOKS", "All hook threads stop signal sent");
    }

    bool isRunning() const {
        return hooks_running.load();
    }

    size_t getThreadCount() const {
        return hook_threads.size();
    }
};

static ThreadSafePlayerCache g_player_cache;
static ThreadSafeHookManager g_hook_manager;

namespace hooking {
    void launchThreads() {
        if (!g_safe_globals) {
            g_safe_globals = std::unique_ptr<ThreadSafeGlobals>(new ThreadSafeGlobals());
        }

        logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            "INIT", "Launching individual threads for maximum performance...");

        g_hook_manager.launchThreads();

        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            "SUCCESS", "All " + to_string_compat(g_hook_manager.getThreadCount()) + " hooks running on separate threads");
    }

    void stopThreads() {
        g_hook_manager.stopHooks();
        g_player_cache.stop();
    }

    bool isRunning() {
        return g_hook_manager.isRunning();
    }

    void forceRescan() {
        g_player_cache.ForceImmediateUpdate();
    }
}

class SimplePlayerCache {
public:
    static void Initialize() {
        if (is_valid_address(globals::instances::datamodel.address)) {
            globals::instances::players = globals::instances::datamodel.read_service("Players");
        }
    }

    static void UpdateCache() {
        std::vector<roblox::player> tempplayers;

        if (!is_valid_address(globals::instances::players.address)) {
            return;
        }

        // Game ID detection - only scan once per game session
        static uint64_t last_detected_game_id = 0;
        static bool game_structure_cached = false;
        
        uint64_t current_game_id = 0;
        bool isPhantomForcesByPlaceId = false;
        bool isArsenalByPlaceId = false;
        bool isBadBusinessByPlaceId = false;
        
        if (is_valid_address(globals::instances::datamodel.address)) {
            try {
                current_game_id = read<uint64_t>(globals::instances::datamodel.address + offsets::PlaceId);
                isPhantomForcesByPlaceId = (current_game_id == 292439477ULL); // Phantom Forces
                isArsenalByPlaceId = (current_game_id == 286090429ULL); // Arsenal
                isBadBusinessByPlaceId = (current_game_id == 3233893879ULL); // Bad Business
            }
            catch (...) {
                isPhantomForcesByPlaceId = false;
                isArsenalByPlaceId = false;
                isBadBusinessByPlaceId = false;
            }
        }
        
        // Only scan if game changed or first time
        bool game_changed = (current_game_id != last_detected_game_id);
        bool needs_initial_scan = !game_structure_cached;
        
        if (game_changed || needs_initial_scan) {
            // New game detected - scan once and cache structure
            last_detected_game_id = current_game_id;
            game_structure_cached = true;
        }
        // Always continue scanning to catch new players
        
        if (isPhantomForcesByPlaceId) {
            printf("Phantom Forces detected - using enhanced player detection\n");
            
            // Use cached workspace approach for better performance
            static uintptr_t cachedWorkspace = 0;
            static auto lastWorkspaceCheck = std::chrono::steady_clock::now();
            
            auto now = std::chrono::steady_clock::now();
            
            if (!cachedWorkspace || (now - lastWorkspaceCheck) > std::chrono::seconds(5)) {
                auto children = GetAllChildrenNames(globals::instances::datamodel.address);
                for (uintptr_t child : children) {
                    if (GetClassName(child) == "Workspace") {
                        cachedWorkspace = child;
                        lastWorkspaceCheck = now;
                        break;
                    }
                }
            }
            
            if (cachedWorkspace) {
                auto players = GetChildren(cachedWorkspace, "Players");
                if (players) {
                    auto playersChildren = GetAllChildrenNames(players);
                    
                    for (uintptr_t teamFolder : playersChildren) {
                        if (GetClassName(teamFolder) != "Folder") continue;
                        
                        auto teamChildren = GetAllChildrenNames(teamFolder);
                        for (uintptr_t playerModel : teamChildren) {
                            if (GetClassName(playerModel) != "Model") continue;
                            
                            roblox::player entity;
                            entity.name = ReadString(playerModel + offsets::Name);
                            if (entity.name.empty()) continue;
                            
                            entity.displayname = entity.name;
                            entity.instance = roblox::instance(playerModel);
                            entity.main = roblox::instance(playerModel);
                            entity.team = roblox::instance(teamFolder);
                            
                            // Find Humanoid first for health reading
                            entity.humanoid = roblox::instance(0);
                            auto playerChildren = GetAllChildrenNames(playerModel);
                            
                            for (uintptr_t child : playerChildren) {
                                if (GetClassName(child) == "Humanoid") {
                                    entity.humanoid = roblox::instance(child);
                                    break;
                                }
                            }
                            
                            // Read health from Humanoid for healthbar functionality
                            if (is_valid_address(entity.humanoid.address)) {
                                try {
                                    float rawHealth = entity.humanoid.read_health();
                                    float rawMaxHealth = entity.humanoid.read_maxhealth();
                                    entity.health = static_cast<int>(rawHealth);
                                    entity.maxhealth = static_cast<int>(rawMaxHealth);
                                    printf("PF: Found Humanoid for %s - Raw Health: %.1f/%.1f, Int Health: %d/%d\n", 
                                           entity.name.c_str(), rawHealth, rawMaxHealth, entity.health, entity.maxhealth);
                                } catch (...) {
                                    entity.health = 100;
                                    entity.maxhealth = 100;
                                    printf("PF: Failed to read health from Humanoid for %s, using defaults\n", entity.name.c_str());
                                }
                            } else {
                                entity.health = 100;
                                entity.maxhealth = 100;
                                printf("PF: No Humanoid found for %s, using default health\n", entity.name.c_str());
                            }
                            
                            // Enhanced part detection using GUI/Spotlight heuristics
                            uintptr_t headPart = 0;
                            uintptr_t torsoPart = 0;
                            uintptr_t nameTagPart = 0;
                            
                            std::vector<uintptr_t> allBodyParts;
                            for (uintptr_t child : playerChildren) {
                                std::string className = GetClassName(child);
                                if (className == "Part" || className == "MeshPart") {
                                    allBodyParts.push_back(child);
                                }
                            }
                            
                            // Sort parts by Y position (highest first for head detection)
                            std::sort(allBodyParts.begin(), allBodyParts.end(), [&](uintptr_t a, uintptr_t b) {
                                uintptr_t primitiveA = read<uintptr_t>(a + offsets::Primitive);
                                uintptr_t primitiveB = read<uintptr_t>(b + offsets::Primitive);
                                Vector3 posA = read<Vector3>(primitiveA + offsets::Position);
                                Vector3 posB = read<Vector3>(primitiveB + offsets::Position);
                                return posA.y > posB.y;
                            });
                            
                            for (uintptr_t part : allBodyParts) {
                                auto partChildren = GetAllChildrenNames(part);
                                
                                for (uintptr_t partChild : partChildren) {
                                    std::string childName = ReadString(partChild + offsets::Name);
                                    if (childName == "NameTagGui") {
                                        nameTagPart = part;
                                        break;
                                    }
                                }
                                
                                bool hasGUI = false;
                                bool hasSpotlight = false;
                                
                                for (uintptr_t partChild : partChildren) {
                                    std::string childClass = GetClassName(partChild);
                                    if (childClass == "BillboardGui") {
                                        hasGUI = true;
                                    }
                                    if (childClass == "SpotLight") {
                                        hasSpotlight = true;
                                    }
                                }
                                
                                if (hasGUI) {
                                    headPart = part;
                                }
                                if (hasSpotlight) {
                                    torsoPart = part;
                                }
                            }
                            
                            // Set head and HRP
                            if (headPart) {
                                entity.head = roblox::instance(headPart);
                            }
                            if (torsoPart) {
                                entity.hrp = roblox::instance(torsoPart);
                            } else if (headPart) {
                                entity.hrp = roblox::instance(headPart);
                            }
                            
                            // Skip if no valid parts found
                            if (!is_valid_address(entity.head.address) || !is_valid_address(entity.hrp.address)) {
                                printf("PF: Skipping %s - no valid head/HRP found\n", entity.name.c_str());
                                continue;
                            }
                            
                            // Skip local player
                            if (entity.name == globals::instances::localplayer.get_name()) {
                                globals::instances::lp = entity;
                                continue;
                            }
                            
                            tempplayers.push_back(entity);
                            printf("PF: Added player %s with health %d/%d\n", entity.name.c_str(), entity.health, entity.maxhealth);
                        }
                    }
                }
                
                if (!tempplayers.empty()) {
                    visuals::espData.cachedmutex.lock();
                    globals::instances::cachedplayers = tempplayers;
                    visuals::espData.cachedmutex.unlock();
                    printf("PF: Updated cached players with %zu players\n", tempplayers.size());
                    
                    // Debug: Print health values for each player in cache
                    for (const auto& cachedPlayer : tempplayers) {
                        printf("PF Cache: %s - Health: %d/%d (%.1f%%)\n", 
                               cachedPlayer.name.c_str(), cachedPlayer.health, cachedPlayer.maxhealth,
                               (cachedPlayer.maxhealth > 0) ? (static_cast<float>(cachedPlayer.health) / cachedPlayer.maxhealth * 100.0f) : 0.0f);
                    }
                    return;
                }
            }
            // If no players found, fall through to default path
        }

        if (isBadBusinessByPlaceId) {
            printf("Bad Business detected - using direct character collection\n");
            std::vector<PlayerStaticData> newStaticData;
            
            uintptr_t DataModel = globals::instances::datamodel.address;
            if (!DataModel) {
                return;
            }

            uintptr_t Workspace = GetChildren(DataModel, "Workspace");
            if (!Workspace) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return;
            }

            uintptr_t Characters = GetChildren(Workspace, "Characters");
            if (!Characters) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return;
            }

            auto characterList = GetAllChildrenNames(Characters);
            printf("Bad Business: Found %zu characters\n", characterList.size());

            for (uintptr_t character : characterList) {

                PlayerStaticData data;
                data.playerPtr = character;
                data.character = character;
                data.isValid = false;
                data.textColorR = 0;
                data.textColorG = 0;
                data.textColorB = 0;
                data.hasTextColor = false;

                data.name = ReadString(character + offsets::Name);
                printf("Bad Business: Processing character %s at 0x%p\n", data.name.c_str(), character);
                if (data.name.empty()) continue;

                data.humanoid = 0;

                uintptr_t Clothes = GetChildren(data.character, "Clothes");

                if (!Clothes) continue; 

                uintptr_t Body = GetChildren(data.character, "Body");
                if (!Body) continue;

                uintptr_t Chest = GetChildren(Body, "Chest");
                if (!Chest) continue;

                uintptr_t Head = GetChildren(Body, "Head");
                if (!Head) continue;

                data.hrpPtr = Chest;
                data.headPtr = Head;

                data.hrpPrimitive = read<uintptr_t>(data.hrpPtr + offsets::Primitive);
                data.headPrimitive = read<uintptr_t>(data.headPtr + offsets::Primitive);
                printf("Bad Business: %s - HRP: 0x%p, Head: 0x%p, HRP Primitive: 0x%p, Head Primitive: 0x%p\n", 
                       data.name.c_str(), data.hrpPtr, data.headPtr, data.hrpPrimitive, data.headPrimitive);
                if (!data.hrpPrimitive || !data.headPrimitive) continue;

                data.isR6 = false; 

                std::vector<std::string> essentialParts = {
                    "LeftHand", "Abdomen", "RightLeg", "Hips", "RightFoot",
                    "LeftArm", "LeftForearm", "RightForearm", "RightForeleg",
                    "RightHand", "Chest", "RightArm", "Neck", "Head",
                    "LeftForeleg", "LeftLeg", "LeftFoot"
                };

                for (const std::string& partName : essentialParts) {
                    uintptr_t bodyPart = GetChildren(Body, partName);
                    if (bodyPart) {
                        uintptr_t primitive = read<uintptr_t>(bodyPart + offsets::Primitive);
                        if (primitive) {
                            data.bodyPartPrimitives.emplace_back(partName, primitive);
                        }
                    }
                }

                data.isValid = true;
                newStaticData.push_back(std::move(data));
                printf("Bad Business: Successfully added player %s\n", data.name.c_str());
            }

            // Convert Bad Business data to roblox::player format for compatibility
            printf("Bad Business: Converting %zu players to roblox::player format\n", newStaticData.size());
            for (const auto& staticData : newStaticData) {
                if (!staticData.isValid) {
                    printf("Bad Business: Skipping invalid player data\n");
                    continue;
                }

                roblox::player entity;
                entity.name = staticData.name;
                entity.displayname = staticData.name;
                
                // Create instances for head and HRP
                roblox::instance headInstance(staticData.headPtr);
                roblox::instance hrpInstance(staticData.hrpPtr);
                
                printf("Bad Business: Setting head (0x%p) and HRP (0x%p) for %s\n", 
                       staticData.headPtr, staticData.hrpPtr, entity.name.c_str());
                
                entity.head = headInstance;
                entity.hrp = hrpInstance;
                entity.humanoid = roblox::instance(staticData.humanoid);
                
                // Try to read health from humanoid
                try {
                    entity.health = static_cast<int>(entity.humanoid.read_health());
                    entity.maxhealth = static_cast<int>(entity.humanoid.read_maxhealth());
                } catch (...) {
                    // Fallback to default values
                    entity.health = 100;
                    entity.maxhealth = 100;
                }
                
                // Set rigtype to R6
                entity.rigtype = 0; // R6
                
                // Set the main player instance to the character for Bad Business
                entity.main = roblox::instance(staticData.character);
                entity.instance = roblox::instance(staticData.character);
                
                // Set userid (might need adjustment for Bad Business)
                entity.userid = roblox::instance(0);
                
                // Check if this is the local player - SKIP IT COMPLETELY
                if (entity.name == globals::instances::localplayer.get_name()) {
                    printf("Bad Business: Found local player %s - SKIPPING COMPLETELY\n", entity.name.c_str());
                    continue; // Skip local player entirely
                }
                
                // Set tool information
                entity.tool = roblox::instance(0);
                entity.toolname = "NONE";
                
                // Set team information
                entity.team = roblox::instance(0);
                
                // Set body effects
                entity.bodyeffects = roblox::instance(0);
                
                // Initialize missing fields
                entity.address = staticData.character;
                entity.torso = roblox::instance(0);
                entity.uppertorso = roblox::instance(0);
                entity.lowertorso = roblox::instance(0);
                entity.rightupperarm = roblox::instance(0);
                entity.leftupperarm = roblox::instance(0);
                entity.rightlowerarm = roblox::instance(0);
                entity.leftlowerarm = roblox::instance(0);
                entity.lefthand = roblox::instance(0);
                entity.righthand = roblox::instance(0);
                entity.leftupperleg = roblox::instance(0);
                entity.rightupperleg = roblox::instance(0);
                entity.rightlowerleg = roblox::instance(0);
                entity.leftlowerleg = roblox::instance(0);
                entity.rightfoot = roblox::instance(0);
                entity.leftfoot = roblox::instance(0);
                

                

                entity.flag = "";
                entity.pictureDownloaded = false;
                entity.armor = 0;
                entity.status = 0;
                entity.bones.clear();
                
                // Add to temporary players list
                tempplayers.push_back(entity);
                printf("Bad Business: Successfully converted player %s to roblox::player format\n", entity.name.c_str());
            }
            
            // Update cached players
            printf("Bad Business: About to update cache with %zu players\n", tempplayers.size());
            visuals::espData.cachedmutex.lock();
            globals::instances::cachedplayers = tempplayers;
            visuals::espData.cachedmutex.unlock();
            printf("Bad Business: Updated cached players with %zu players\n", tempplayers.size());
            return;
        }



        for (roblox::instance& player : globals::instances::players.get_children()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));

            if (!is_valid_address(player.address))
                continue;

            if (player.get_class_name() != "Player")
                continue;

            auto character = player.model_instance();
            if (!is_valid_address(character.address))
                continue;

            std::unordered_map<std::string, roblox::instance> cachedchildren;
            for (auto& child : character.get_children()) {
                cachedchildren[child.get_name()] = child;
            }

            roblox::player entity;
            entity.name = player.get_name();
            entity.displayname = player.get_humdisplayname();
            entity.head = cachedchildren["Head"];
            entity.hrp = cachedchildren["HumanoidRootPart"];
            entity.humanoid = cachedchildren["Humanoid"];

            if (cachedchildren["LeftUpperLeg"].address) {
                entity.rigtype = 1;
                entity.uppertorso = cachedchildren["UpperTorso"];
                entity.lowertorso = cachedchildren["LowerTorso"];
                entity.lefthand = cachedchildren["LeftHand"];
                entity.righthand = cachedchildren["RightHand"];
                entity.leftlowerarm = cachedchildren["LeftLowerArm"];
                entity.rightlowerarm = cachedchildren["RightLowerArm"];
                entity.leftupperarm = cachedchildren["LeftUpperArm"];
                entity.rightupperarm = cachedchildren["RightUpperArm"];
                entity.leftfoot = cachedchildren["LeftFoot"];
                entity.leftlowerleg = cachedchildren["LeftLowerLeg"];
                entity.leftupperleg = cachedchildren["LeftUpperLeg"];
                entity.rightlowerleg = cachedchildren["RightLowerLeg"];
                entity.rightfoot = cachedchildren["RightFoot"];
                entity.rightupperleg = cachedchildren["RightUpperLeg"];
            }
            else {
                entity.rigtype = 0;
                entity.uppertorso = cachedchildren["Torso"];
                entity.lefthand = cachedchildren["Left Arm"];
                entity.righthand = cachedchildren["Right Arm"];
                entity.rightfoot = cachedchildren["Left Leg"];
                entity.leftfoot = cachedchildren["Right Leg"];
            }

            if (globals::instances::gamename != "Arsenal") {
                entity.health = entity.humanoid.read_health();
                entity.maxhealth = entity.humanoid.read_maxhealth();
            }
            else {
                try {
                    entity.health = player.findfirstchild("NRPBS").findfirstchild("Health").read_double_value();
                    entity.maxhealth = player.findfirstchild("NRPBS").findfirstchild("MaxHealth").read_double_value();
                }
                catch (...) {
                    entity.health = 0;
                    entity.maxhealth = 100;
                }
            }

            entity.userid = player.read_userid();
            entity.instance = character;
            entity.main = player;
            entity.bodyeffects = character.findfirstchild("BodyEffects");
            entity.displayname = player.get_displayname();

            if (entity.displayname.empty()) {
                entity.displayname = entity.humanoid.get_humdisplayname();
            }



            if (entity.name == globals::instances::localplayer.get_name()) {
                globals::instances::lp = entity;
            }

            auto tool = character.read_service("Tool");
            entity.tool = tool;
            entity.toolname = tool.address ? tool.get_name() : "NONE";

            // Assign team information for team checking
            entity.team = player.Team();

            std::this_thread::sleep_for(std::chrono::microseconds(10));
            tempplayers.push_back(entity);
        }

        visuals::espData.cachedmutex.lock();
        globals::instances::cachedplayers = tempplayers;
        visuals::espData.cachedmutex.unlock();
    }

    static void Cleanup() {
        globals::instances::players.address = 0;
        globals::instances::cachedplayers.clear();
    }

    static void Refresh() {
        Cleanup();
        Initialize();
    }
};

void player_cache::Initialize() {
    try {
        SimplePlayerCache::Initialize();
        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "CACHE",
            "Player cache initialized successfully");
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "INIT_ERROR",
            "Failed to initialize player cache");
    }
}

void player_cache::UpdateCache() {
    try {
        SimplePlayerCache::UpdateCache();
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "UPDATE_ERROR",
            "Failed to update player cache");
    }
}

void player_cache::Cleanup() {
    try {
        SimplePlayerCache::Cleanup();
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "CLEANUP_ERROR",
            "Failed to cleanup player cache");
    }
}

void player_cache::Refresh() {
    try {
        SimplePlayerCache::Refresh();
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "REFRESH_ERROR",
            "Failed to refresh player cache");
    }
}