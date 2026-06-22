cmake_minimum_required(VERSION 3.20)

project(asek VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(asek
    src/main.cpp
)

target_link_libraries(asek PRIVATE 
    imgui::imgui
    glfw
	opengl32
)

target_include_directories(asek PRIVATE include)