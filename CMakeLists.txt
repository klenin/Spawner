# Compatibility
cmake_minimum_required(VERSION 2.6)

if(WIN32)
    # Windows XP compatible platform toolset. Must be set before project(),
    # otherwise change of CMAKE_*_TOOLSET will take no effect.
    # We get VS version from the generator name because neither MSVC* nor other
    # variables that describe the compiler aren't available before project().
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio ([0-9]+)")
        # Note: "v110_xp" is for Visual Studio 11 2012, which is unsupported.
        if (${CMAKE_MATCH_1} EQUAL 12)
            # Visual Studio 12 2013
            set(CMAKE_GENERATOR_TOOLSET "v120_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
            set(CMAKE_VS_PLATFORM_TOOLSET "v120_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
        elseif (${CMAKE_MATCH_1} EQUAL 14)
            # Visual Studio 14 2015
            set(CMAKE_GENERATOR_TOOLSET "v140_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
            set(CMAKE_VS_PLATFORM_TOOLSET "v140_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
        elseif (${CMAKE_MATCH_1} EQUAL 15)
            # Visual Studio 15 2017
            set(CMAKE_GENERATOR_TOOLSET "v141_xp" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
            set(CMAKE_VS_PLATFORM_TOOLSET "v141_xp" CACHE STRING "CMAKE_VS_PLATFORM_TOOLSET" FORCE)
        else()
            message(FATAL_ERROR "Supported Visual Studio: 12 (v120_xp), 14 (v140_xp), 15 (v141_xp)")
        endif()
    endif()
endif()

# Project name
project(sp)

# Project version
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 1)
set(${PROJECT_NAME}_PATCH_LEVEL 0)

set(SPAWNER_PATH "${CMAKE_BINARY_DIR}/sp")
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(ROOT_DIR "${PROJECT_SOURCE_DIR}")
set(PROTOTYPE_TESTER "${ROOT_DIR}/bin/sp.exe")

add_subdirectory("${PROJECT_SOURCE_DIR}/libspawner" libspawner)

include_directories("${PROJECT_SOURCE_DIR}/inc")
include_directories("${PROJECT_SOURCE_DIR}/libspawner")
include_directories("${PROJECT_SOURCE_DIR}/lib/rapidjson/include")

if(UNIX)
    include_directories("${PROJECT_SOURCE_DIR}/libspawner/inc/posix")
elseif(WIN32)
    include_directories("${PROJECT_SOURCE_DIR}/libspawner/inc/win32")
endif()

# Always include symbols
if(DEBUG)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(SP_BUILD_TYPE ${CMAKE_BUILD_TYPE})

set(HEADERS
    inc/arguments.h
    inc/spawner_base.h
    inc/spawner_new.h
    inc/spawner_old.h
    inc/spawner_pcms2.h
)

set(SOURCES
    src/main.cpp
    src/arguments.cpp
    src/spawner_base.cpp
    src/spawner_new.cpp
    src/spawner_old.cpp
    src/spawner_pcms2.cpp
)

set(LIBRARIES "libspawner")

if(WIN32)
    set(LIBRARIES ${LIBRARIES} "userenv" "imagehlp")
    add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=WINVER)
endif()

if(UNIX)
    # Linux requires -lrt for shm_{open,...}
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        set(LIBRARIES ${LIBRARIES} "rt")
    endif()
    set(LIBRARIES ${LIBRARIES} "pthread")
endif()

set(PROJECT_EXECUTABLE ${PROJECT_NAME})
set(BIN_PATH "../bin/new")
set(BIN_SPECIFIC_PATH "../bin/new/${CMAKE_GENERATOR}")

add_executable(${PROJECT_EXECUTABLE} ${SOURCES} ${HEADERS})
if (BIT32)
    set_target_properties(${PROJECT_EXECUTABLE} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${BIN_PATH}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_SPECIFIC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${BIN_SPECIFIC_PATH}
)

target_link_libraries(${PROJECT_EXECUTABLE} ${LIBRARIES})

set_property(TARGET sp PROPERTY CXX_STANDARD 11)
set_property(TARGET sp PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET libspawner PROPERTY CXX_STANDARD 11)
set_property(TARGET libspawner PROPERTY CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
    if(NOT DEBUG)
        set(CMAKE_EXE_LINKER_FLAGS "-s ${CMAKE_EXE_LINKER_FLAGS}")
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "--stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()

if(MSVC)
    # MultiThreaded static
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    # Don't use min/max macros from Windef.h (MinGW default behavior)
    add_definitions(-DNOMINMAX)
endif()
