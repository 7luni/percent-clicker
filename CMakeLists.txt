cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(PercentClicker VERSION 1.2.0)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# SystemInput.cpp includes the operating system's headers (Windows.h / Apple's
# ApplicationServices). Those clash with Geode's names (e.g. "CommentType") if
# they end up in the same "unity build" file, so compile it on its own.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemInput.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} user32)
endif()

if (APPLE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    target_link_libraries(${PROJECT_NAME} "-framework ApplicationServices")
endif()
