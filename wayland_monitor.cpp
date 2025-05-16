#include "wayland_monitor.h"
#include "monitor_common.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <wayland-client.h>

// Global vector to store monitor information
static std::vector<MonitorInfo> monitors;

// Wayland protocol handlers
static void output_geometry(void* data, struct wl_output* output,
                            int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height,
                            int32_t subpixel, const char* make,
                            const char* model, int32_t transform)
{
    MonitorInfo* info = static_cast<MonitorInfo*>(data);

    info->width_mm = physical_width;
    info->height_mm = physical_height;
    info->x_pos = x;
    info->y_pos = y;

    // Create monitor name from make and model
    std::string make_str = make ? make : "Unknown";
    std::string model_str = model ? model : "Display";
    info->name = make_str + " " + model_str;
}

static void output_mode(void* data, struct wl_output* output,
                        uint32_t flags, int32_t width, int32_t height,
                        int32_t refresh)
{
    MonitorInfo* info = static_cast<MonitorInfo*>(data);

    // Record the current resolution
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        info->width_px = width;
        info->height_px = height;
    }

    // Keep track of the highest resolution mode seen
    if (width * height > info->native_width * info->native_height) {
        info->native_width = width;
        info->native_height = height;
    }
}

static void output_done(void* data, struct wl_output* output) {
    // This is called when all output information has been sent
    // We'll use this opportunity to get more info from the system
    MonitorInfo* info = static_cast<MonitorInfo*>(data);

    // If we didn't get all modes from Wayland, try DRM subsystem
    get_native_resolution_from_drm(*info);
}

static void output_scale(void* data, struct wl_output* output, int32_t factor) {
    MonitorInfo* info = static_cast<MonitorInfo*>(data);
    info->scale = factor;
}

// Registry handlers
static void registry_global(void* data, struct wl_registry* registry,
                            uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, "wl_output") == 0) {
        // Add a new monitor to our list
        monitors.emplace_back();
        MonitorInfo* info = &monitors.back();

        // Use at most version 2 of the output interface for maximum compatibility
        uint32_t bind_version = version;
        if (bind_version > 2) bind_version = 2;

        struct wl_output* output = static_cast<struct wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, bind_version)
            );

        if (!output) {
            std::cerr << "Failed to bind output interface\n";
            monitors.pop_back();
            return;
        }

        // Set up output listener with all fields properly initialized
        static const struct wl_output_listener output_listener = {
            .geometry = output_geometry,
            .mode = output_mode,
            .done = output_done,
            .scale = output_scale,
            .name = nullptr,          // For newer Wayland protocol versions
            .description = nullptr     // For newer Wayland protocol versions
        };

        if (wl_output_add_listener(output, &output_listener, info) != 0) {
            std::cerr << "Failed to add output listener\n";
            wl_output_destroy(output);
            monitors.pop_back();
        }
    }
}

static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    // We don't need to handle output removal for this simple tool
}

void list_wayland_monitors() {
    // Clear any previous monitor data
    monitors.clear();

    // Connect to Wayland display
    struct wl_display* display = wl_display_connect(nullptr);
    if (!display) {
        std::cerr << "Error: Failed to connect to Wayland display\n";
        return;
    }

    // Get registry
    struct wl_registry* registry = wl_display_get_registry(display);
    if (!registry) {
        std::cerr << "Error: Failed to get Wayland registry\n";
        wl_display_disconnect(display);
        return;
    }

    // Set up registry listener
    static const struct wl_registry_listener registry_listener = {
        .global = registry_global,
        .global_remove = registry_global_remove
    };

    if (wl_registry_add_listener(registry, &registry_listener, nullptr) != 0) {
        std::cerr << "Error: Failed to add registry listener\n";
        wl_display_disconnect(display);
        return;
    }

    // Process events to discover outputs
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    // Process events again to get output properties
    wl_display_roundtrip(display);

    // Display results
    if (monitors.empty()) {
        std::cout << "No Wayland outputs detected.\n";
    } else {
        std::cout << "Found " << monitors.size() << " Wayland output(s):\n";

        for (size_t i = 0; i < monitors.size(); i++) {
            print_monitor_info(monitors[i], i);
        }
    }

    // Clean up
    wl_display_disconnect(display);
}
