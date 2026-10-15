cmake_minimum_required(VERSION 3.25)
project(wwe2k14 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

find_package(Git QUIET)
set(WWE13_BUILD_COMMIT unknown)
set(WWE13_BUILD_DIRTY unknown)
set(WWE13_SDK_COMMIT unknown)
set(WWE13_SDK_DIRTY unknown)
if(GIT_FOUND)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE WWE13_BUILD_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE WWE13_GIT_RESULT ERROR_QUIET)
  if(NOT WWE13_GIT_RESULT EQUAL 0)
    set(WWE13_BUILD_COMMIT unknown)
  else()
    execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE WWE13_GIT_STATUS OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE WWE13_GIT_STATUS_RESULT ERROR_QUIET)
    if(WWE13_GIT_STATUS_RESULT EQUAL 0)
      if(WWE13_GIT_STATUS STREQUAL "")
        set(WWE13_BUILD_DIRTY no)
      else()
        set(WWE13_BUILD_DIRTY yes)
      endif()
    endif()
  endif()

  if(NOT DEFINED WWE13_SDK_ROOT OR WWE13_SDK_ROOT STREQUAL "")
    set(WWE13_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../tools/rexglue")
  endif()
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
    WORKING_DIRECTORY "${WWE13_SDK_ROOT}"
    OUTPUT_VARIABLE WWE13_SDK_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE WWE13_SDK_GIT_RESULT ERROR_QUIET)
  if(WWE13_SDK_GIT_RESULT EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain
      WORKING_DIRECTORY "${WWE13_SDK_ROOT}"
      OUTPUT_VARIABLE WWE13_SDK_GIT_STATUS OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE WWE13_SDK_GIT_STATUS_RESULT ERROR_QUIET)
    if(WWE13_SDK_GIT_STATUS_RESULT EQUAL 0)
      if(WWE13_SDK_GIT_STATUS STREQUAL "")
        set(WWE13_SDK_DIRTY no)
      else()
        set(WWE13_SDK_DIRTY yes)
      endif()
    endif()
  endif()
endif()
include(cmake/wwe13_version.cmake)
string(TIMESTAMP WWE13_BUILD_DATE "%Y-%m-%dT%H:%M:%SZ" UTC)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(src/wwe13_build_info.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/generated/wwe13_build_info.h" @ONLY)

add_executable(wwe2k14
  src/main.cpp
  src/game_version_check.cpp
  src/release_diagnostics.cpp
  src/single_instance.cpp
  src/startup_probe.cpp
  src/present_interval_2k14.cpp
  src/cloth_handshake_2k14.cpp)
target_include_directories(wwe2k14 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
rexglue_setup_target(wwe2k14)
if(WIN32)
  target_link_libraries(wwe2k14 PRIVATE advapi32)
  # Link map next to the exe: symbol -> object resolution for Windows-only link checks.
  target_link_options(wwe2k14 PRIVATE "LINKER:/map:$<TARGET_FILE_DIR:wwe2k14>/wwe2k14.map")
endif()
