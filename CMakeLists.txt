# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.21)
project(group-visibility-sync VERSION 0.1.0 LANGUAGES CXX)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets)

# Prefer the CMake packages exported by an OBS Studio development build.
# Alternatively, point at an OBS source tree, its generated headers, and the
# two import libraries when an installed SDK does not provide those packages.
set(OBS_SOURCE_DIR "" CACHE PATH "OBS Studio source tree")
set(OBS_GENERATED_INCLUDE_DIR "" CACHE PATH "Directory with generated OBS headers")
set(OBS_LIBOBS_LIBRARY "" CACHE FILEPATH "libobs import library")
set(OBS_FRONTEND_LIBRARY "" CACHE FILEPATH "OBS frontend API import library")
if(OBS_SOURCE_DIR AND OBS_GENERATED_INCLUDE_DIR AND OBS_LIBOBS_LIBRARY AND OBS_FRONTEND_LIBRARY)
  add_library(obs-sdk-libobs UNKNOWN IMPORTED)
  set_target_properties(obs-sdk-libobs PROPERTIES
    IMPORTED_LOCATION "${OBS_LIBOBS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/libobs;${OBS_GENERATED_INCLUDE_DIR}")
  add_library(obs-sdk-frontend UNKNOWN IMPORTED)
  set_target_properties(obs-sdk-frontend PROPERTIES
    IMPORTED_LOCATION "${OBS_FRONTEND_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/frontend/api")
  set(OBS_LIB_TARGET obs-sdk-libobs)
  set(OBS_FRONTEND_TARGET obs-sdk-frontend)
else()
  find_package(libobs CONFIG REQUIRED)
  find_package(obs-frontend-api CONFIG REQUIRED)
  set(OBS_LIB_TARGET OBS::libobs)
  set(OBS_FRONTEND_TARGET OBS::obs-frontend-api)
endif()

add_library(group-visibility-sync MODULE src/plugin.cpp src/sync-core.cpp)
target_compile_features(group-visibility-sync PRIVATE cxx_std_17)
target_compile_definitions(group-visibility-sync PRIVATE NOMINMAX)
target_link_libraries(group-visibility-sync PRIVATE
  ${OBS_LIB_TARGET} ${OBS_FRONTEND_TARGET} Qt6::Core Qt6::Widgets)
set_target_properties(group-visibility-sync PROPERTIES PREFIX "")
if(MSVC)
  target_compile_options(group-visibility-sync PRIVATE /utf-8 /permissive- /Zc:preprocessor /Zc:__cplusplus)
endif()

option(BUILD_GROUP_SYNC_TESTS "Build the synthetic libobs test" OFF)
if(BUILD_GROUP_SYNC_TESTS)
  add_executable(group-sync-tests tests/sync-tests.cpp src/sync-core.cpp)
  target_compile_features(group-sync-tests PRIVATE cxx_std_17)
  target_compile_definitions(group-sync-tests PRIVATE NOMINMAX)
  target_link_libraries(group-sync-tests PRIVATE ${OBS_LIB_TARGET})
  if(MSVC)
    target_compile_options(group-sync-tests PRIVATE /utf-8 /permissive- /Zc:preprocessor /Zc:__cplusplus)
  endif()
endif()
