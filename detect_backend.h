#pragma once

enum class DisplayServer {
    Wayland,
    X11,
    Unknown
};

DisplayServer detect_display_server();
