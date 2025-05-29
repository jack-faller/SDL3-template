cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

project(sdl3-test)

set(EXECUTABLE_NAME ${PROJECT_NAME})

if(EMSCRIPTEN)
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(SDL_SHARED OFF)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

    add_subdirectory(libraries/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

add_subdirectory(libraries/glad/)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS sources/*)
add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})

if(EMSCRIPTEN)
	target_link_libraries(${EXECUTABLE_NAME} PRIVATE
		"--preload-file \"${CMAKE_CURRENT_LIST_DIR}/assets@/assets\""
		"-sMIN_WEBGL_VERSION=2")
	file(GLOB_RECURSE ASSET_SOURCES CONFIGURE_DEPENDS assets/*)
	set_property(TARGET ${EXECUTABLE_NAME} APPEND PROPERTY LINK_DEPENDS ${ASSET_SOURCES})
else()
	add_custom_target(assets DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/assets)
	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets
			COMMAND ${CMAKE_COMMAND} -E create_symlink
					${CMAKE_SOURCE_DIR}/assets
					${CMAKE_CURRENT_BINARY_DIR}/assets)
	add_dependencies(${EXECUTABLE_NAME} assets)
endif()

target_link_libraries(${EXECUTABLE_NAME} PUBLIC SDL3::SDL3 glad)
