#include <iostream>
#include "detect_backend.h"
#include "wayland_monitor.h"
#include "x11_monitor.h"

int main() {
    DisplayServer backend = detect_display_server();

    std::cout << "Display Server Detection: ";

    switch (backend) {
    case DisplayServer::Wayland:
        std::cout << "Wayland\n";
        std::cout << "------------------------------\n";
        list_wayland_monitors();
        break;

    case DisplayServer::X11:
        std::cout << "X11\n";
        std::cout << "------------------------------\n";
        list_x11_monitors();
        break;

    default:
        std::cout << "Unknown\n";
        std::cerr << "Error: Unable to determine display server type\n";
        return 1;
    }

    return 0;
}
