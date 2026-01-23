#include "Protection.hpp"
#include "../globals/globals.hpp"
#include "../misc/output_system/output/output.hpp"
void RBX::EXE::Engine::Protection::EnterSafeMode() {
   
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus) && powerStatus.ACLineStatus == 1) {
      
        GUID powerGuid = GUID_MAX_POWER_SAVINGS;
        PowerSetActiveScheme(0, &powerGuid);
    }

  
    SystemParametersInfo(SPI_SETUIEFFECTS, FALSE, NULL, SPIF_UPDATEINIFILE);

   
    std::vector<std::thread> backgroundTasks;

    for (int i = 0; i < 5; ++i) {
        backgroundTasks.push_back(std::thread([i]() {
            // Simulating a task
            std::this_thread::sleep_for(std::chrono::seconds(10));
         //   std::cout << "Background Task " << i << " completed.\n";
            }));
    }

   
    for (auto& task : backgroundTasks) {
        task.detach();
    }

   
    DWORD_PTR affinityMask = 3; 
    if (!SetProcessAffinityMask(GetCurrentProcess(), affinityMask)) {
        std::cerr << "Failed to set CPU affinity!" << std::endl;
    }

   
    SystemParametersInfo(SPI_SETANIMATION, FALSE, NULL, SPIF_UPDATEINIFILE);

    
  
    if (globals::debug_info)
    utils::output::System("Entering Safe Mode: Optimizing performance...");
}