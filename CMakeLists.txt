cmake_minimum_required(VERSION 3.21)

project(Orbit2D LANGUAGES CXX)

add_executable(orbit2d WIN32
    src/main.cpp
    src/engine/Application.cpp
    src/engine/Renderer.cpp
    src/engine/Scene.cpp
)

target_compile_features(orbit2d PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(orbit2d PRIVATE /W4 /permissive-)
else()
    target_compile_options(orbit2d PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(orbit2d PRIVATE user32 gdi32)

set_target_properties(orbit2d PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)

add_executable(arena_clash WIN32
    src/fighting_game.cpp
    src/engine/Application.cpp
    src/engine/Renderer.cpp
)

target_compile_features(arena_clash PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(arena_clash PRIVATE /W4 /permissive-)
else()
    target_compile_options(arena_clash PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(arena_clash PRIVATE user32 gdi32)

set_target_properties(arena_clash PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)
