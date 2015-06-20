# Compatibility
cmake_minimum_required(VERSION 2.6)

# Project name
project(libspawner)

# Project version
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 2)
set(${PROJECT_NAME}_PATCH_LEVEL 0)

include_directories("${PROJECT_SOURCE_DIR}/inc")
include_directories("${PROJECT_SOURCE_DIR}/inc/md5")
include_directories("${PROJECT_SOURCE_DIR}")

set(CMAKE_BUILD_TYPE ${SP_BUILD_TYPE})

set(SOURCES
    src/md5/md5.c
    src/options.cpp
    src/restrictions.cpp
)

set(HEADERS
    sp.h
    inc/md5/md5.h
    inc/buffer.h
    inc/delegate.h
    inc/options.h
    inc/platform.h
    inc/restrictions.h
    inc/securerunner.h
    inc/stack_walker.h
    inc/uconvert.h
    inc/compatibility.h
    inc/error.h
    inc/mutex.h
    inc/pipes.h
    inc/report.h
    inc/runner.h
    inc/session.h
    inc/status.h
)

set(LIB_WIN32_SOURCES
    src/win32/buffer.cpp
    src/win32/compatibility.cpp
    src/win32/delegate.cpp
    src/win32/error.cpp
    src/win32/pipes.cpp
    src/win32/platform.cpp
    src/win32/report.cpp
    src/win32/runner.cpp
    src/win32/securerunner.cpp
    src/win32/session.cpp
    src/win32/uconvert.cpp
)

if(UNIX)
    set(SOURCES ${SOURCES} ${LIB_NIX_SOURCES})
else()
    if(WIN32)
        set(SOURCES ${SOURCES} ${LIB_WIN32_SOURCES})
    endif()
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(SOURCES ${SOURCES} src/stack_walker.cpp)
    add_definitions(-DWANT_STACKWALKER)
endif()

set(PROJECT_LIBRARY ${PROJECT_NAME})

add_library(${PROJECT_LIBRARY} ${SOURCES} ${HEADERS})
