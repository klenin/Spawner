# Compatibility
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)


INCLUDE("../../cmake/gather.cmake")

GET_PROJECT_NAME(PROJECT_ID)
GET_PROJECT_EXECUTABLE_NAME(${PROJECT_ID} EXECUTABLE_NAME)

# Project name
PROJECT (${PROJECT_ID})

LIST (APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

SET (CMAKE_BUILD_TYPE Debug)

FILE (GLOB SOURCES main.cpp)

SET (PROJECT_EXECUTABLE ${EXECUTABLE_NAME})

SET(CMAKE_EXE_LINKER_FLAGS "-static")

ADD_EXECUTABLE (${PROJECT_EXECUTABLE} ${SOURCES})

ADD_COPY_STAGE(${PROJECT_EXECUTABLE})
