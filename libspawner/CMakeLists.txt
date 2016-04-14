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
    src/multibyte.cpp
    src/session.cpp
    src/uconvert.cpp
    src/compatibility.cpp
    src/error.cpp
    src/report.cpp
)

set(HEADERS
    sp.h
    inc/md5/md5.h
    inc/delegate.h
    inc/options.h
    inc/restrictions.h
    inc/securerunner.h
    inc/uconvert.h
    inc/compatibility.h
    inc/error.h
    inc/report.h
    inc/runner.h
    inc/session.h
    inc/status.h
)

set(LIB_WIN32_HEADERS
    inc/buffer.h
    inc/pipes.h
    inc/mutex.h
    inc/win32/platform.h
    inc/win32/exceptions.h
    inc/win32/stack_walker.h
    inc/win32/platform_report.h
)

set(LIB_POSIX_HEADERS
    inc/posix/platform.h
    inc/posix/signals.h
    inc/posix/rlimit.h
    inc/posix/platform_report.h
)

set(LIB_LINUX_HEADERS
    inc/posix/linux_affinity.h
    inc/posix/linux_procfs.h
    inc/posix/linux_seccomp.h
)

set(LIB_WIN32_SOURCES
    src/win32/buffer.cpp
    src/win32/delegate.cpp
    src/win32/pipes.cpp
    src/win32/platform.cpp
    src/win32/platform_report.cpp
    src/win32/runner.cpp
    src/win32/securerunner.cpp
)
set(LIB_UNIX_SOURCES
    src/posix/delegate.cpp
    src/posix/platform.cpp
    src/posix/runner.cpp
    src/posix/securerunner.cpp
    src/posix/rlimit.cpp
)

set(LIB_LINUX_SOURCES
    src/posix/linux_affinity.cpp
    src/posix/linux_seccomp.cpp
    src/posix/linux_procfs.cpp
)

if(UNIX)
    set(SOURCES ${SOURCES} ${LIB_UNIX_SOURCES})
    set(HEADERS ${HEADERS} ${LIB_UNIX_HEADERS})
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SOURCES ${SOURCES} ${LIB_LINUX_SOURCES})
        set(HEADERS ${HEADERS} ${LIB_LINUX_HEADERS})
    endif()
    include_directories("${PROJECT_SOURCE_DIR}/inc/posix")
elseif(WIN32)
    set(SOURCES ${SOURCES} ${LIB_WIN32_SOURCES})
    set(HEADERS ${HEADERS} ${LIB_WIN32_HEADERS})
    include_directories("${PROJECT_SOURCE_DIR}/inc/win32")
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
        set(SOURCES ${SOURCES} src/win32/stack_walker.cpp)
        add_definitions(-DWANT_STACKWALKER)
    endif()
endif()

set(PROJECT_LIBRARY ${PROJECT_NAME})

add_library(${PROJECT_LIBRARY} ${SOURCES} ${HEADERS})
