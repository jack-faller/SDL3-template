cmake_minimum_required(VERSION 3.10)
project(sdl3-test)

option(SDL3-TEST_VENDORED "Use vendored libraries" OFF)

if(SDL3-TEST_VENDORED)
    add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

add_subdirectory(glad/)

add_executable(sdl3-test WIN32 sdl3-test.c)

target_link_libraries(sdl3-test PRIVATE SDL3::SDL3 glad)
