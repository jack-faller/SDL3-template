cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

project(sdl3-template)

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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.[ch]pp)
add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})

if(EMSCRIPTEN)
	target_link_libraries(${EXECUTABLE_NAME} PRIVATE
		"--preload-file \"${CMAKE_CURRENT_LIST_DIR}/assets@/assets\""
		"--shell-file \"${CMAKE_CURRENT_LIST_DIR}/src/shell.html\""
		"-sMIN_WEBGL_VERSION=2")
	file(GLOB_RECURSE ASSET_SOURCES CONFIGURE_DEPENDS assets/*)
	# This is not working when changes are made to "shell.html".
	set_property(TARGET ${EXECUTABLE_NAME} APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/shell.html ${ASSET_SOURCES})
else()
	# This doesn't work in release builds.
	set(AssetTargetPath ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/assets)
	add_custom_target(assets DEPENDS ${AssetTargetPath})
	add_custom_command(
			OUTPUT ${AssetTargetPath}
			COMMAND ${CMAKE_COMMAND} -E create_symlink
					${CMAKE_SOURCE_DIR}/assets
					${AssetTargetPath})
	add_dependencies(${EXECUTABLE_NAME} assets)
endif()

if(MSVC)
  target_compile_options(${EXECUTABLE_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(${EXECUTABLE_NAME} PUBLIC SDL3::SDL3 glad)
