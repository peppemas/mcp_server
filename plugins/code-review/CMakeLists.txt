cmake_minimum_required(VERSION 3.10)

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")

    # This forces static linking of all standard libraries
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")

    # Add pthread explicitly
    link_libraries(pthread)
endif()

# Add this to your CMakeLists.txt
add_library(codereview SHARED
        ${PROJECT_SOURCE_DIR}/plugins/code-review/CodeReview.cpp
)

if(WIN32)
    target_link_libraries(codereview PRIVATE ws2_32)
endif()

target_link_libraries(codereview PRIVATE "-static -static-libgcc -static-libstdc++ -lpthread")

# Define WEATHER_EXPORTS when building the DLL
target_compile_definitions(codereview PRIVATE WEATHER_EXPORTS)
target_include_directories(codereview PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/interface)
