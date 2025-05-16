#include "detect_backend.h"
#include <cstdlib>

DisplayServer detect_display_server() {
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    const char* x11 = std::getenv("DISPLAY");

    if (wayland) return DisplayServer::Wayland;
    if (x11) return DisplayServer::X11;
    return DisplayServer::Unknown;
}
