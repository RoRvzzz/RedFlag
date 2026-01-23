#include "../../combat/combat.h"
#include <thread>
#include <chrono>
#include <Windows.h>
#include "../../../util/console/console.h"
#include <iostream>
#include <random>
#include "../../hook.h"
#include "../../../util/driver/driver.h"

#define max
#undef max
#define min
#undef min

void hooks::animation() {
    while (true) {
        if (!globals::combat::animation_enable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto localplayer = globals::instances::localplayer.model_instance();
        if (!localplayer.address)
            continue;

        auto animate = localplayer.findfirstchild("Animate");
        if (!animate.address)
            continue;

        auto idle1 = animate.findfirstchild("idle").findfirstchild("Animation1");
        auto idle2 = animate.findfirstchild("idle").findfirstchild("Animation2");
        auto run = animate.findfirstchild("run").findfirstchild("RunAnim");
        auto walk = animate.findfirstchild("walk").findfirstchild("WalkAnim");
        auto jump = animate.findfirstchild("jump").findfirstchild("JumpAnim");
        auto fall = animate.findfirstchild("fall").findfirstchild("FallAnim");
        auto climb = animate.findfirstchild("climb").findfirstchild("ClimbAnim");
        auto swim = animate.findfirstchild("swim").findfirstchild("Swim");

        // Helper function to get animation ID based on pack type
        auto getAnimationID = [](int pack_type, int anim_type) -> std::string {
            // anim_type: 0=idle1, 1=idle2, 2=run, 3=walk, 4=jump, 5=fall, 6=climb, 7=swim
            switch (pack_type) {
            case 0: { // Default - return empty string to keep original animations
                return "";
            }
            case 1: { // Ninja
                const char* ids[] = { "http://www.roblox.com/asset/?id=656117400", "http://www.roblox.com/asset/?id=656118341", "http://www.roblox.com/asset/?id=656118852", "http://www.roblox.com/asset/?id=656121766", "http://www.roblox.com/asset/?id=656117878", "http://www.roblox.com/asset/?id=656115606", "http://www.roblox.com/asset/?id=656114359", "http://www.roblox.com/asset/?id=656119721" };
                return ids[anim_type];
            }
            case 2: { // Robot
                const char* ids[] = { "http://www.roblox.com/asset/?id=616136790", "http://www.roblox.com/asset/?id=616138447", "http://www.roblox.com/asset/?id=616140816", "http://www.roblox.com/asset/?id=616146177", "http://www.roblox.com/asset/?id=616139451", "http://www.roblox.com/asset/?id=616134815", "http://www.roblox.com/asset/?id=616133594", "http://www.roblox.com/asset/?id=616143378" };
                return ids[anim_type];
            }
            case 3: { // Zombie
                const char* ids[] = { "http://www.roblox.com/asset/?id=616158929", "http://www.roblox.com/asset/?id=616160636", "http://www.roblox.com/asset/?id=616163682", "http://www.roblox.com/asset/?id=616168032", "http://www.roblox.com/asset/?id=616161997", "http://www.roblox.com/asset/?id=616157476", "http://www.roblox.com/asset/?id=616156119", "http://www.roblox.com/asset/?id=616165109" };
                return ids[anim_type];
            }
            case 4: { // Levitation
                const char* ids[] = { "http://www.roblox.com/asset/?id=616006778", "http://www.roblox.com/asset/?id=616008087", "http://www.roblox.com/asset/?id=616010382", "http://www.roblox.com/asset/?id=616013216", "http://www.roblox.com/asset/?id=616008936", "http://www.roblox.com/asset/?id=616005863", "http://www.roblox.com/asset/?id=616003713", "http://www.roblox.com/asset/?id=616011509" };
                return ids[anim_type];
            }
            case 5: { // Stylish
                const char* ids[] = { "http://www.roblox.com/asset/?id=616136790", "http://www.roblox.com/asset/?id=616138447", "http://www.roblox.com/asset/?id=616140816", "http://www.roblox.com/asset/?id=616146177", "http://www.roblox.com/asset/?id=616139451", "http://www.roblox.com/asset/?id=616134815", "http://www.roblox.com/asset/?id=616133594", "http://www.roblox.com/asset/?id=616143378" };
                return ids[anim_type];
            }
            case 6: { // Cartoony
                const char* ids[] = { "http://www.roblox.com/asset/?id=742637544", "http://www.roblox.com/asset/?id=742638445", "http://www.roblox.com/asset/?id=742638842", "http://www.roblox.com/asset/?id=742640026", "http://www.roblox.com/asset/?id=742637942", "http://www.roblox.com/asset/?id=742637151", "http://www.roblox.com/asset/?id=742636889", "http://www.roblox.com/asset/?id=742639220" };
                return ids[anim_type];
            }
            case 7: { // Super Hero
                const char* ids[] = { "http://www.roblox.com/asset/?id=616111295", "http://www.roblox.com/asset/?id=616113536", "http://www.roblox.com/asset/?id=616117076", "http://www.roblox.com/asset/?id=616122287", "http://www.roblox.com/asset/?id=616115533", "http://www.roblox.com/asset/?id=616118211", "http://www.roblox.com/asset/?id=616120861", "http://www.roblox.com/asset/?id=616119360" };
                return ids[anim_type];
            }
            case 8: { // Elder
                const char* ids[] = { "http://www.roblox.com/asset/?id=845397899", "http://www.roblox.com/asset/?id=845400520", "http://www.roblox.com/asset/?id=845386501", "http://www.roblox.com/asset/?id=845403856", "http://www.roblox.com/asset/?id=845398858", "http://www.roblox.com/asset/?id=845396048", "http://www.roblox.com/asset/?id=845392038", "http://www.roblox.com/asset/?id=845401742" };
                return ids[anim_type];
            }
            case 9: { // Toy
                const char* ids[] = { "http://www.roblox.com/asset/?id=782841498", "http://www.roblox.com/asset/?id=782845736", "http://www.roblox.com/asset/?id=782842708", "http://www.roblox.com/asset/?id=782843345", "http://www.roblox.com/asset/?id=782847020", "http://www.roblox.com/asset/?id=782846423", "http://www.roblox.com/asset/?id=782843869", "http://www.roblox.com/asset/?id=782845186" };
                return ids[anim_type];
            }
            case 10: { // Old School
                const char* ids[] = { "http://www.roblox.com/asset/?id=5319828216", "http://www.roblox.com/asset/?id=5319831086", "http://www.roblox.com/asset/?id=5319844329", "http://www.roblox.com/asset/?id=5319847204", "http://www.roblox.com/asset/?id=5319841935", "http://www.roblox.com/asset/?id=5319839762", "http://www.roblox.com/asset/?id=5319816685", "http://www.roblox.com/asset/?id=5319850266" };
                return ids[anim_type];
            }
            default:
                return "";
            }
        };

        // Apply idle animations (both use idle_animation selection)
        std::string idle1_id = getAnimationID(globals::combat::idle_animation, 0);
        if (!idle1_id.empty()) idle1.SetAnimationID(idle1_id);
        std::string idle2_id = getAnimationID(globals::combat::idle_animation, 1);
        if (!idle2_id.empty()) idle2.SetAnimationID(idle2_id);
        // Apply run animation
        std::string run_id = getAnimationID(globals::combat::run_animation, 2);
        if (!run_id.empty()) run.SetAnimationID(run_id);
        // Apply walk animation
        std::string walk_id = getAnimationID(globals::combat::walk_animation, 3);
        if (!walk_id.empty()) walk.SetAnimationID(walk_id);
        // Apply jump animation
        std::string jump_id = getAnimationID(globals::combat::jump_animation, 4);
        if (!jump_id.empty()) jump.SetAnimationID(jump_id);
        // Apply fall animation
        std::string fall_id = getAnimationID(globals::combat::fall_animation, 5);
        if (!fall_id.empty()) fall.SetAnimationID(fall_id);
        // Apply climb animation
        std::string climb_id = getAnimationID(globals::combat::climb_animation, 6);
        if (!climb_id.empty()) climb.SetAnimationID(climb_id);
        // Apply swim animation
        std::string swim_id = getAnimationID(globals::combat::swim_animation, 7);
        if (!swim_id.empty()) swim.SetAnimationID(swim_id);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
