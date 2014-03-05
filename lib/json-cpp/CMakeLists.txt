# Compatibility
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# Project name
PROJECT (jsoncpp)

INCLUDE_DIRECTORIES ("${PROJECT_SOURCE_DIR}/json-cpp/include")
INCLUDE_DIRECTORIES ("${PROJECT_SOURCE_DIR}/json-cpp/src/lib_json")

SET(JSONCPP_INCLUDE_DIRECTORY "${PROJECT_SOURCE_DIR}/json-cpp/src/lib_json" "${PROJECT_SOURCE_DIR}/include")

SET (CMAKE_BUILD_TYPE Release)

FILE (GLOB JSON_SOURCES ./json-cpp/src/lib_json/*.cpp)
FILE (GLOB JSON_HEADERS ./json-cpp/src/lib_json/*.h ./src/lib_json/*.inl ./include/json/*.h)


SET (PROJECT_LIBRARY ${PROJECT_NAME})


ADD_LIBRARY (${PROJECT_LIBRARY} ${JSON_SOURCES} ${JSON_HEADERS})
