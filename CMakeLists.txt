cmake_minimum_required(VERSION 3.30)
project(mcp_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the project version
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 5)
set(PROJECT_VERSION_PATCH 0)

# Create a version string
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Create a version.h.in template file
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/server/Server.cpp
    src/transport/StdioTransport.cpp
    src/loader/PluginsLoader.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
        include
        src/transport
        src/interface
        libs_tier_01/aixlog-1.5.0/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Link platform-specific libraries
if(WIN32)
    # Windows Sockets library is required on Windows
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
else()
    # On POSIX systems (Linux, macOS), standard socket functions are
    # typically part of the standard C library (libc) and usually
    # don't require explicit linking.
    # If you encounter specific linker errors on older Linux systems related
    # to networking functions (like gethostbyname), you might need:
    # target_link_libraries(coversight_server PRIVATE socket nsl)
    # However, try without them first, as they are often unnecessary now.
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(WIN32)
        target_link_options(${PROJECT_NAME} PRIVATE "-static-libgcc" "-static-libstdc++" "-static")
    elseif(UNIX AND NOT APPLE) # e.g., Linux
        target_link_options(${PROJECT_NAME} PRIVATE "-static-libgcc" "-static-libstdc++")
    endif()
endif()

# Example Plugins
add_subdirectory(plugins/weather)
add_subdirectory(plugins/sleep)
add_subdirectory(plugins/code-review)
add_subdirectory(plugins/bacio-quote)
