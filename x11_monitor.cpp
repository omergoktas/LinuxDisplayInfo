#include "x11_monitor.h"
#include "monitor_common.h"
#include <iostream>
#include <vector>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <cstdint>

void list_x11_monitors() {
    // Connect to X server
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Error: Failed to open X display\n";
        return;
    }

    // Check if XRandR extension is available
    int event_base, error_base;
    if (!XRRQueryExtension(display, &event_base, &error_base)) {
        std::cerr << "Error: XRandR extension not available\n";
        XCloseDisplay(display);
        return;
    }

    // Get default screen and root window
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Get XRandR screen resources
    XRRScreenResources* screen_res = XRRGetScreenResources(display, root);
    if (!screen_res) {
        std::cerr << "Error: Failed to get screen resources\n";
        XCloseDisplay(display);
        return;
    }

    // Vector to store all monitor information
    std::vector<MonitorInfo> monitors;

    // For each output
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo* output_info = XRRGetOutputInfo(display, screen_res, screen_res->outputs[i]);
        if (!output_info) {
            continue;
        }

        // Skip disconnected outputs
        if (output_info->connection != RR_Connected) {
            XRRFreeOutputInfo(output_info);
            continue;
        }

        // Create monitor info
        MonitorInfo info;
        info.name = output_info->name;
        info.width_mm = output_info->mm_width;
        info.height_mm = output_info->mm_height;

        // Get current mode information if output is active
        if (output_info->crtc) {
            XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(display, screen_res, output_info->crtc);
            if (crtc_info) {
                info.width_px = crtc_info->width;
                info.height_px = crtc_info->height;
                info.x_pos = crtc_info->x;
                info.y_pos = crtc_info->y;

                // Check for any scaling
                if (info.width_mm > 0 && info.height_mm > 0) {
                    float physical_aspect = (float)info.width_mm / info.height_mm;
                    float pixel_aspect = (float)info.width_px / info.height_px;

                    // If aspects differ significantly, scaling might be in effect
                    if (std::abs(physical_aspect - pixel_aspect) > 0.1f) {
                        info.scale = pixel_aspect / physical_aspect;
                    }
                }

                XRRFreeCrtcInfo(crtc_info);
            }
        }

        // Find highest resolution mode available for this output
        for (int j = 0; j < output_info->nmode; j++) {
            RRMode mode_id = output_info->modes[j];

            // Find the mode in screen resources
            for (int k = 0; k < screen_res->nmode; k++) {
                // Fix signedness comparison by using the same type
                if (mode_id == screen_res->modes[k].id) {
                    XRRModeInfo& mode = screen_res->modes[k];

                    // Fix signedness comparison with explicit casts to uint64_t
                    uint64_t mode_pixels = static_cast<uint64_t>(mode.width) * static_cast<uint64_t>(mode.height);
                    uint64_t native_pixels = static_cast<uint64_t>(info.native_width) * static_cast<uint64_t>(info.native_height);

                    // Check if higher than current native resolution
                    if (mode_pixels > native_pixels) {
                        info.native_width = mode.width;
                        info.native_height = mode.height;
                    }
                    break;
                }
            }
        }

        // If we couldn't find native resolution through XRandR, try the DRM subsystem
        if (info.native_width == 0 || info.native_height == 0) {
            get_native_resolution_from_drm(info);
        }

        // Fix parentheses warning by properly grouping the condition
        if (info.native_width == 0 || (info.native_height == 0 && info.width_px > 0)) {
            info.native_width = info.width_px;
            info.native_height = info.height_px;
        }

        // Add to our list
        monitors.push_back(info);

        XRRFreeOutputInfo(output_info);
    }

    // Display results
    if (monitors.empty()) {
        std::cout << "No X11 outputs detected.\n";
    } else {
        std::cout << "Found " << monitors.size() << " X11 output(s):\n";

        for (size_t i = 0; i < monitors.size(); i++) {
            print_monitor_info(monitors[i], i);
        }
    }

    // Clean up
    XRRFreeScreenResources(screen_res);
    XCloseDisplay(display);
}
