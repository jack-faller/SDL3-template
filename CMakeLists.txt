cmake_minimum_required(VERSION 3.26)
project(sdl3-test)

if(EMSCRIPTEN)
    add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

add_subdirectory(glad/)

add_executable(sdl3-test WIN32 sdl3-test.c)

if(EMSCRIPTEN)
	target_link_libraries(${EXECUTABLE_NAME} PRIVATE
		"--preload-file \"${CMAKE_CURRENT_LIST_DIR}/assets@/assets\"")
else()
	# make sure this always runs
	add_custom_target(assets DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dummy_output)
	add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dummy_output
			COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
					${CMAKE_SOURCE_DIR}/assets
					${CMAKE_CURRENT_BINARY_DIR}/assets)
    add_dependencies(sdl3-test assets)
endif()

target_link_libraries(sdl3-test PRIVATE SDL3::SDL3 glad)
