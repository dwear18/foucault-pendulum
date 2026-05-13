cmake_minimum_required(VERSION 3.10)

project(FoucaultPendulum)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(FoucaultPendulum
    src/main.cpp
    src/pendulum.cpp
    src/physics_engine.cpp
    src/runge_kutta.cpp
    src/simulation.cpp
    src/renderer.cpp
    src/controller.cpp
)

target_link_libraries(FoucaultPendulum
    SFML::Graphics
    SFML::Window
    SFML::System
)