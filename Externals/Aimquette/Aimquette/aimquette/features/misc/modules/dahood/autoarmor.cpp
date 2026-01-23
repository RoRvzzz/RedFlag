#include "../../../hook.h"
#include <thread>
#include <chrono>

// Attempts to keep armor/defense topped up by writing to common BodyEffects fields used by Da Hood-like games.
// Best-effort: checks multiple typical property names.
void hooks::autoarmor() {
    using namespace std::chrono_literals;
    while (true) {
        if (!globals::focused || !globals::misc::autoarmor) {
            std::this_thread::sleep_for(200ms);
            continue;
        }

        try {
            auto character = globals::instances::lp.instance;
            if (!is_valid_address(character.address)) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            auto body = character.findfirstchild("BodyEffects");
            if (!is_valid_address(body.address)) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            // Common field variants encountered in similar games
            const char* candidates[] = { "Armor", "Armour", "Defense", "Defence" };
            for (const char* field : candidates) {
                auto inst = body.findfirstchild(field);
                if (!inst.address) continue;

                // Prefer int if present; fallback to double
                int current_i = 0;
                double current_d = 0;
                bool wrote = false;
                // Guard reads with try in case of wrong accessor
                try { current_i = inst.read_int_value(); if (current_i < 90) { inst.write_int_value(100); wrote = true; } } catch (...) {}
                if (!wrote) {
                    try { current_d = inst.read_double_value(); if (current_d < 90.0) { inst.write_double_value(100.0); wrote = true; } } catch (...) {}
                }
            }
        } catch (...) {
            // ignore and keep looping
        }

        std::this_thread::sleep_for(50ms);
    }
}


