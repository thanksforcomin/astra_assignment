cmake_minimum_required(VERSION 3.16)
project(dirview_assignment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(dirview_assignment main.cpp fsmodel.cpp)

target_link_libraries(dirview_assignment Qt5::Core Qt5::Widgets)
