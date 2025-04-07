cmake_minimum_required(VERSION 3.10)

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")

    # This forces static linking of all standard libraries
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")

    # Add pthread explicitly
    link_libraries(pthread)
endif()

# Add this to your CMakeLists.txt
add_library(codereview SHARED
        ${PROJECT_SOURCE_DIR}/plugins/code-review/CodeReview.cpp
)

# For Linux/Unix, ensure we're building with PIC
if(UNIX)
    set_target_properties(codereview PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()


if(WIN32)
    target_link_libraries(codereview PRIVATE ws2_32)
endif()

# Avoid forced static linking on non-Windows (Unix/Linux)
if(WIN32)
    target_link_libraries(codereview PRIVATE "-static -static-libgcc -static-libstdc++ -lpthread")
else()
    find_package(Threads REQUIRED)
    target_link_libraries(codereview PRIVATE Threads::Threads)
endif()

target_compile_definitions(codereview PRIVATE PLUGIN_API)
target_include_directories(codereview PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/interface)
