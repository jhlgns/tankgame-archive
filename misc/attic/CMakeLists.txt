project(tankgame)
cmake_minimum_required(VERSION 3.17)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()


set(shared_sources
	game.cpp
	game_sim.cpp
	socket.cpp
	log.cpp
	)


### LIBS ###
find_package(Threads REQUIRED)


### SERVER ###
add_executable(server
	server/server.cpp
	server/main.cpp
	server/session.cpp
	${shared_sources}
	)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) 
target_link_libraries(server PRIVATE Threads::Threads)
target_compile_definitions(server PRIVATE SERVER=1)
target_compile_features(server PRIVATE cxx_std_17)


### TEST CLIENT ###
#add_executable(test-client
#	server/server.cpp
#	server/main.cpp
#	server/session.cpp
#	${shared_sources}
#	)
#target_include_directories(test-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#target_link_libraries(test-client PRIVATE Threads::Threads)


### CLIENT ###

add_executable(client
	game_render.cpp
	client/client.cpp
	client/main.cpp
	client/states/connecting.cpp
	client/states/handshake.cpp
	client/states/menu.cpp
	client/states/sessionlist.cpp
	${shared_sources}
	)

if(REAL_CLIENT)
	set(CMAKE_MODULE_PATH /usr/local/share/cmake-3.17/Modules)
	message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
	find_package(SDL2      REQUIRED)
	find_package(SDL_image MODULE REQUIRED)
	find_package(SDL_mixer MODULE REQUIRED)
	target_include_directories(client PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL_IMAGE_INCLUDE_DIRS}
		${SDL_MIXER_INCLUDE_DIRS}
		)
	target_link_libraries(client PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL_IMAGE_LIBRARIES}
		${SDL_MIXER_LIBRARIES}
		)
	target_link_libraries(client PRIVATE SDL2::Main SDL2::Image SDL2::SDL2)
else()
	set(client_wasm_use_options
		"SHELL:-s USE_SDL=2"
		"SHELL:-s USE_SDL_IMAGE=2"
		"SHELL:-s USE_SDL_MIXER=2"
		)

	set(client_wasm_link_options
		${client_wasm_use_options}
		"SHELL:--preload-file ../assets"
		"SHELL:-s TOTAL_MEMORY=33554432"  # 32Mb
		"SHELL:-s SDL2_IMAGE_FORMATS=['png']"
		--shared-memory
		)
	target_compile_definitions(client PRIVATE WASM_CLIENT)
endif()

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(client PRIVATE CLIENT=1)
target_compile_features(client PRIVATE cxx_std_17)
target_compile_options(client PRIVATE
	${client_wasm_use_options}
	"SHELL:-O0"
	-Wno-writable-strings
	)
target_link_options(client PRIVATE ${client_wasm_link_options})

if(NOT REAL_CLIENT)
	set_target_properties(client PROPERTIES SUFFIX ".js")
endif()
