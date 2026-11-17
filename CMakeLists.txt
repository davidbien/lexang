# CMakeLists.txt for lexang.
# lexang is a header-only library: this defines an interface library for it and builds its unit tests.
# It may be built standalone or added to another project with add_subdirectory().
#
# lexang requires the bienutil and dgraph headers:
#   set LEXANG_BIENUTIL_DIR to where the bienutil headers are found if they are not in ../bienutil.
#   set LEXANG_DGRAPH_DIR to where the dgraph headers are found if they are not in ../dgraph.
# Set LEXANG_BUILD_TESTS to OFF to not build the unit tests.

cmake_minimum_required (VERSION 3.14)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(LEXANG_STANDALONE TRUE)
  project ("lexang" CXX)
else()
  set(LEXANG_STANDALONE FALSE)
endif()

set(LEXANG_BIENUTIL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bienutil" CACHE PATH "Directory containing the bienutil headers.")
if(NOT EXISTS "${LEXANG_BIENUTIL_DIR}/bienutil.h")
  message(FATAL_ERROR "lexang: bienutil headers not found in [${LEXANG_BIENUTIL_DIR}] - set LEXANG_BIENUTIL_DIR.")
endif()
set(LEXANG_DGRAPH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../dgraph" CACHE PATH "Directory containing the dgraph headers.")
if(NOT EXISTS "${LEXANG_DGRAPH_DIR}/_gr_inc.h")
  message(FATAL_ERROR "lexang: dgraph headers not found in [${LEXANG_DGRAPH_DIR}] - set LEXANG_DGRAPH_DIR.")
endif()

if(LEXANG_STANDALONE)
  # When part of another project we expect it to have set things up - bienutil.cmake modifies global compiler flags.
  include ( "${LEXANG_BIENUTIL_DIR}/bienutil.cmake" )
  enable_testing()
endif()

add_library(lexang INTERFACE)
target_include_directories(lexang INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${LEXANG_BIENUTIL_DIR}>
  $<BUILD_INTERFACE:${LEXANG_DGRAPH_DIR}>
)

option(LEXANG_BUILD_TESTS "Build the lexang unit tests." ON)
if(LEXANG_BUILD_TESTS)
  add_subdirectory(test)
endif()
