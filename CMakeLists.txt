project(tankgame)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS On)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT DEVELOPMENT)
    set(DEVELOPMENT 1)
endif()

set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extlib/glm/cmake/glm)

find_package(FMT REQUIRED)
find_package(EnTT REQUIRED)
find_package(glm REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL)

file(GLOB_RECURSE common_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fasel/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fasel/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fasel/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fasel/*.cpp
    )

file(GLOB_RECURSE server_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/server/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cpp
    )

file(GLOB_RECURSE client_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/client/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/client/*.cpp
    )


set(tg_windows_disabled_warnings
    /wd4267
    /wd4101
    /wd4244
)

######## SERVER #########
add_executable(tankgame-sv ${server_sources} ${common_sources})
target_include_directories(tankgame-sv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tankgame-sv PRIVATE
    Threads::Threads
    fmt::fmt
    EnTT::EnTT
    glm::glm
    )

target_compile_definitions(tankgame-sv PRIVATE
    SERVER=1
    DEVELOPMENT=${DEVELOPMENT}
    NOGDI=1
    )
target_precompile_headers(tankgame-sv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common/common.hpp)

if(WIN32)
    target_compile_definitions(tankgame-sv PRIVATE
        WINDOWS=1
        _USE_MATH_DEFINES=1
        NOMINMAX=1
        _WINSOCK_DEPRECATED_NO_WARNINGS=1
        _CRT_SECURE_NO_WARNINGS=1
        )
    target_link_libraries(tankgame-sv PRIVATE ws2_32)
    #target_link_options(tankgame-sv PRIVATE -fsanitize=address /PROFILE)
    if(MSVC)
        target_compile_options(tankgame-sv PRIVATE
            /MP
            ${tg_windows_disabled_warnings}
            )
    endif()
else()
    target_compile_definitions(tankgame-sv PRIVATE LINUX=1)
endif()

target_compile_features(tankgame-sv PRIVATE cxx_std_20)



######## CLIENT #########

add_executable(tankgame-cl ${client_sources} ${common_sources})

if(WIN32)
    find_library(SDL2_LIBRARY SDL2.lib                 PATHS ${CMAKE_CURRENT_SOURCE_DIR}/extlib)
    find_library(SDL2_MIXER_LIBRARIES SDL2_mixer.lib   PATHS ${CMAKE_CURRENT_SOURCE_DIR}/extlib)
    find_library(FREETYPE_LIBRARIES Freetype.lib       PATHS ${CMAKE_CURRENT_SOURCE_DIR}/extlib)
    #find_library(SDL2_TTF_LIBRARIES SDL2_ttf.lib     PATHS ${CMAKE_CURRENT_SOURCE_DIR}/extlib)
    set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_compile_definitions(tankgame-cl PRIVATE
        _USE_MATH_DEFINES=1
        # TODO: This is a hack to disable SDL_Main, I dont't even know what problem SDL_Main is supposed to solve but it's just annoying.
        SDL_main_h_
        )

    target_link_libraries(tankgame-cl PRIVATE ws2_32)
else()
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(Freetype REQUIRED)

     target_link_libraries(tankgame-cl PRIVATE ${CMAKE_DL_LIBS})
endif()

target_include_directories(tankgame-cl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    )

target_link_libraries(tankgame-cl PRIVATE
    OpenGL::GL
    fmt::fmt
    EnTT::EnTT
    glm::glm
    ${SDL2_LIBRARY}
    ${SDL2_MIXER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    #${SDL2_TTF_LIBRARIES}
    )

target_compile_definitions(tankgame-cl PRIVATE
    CLIENT=1
    DEVELOPMENT=${DEVELOPMENT}
    NOGDI=1
    )

target_precompile_headers(tankgame-cl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common/common.hpp)

if(WIN32)
    target_compile_definitions(tankgame-cl PRIVATE
        WINDOWS=1
        _USE_MATH_DEFINES=1
        NOMINMAX=1
        _WINSOCK_DEPRECATED_NO_WARNINGS=1
        _CRT_SECURE_NO_WARNINGS=1
        )
    if(MSVC)
        target_compile_options(tankgame-cl PRIVATE
            /MP
            ${tg_windows_disabled_warnings}
            )
        target_link_libraries(tankgame-cl PRIVATE legacy_stdio_definitions.lib)
    endif()
else()
    target_compile_definitions(tankgame-cl PRIVATE LINUX=1)
endif()

target_compile_features(tankgame-cl PRIVATE cxx_std_20)

if(WIN32)
        #target_link_options(tankgame-cl PRIVATE -fsanitize=address /PROFILE)
endif()



# TODO
#add_subdirectory(genious)
