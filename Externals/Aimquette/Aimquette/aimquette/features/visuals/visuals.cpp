#include "visuals.h"
#include "modules/crosshair.h"


namespace visuals {
    void renderCrosshair() {
        // Call the crosshair module render function
        crosshair::render();
    }
}
