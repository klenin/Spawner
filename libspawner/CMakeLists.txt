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

set(HEADERS
    # Single headers
    sp.h
    inc/status.h

    # General headers
    inc/md5/md5.h
    inc/base_runner.h
    inc/compatibility.h
    inc/error.h
    inc/hexdump.h
    inc/multibyte.h
    inc/multipipe.h
    inc/options.h
    inc/report.h
    inc/restrictions.h
    inc/session.h
    inc/uconvert.h

    # Headers for platform-specific implementations
    inc/delegate.h
    inc/system_pipe.h
)

set(SOURCES
    src/md5/md5.c
    src/base_runner.cpp
    src/compatibility.cpp
    src/error.cpp
    src/hexdump.cpp
    src/multibyte.cpp
    src/multipipe.cpp
    src/options.cpp
    src/report.cpp
    src/restrictions.cpp
    src/session.cpp
    src/uconvert.cpp
)

set(LIB_WIN32_HEADERS
    inc/win32/mutex.h
    inc/win32/platform.h
    inc/win32/platform_report.h
    inc/win32/runner.h
    inc/win32/securerunner.h

    inc/win32/exceptions.h
    inc/win32/stack_walker.h
)

set(LIB_WIN32_SOURCES
    src/win32/delegate.cpp
    src/win32/system_pipe.cpp

    src/win32/platform.cpp
    src/win32/runner.cpp
    src/win32/securerunner.cpp
)

set(LIB_POSIX_HEADERS
    inc/posix/mutex.h
    inc/posix/platform.h
    inc/posix/platform_report.h
    inc/posix/runner.h
    inc/posix/securerunner.h

    inc/posix/signals.h
    inc/posix/rlimit.h
)

set(LIB_POSIX_SOURCES
    src/posix/delegate.cpp
    src/posix/system_pipe.cpp

    src/posix/platform.cpp
    src/posix/runner.cpp
    src/posix/securerunner.cpp
    src/posix/rlimit.cpp
)

set(LIB_LINUX_HEADERS
    inc/posix/linux_affinity.h
    inc/posix/linux_procfs.h
    inc/posix/linux_procfd.h
    inc/posix/linux_seccomp.h
)

set(LIB_LINUX_SOURCES
    src/posix/linux_affinity.cpp
    src/posix/linux_procfs.cpp
    src/posix/linux_procfd.cpp
    src/posix/linux_seccomp.cpp
)

if(UNIX OR CYGWIN)
    set(HEADERS ${HEADERS} ${LIB_POSIX_HEADERS})
    set(SOURCES ${SOURCES} ${LIB_POSIX_SOURCES})
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        set(HEADERS ${HEADERS} ${LIB_LINUX_HEADERS})
        set(SOURCES ${SOURCES} ${LIB_LINUX_SOURCES})
    endif()
    include_directories("${PROJECT_SOURCE_DIR}/inc/posix")
elseif(WIN32)
    set(HEADERS ${HEADERS} ${LIB_WIN32_HEADERS})
    set(SOURCES ${SOURCES} ${LIB_WIN32_SOURCES})
    if(MSVC)
        set(HEADERS ${HEADERS} "inc/win32/stack_walker.h")
        set(SOURCES ${SOURCES} "src/win32/stack_walker.cpp")
        add_definitions(-DWANT_STACKWALKER)
    endif()
    include_directories("${PROJECT_SOURCE_DIR}/inc/win32")
    add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=WINVER)
endif()

set(PROJECT_LIBRARY ${PROJECT_NAME})

add_library(${PROJECT_LIBRARY} ${SOURCES} ${HEADERS})
if (BIT32)
    set_target_properties(${PROJECT_LIBRARY} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
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
