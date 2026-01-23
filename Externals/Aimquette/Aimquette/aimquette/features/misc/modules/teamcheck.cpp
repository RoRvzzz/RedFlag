#include "../../../util/globals.h"
#include "../../../util/classes/classes.h"
#include "../../../util/driver/driver.h"

namespace teamcheck {
    void run() {
        // Team check functionality is handled by the global is_teammate function
        // This module can be used for additional team checking logic if needed
        
        // Example: Additional team validation or logging
        if (globals::combat::teamcheck || globals::visuals::teamcheck) {
            // Team checking is enabled
            // The actual team checking logic is in globals.h::is_teammate()
        }
    }
}



