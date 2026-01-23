#include "../../../misc/output_system/output/output.hpp"
#include "../../../misc/output_system/log_system/log_system.h"
#include "../../../misc/Umodule/Umodule.hpp"
#include "../../../globals/globals.hpp"
#include <windows.h> 
#include <TlHelp32.h> 
#include <unordered_map>
#include <vector>

void plrdata() {

    utils::output::info("[PlrData] Starting player data analysis...");

    std::vector<int> playersVal;
    size_t ChildCount = globals::players.GetChildren().size();
    utils::output::printint("[PlrData] Player service child count:", static_cast<int>(ChildCount));

    playersVal.reserve(ChildCount);
    utils::output::info("[PlrData] Reserved space in playersVal vector.");

    for (int i = 0; i < 30; i++) {
        int Result = globals::players.fetchPlayer(globals::players.address);
        playersVal.push_back(Result);
        utils::output::printint("[PlrData] Fetched Player Value:", Result);
    }

    if (playersVal.empty()) {
        utils::output::error("[PlrData] Player values vector is empty. Cannot proceed.");
        return;
    }

    utils::output::printint("[PlrData] Total fetched player values:", static_cast<int>(playersVal.size()));

    std::unordered_map<int, int> countMap;
    int maxCount = 0;
    globals::mostFreq = 0;

    utils::output::info("[PlrData] Analyzing frequency of fetched player values...");

    for (int num : playersVal) {
        countMap[num]++;
        utils::output::printint("[PlrData] Incremented count for value:", num);

        if (countMap[num] > maxCount) {
            maxCount = countMap[num];
            globals::mostFreq = num;

            utils::output::printint("[PlrData] New Most Frequent Value:", globals::mostFreq);
            utils::output::printint("[PlrData] With Count:", maxCount);
        }
    }

    utils::output::info("[PlrData] Frequency analysis complete.");
    utils::output::printint("[PlrData] Final Most Frequent Player Value:", globals::mostFreq);
}
