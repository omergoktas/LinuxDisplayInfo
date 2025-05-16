#include "monitor_common.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <regex>

void get_native_resolution_from_drm(MonitorInfo& info) {
    const std::string drm_path = "/sys/class/drm";

    try {
        // Only process if the directory exists
        if (!std::filesystem::exists(drm_path)) {
            return;
        }

        // Iterate through all entries in the drm directory
        for (const auto& entry : std::filesystem::directory_iterator(drm_path)) {
            const std::string path = entry.path().string();
            const std::string name = entry.path().filename().string();

            // Skip entries that don't look like connectors (cardX-Y)
            if (name.find("card") == std::string::npos || name.find("-") == std::string::npos) {
                continue;
            }

            // Check if this connector is connected
            std::string status_path = path + "/status";
            if (std::filesystem::exists(status_path)) {
                std::ifstream status_file(status_path);
                std::string status;
                std::getline(status_file, status);
                if (status != "connected") {
                    continue;
                }
            }

            // Try to match this connector with our monitor name if possible
            if (!info.name.empty() && name.find(info.name) == std::string::npos &&
                info.name.find(name) == std::string::npos) {
                // Not a perfect match, but we'll continue anyway as a fallback
            }

            // Get modes from this connector
            std::string modes_path = path + "/modes";
            if (std::filesystem::exists(modes_path)) {
                std::ifstream modes_file(modes_path);
                std::string line;

                // Process each mode
                while (std::getline(modes_file, line)) {
                    // Parse the mode string (typically "1920x1080")
                    std::regex mode_regex("([0-9]+)x([0-9]+)");
                    std::smatch match;

                    if (std::regex_search(line, match, mode_regex) && match.size() > 2) {
                        int width = std::stoi(match[1]);
                        int height = std::stoi(match[2]);

                        // Keep track of the highest resolution as the native one
                        if (width * height > info.native_width * info.native_height) {
                            info.native_width = width;
                            info.native_height = height;
                        }
                    }
                }

                // If we found modes, no need to check other connectors
                if (info.native_width > 0 && info.native_height > 0) {
                    break;
                }
            }

            // As a fallback, try EDID parsing
            if (info.native_width == 0 || info.native_height == 0) {
                std::string edid_path = path + "/edid";
                if (std::filesystem::exists(edid_path)) {
                    // Read raw EDID data
                    std::ifstream edid_file(edid_path, std::ios::binary);
                    if (edid_file) {
                        // EDID block is 128 bytes
                        char edid[128];
                        edid_file.read(edid, 128);

                        if (edid_file.gcount() == 128) {
                            // EDID format: horizontal resolution is at byte 56-57 (combined)
                            // vertical resolution is at byte 59-60 (combined)
                            unsigned char *e = reinterpret_cast<unsigned char*>(edid);

                            // Check EDID header
                            if (e[0] == 0x00 && e[1] == 0xFF && e[2] == 0xFF && e[3] == 0xFF &&
                                e[4] == 0xFF && e[5] == 0xFF && e[6] == 0xFF && e[7] == 0x00) {

                                // Parse display parameters from detailed timing block
                                uint16_t h_active = ((e[56 + 4] >> 4) << 8) | e[54 + 2];
                                uint16_t v_active = ((e[56 + 7] >> 4) << 8) | e[54 + 5];

                                if (h_active > 0 && v_active > 0) {
                                    info.native_width = h_active;
                                    info.native_height = v_active;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Warning: Error accessing DRM subsystem: " << e.what() << std::endl;
    }
}

void print_monitor_info(const MonitorInfo& monitor, int index) {
    std::cout << "Monitor " << index << " (" << monitor.name << "):\n";

    if (monitor.width_px > 0 && monitor.height_px > 0) {
        std::cout << "  Current Resolution: " << monitor.width_px << "x" << monitor.height_px << " pixels";
        if (monitor.scale != 1.0f) {
            std::cout << " (scaled by " << std::fixed << std::setprecision(1) << monitor.scale << "x)";
        }
        std::cout << "\n";

        if (monitor.x_pos != 0 || monitor.y_pos != 0) {
            std::cout << "  Position: (" << monitor.x_pos << ", " << monitor.y_pos << ")\n";
        }
    } else {
        std::cout << "  Status: Connected but not active (no display mode assigned)\n";
    }

    if (monitor.native_width > 0 && monitor.native_height > 0 &&
        (monitor.native_width != monitor.width_px || monitor.native_height != monitor.height_px)) {
        std::cout << "  Native Resolution: " << monitor.native_width << "x" << monitor.native_height << " pixels\n";
    }

    if (monitor.width_mm > 0 && monitor.height_mm > 0) {
        std::cout << "  Physical size: " << monitor.width_mm << "x" << monitor.height_mm << " mm\n";

        // Calculate native DPI if we have native resolution
        if (monitor.native_width > 0 && monitor.native_height > 0) {
            float native_dpi_x = (monitor.native_width / (monitor.width_mm / 25.4f));
            float native_dpi_y = (monitor.native_height / (monitor.height_mm / 25.4f));
            std::cout << "  Native DPI: " << std::fixed << std::setprecision(1)
                      << native_dpi_x << "x" << native_dpi_y << "\n";
        }

        // Calculate current DPI if the monitor is active
        if (monitor.width_px > 0 && monitor.height_px > 0) {
            float dpi_x = (monitor.width_px / (monitor.width_mm / 25.4f));
            float dpi_y = (monitor.height_px / (monitor.height_mm / 25.4f));
            std::cout << "  Current DPI: " << std::fixed << std::setprecision(1)
                      << dpi_x << "x" << dpi_y << "\n";
        }
    }
}

