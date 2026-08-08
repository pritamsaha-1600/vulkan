cmake_minimum_required(VERSION 4.3)
project(vulkan)

find_package(X11 REQUIRED)
set(CMAKE_CXX_STANDARD 23)

add_executable(vulkan main.cpp
        Application.cpp
        Application.h
)
target_link_libraries(vulkan PRIVATE X11::X11)