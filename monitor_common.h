#pragma once

#include <string>
#include <vector>

// Common monitor information structure to use across X11 and Wayland
struct MonitorInfo {
    std::string name = "Unknown Monitor";
    int width_px = 0;       // Current width in pixels
    int height_px = 0;      // Current height in pixels
    int native_width = 0;   // Native/max width in pixels
    int native_height = 0;  // Native/max height in pixels
    int width_mm = 0;       // Physical width in mm
    int height_mm = 0;      // Physical height in mm
    float scale = 1.0f;     // Scale factor
    int x_pos = 0;          // Position X
    int y_pos = 0;          // Position Y
};

// Get native resolution from DRM subsystem
void get_native_resolution_from_drm(MonitorInfo& info);

// Calculate and print monitor DPI information
void print_monitor_info(const MonitorInfo& monitor, int index);

