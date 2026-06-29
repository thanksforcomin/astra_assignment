cmake_minimum_required(VERSION 3.16)
project(dirview_assignment)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(dirview_assignment main.cpp)

target_link_libraries(dirview_assignment Qt5::Core Qt5::Widgets)
