cmake_minimum_required(VERSION 3.10)
project(LinuxDisplayInfo VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable compiler warnings but don't treat them as errors
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Source files
set(SOURCES
    main.cpp
    detect_backend.cpp
    wayland_monitor.cpp
    x11_monitor.cpp
    monitor_common.cpp
)

set(HEADERS
    detect_backend.h
    wayland_monitor.h
    x11_monitor.h
    monitor_common.h
)

# Add executable
add_executable(LinuxDisplayInfo ${SOURCES} ${HEADERS})

# Package dependencies
find_package(PkgConfig REQUIRED)

# Find X11 and XRandR
find_package(X11 REQUIRED)
if(NOT X11_FOUND)
    message(FATAL_ERROR "X11 libraries not found")
endif()

if(NOT X11_Xrandr_FOUND)
    message(FATAL_ERROR "XRandR library not found")
endif()

# Find Wayland
pkg_check_modules(WAYLAND wayland-client REQUIRED)
if(NOT WAYLAND_FOUND)
    message(FATAL_ERROR "Wayland client library not found")
endif()

# Include directories
target_include_directories(LinuxDisplayInfo PRIVATE
    ${X11_INCLUDE_DIR}
    ${WAYLAND_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(LinuxDisplayInfo PRIVATE
    ${X11_LIBRARIES}
    ${X11_Xrandr_LIB}
    ${WAYLAND_LIBRARIES}
)

# For older GCC that needs explicit linking to filesystem
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(LinuxDisplayInfo PRIVATE stdc++fs)
endif()

# Install the executable
install(TARGETS LinuxDisplayInfo DESTINATION bin)
