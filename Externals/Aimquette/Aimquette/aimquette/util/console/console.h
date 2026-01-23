#pragma once

namespace tray {
    // Initialize system tray icon and menu in a background thread
    void initialize();
    // Remove tray icon and stop background thread
    void shutdown();
}