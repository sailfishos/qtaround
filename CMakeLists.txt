project(QTAROUND)
cmake_minimum_required(VERSION 2.8.8)

IF(NOT DEFINED VERSION OR VERSION STREQUAL "")
message(FATAL_ERROR "Define VERSION")
ENDIF(NOT DEFINED VERSION OR VERSION STREQUAL "")

IF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")
set(LONG_VERSION ${VERSION})
ENDIF(NOT DEFINED LONG_VERSION OR LONG_VERSION STREQUAL "")

find_package(Cor REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories(
  ${COR_INCLUDE_DIRS}
)
link_directories(
  ${COR_LIBRARY_DIRS}
)

set(QTAROUND_DBUS_MOC_HEADERS
  ${CMAKE_SOURCE_DIR}/include/qtaround/dbus.hpp
)

set(QTAROUND_MOC_HEADERS
  ${CMAKE_SOURCE_DIR}/include/qtaround/mt.hpp
)

add_subdirectory(src)
add_subdirectory(tests)

configure_file(qtaround.pc.in qtaround.pc @ONLY)
configure_file(qtaround-1.pc.in qtaround-1.pc @ONLY)
configure_file(qtaround-dbus.pc.in qtaround-dbus.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/qtaround.pc
  ${CMAKE_CURRENT_BINARY_DIR}/qtaround-1.pc
  ${CMAKE_CURRENT_BINARY_DIR}/qtaround-dbus.pc
  DESTINATION
  ${DST_LIB}/pkgconfig
)

install(
  DIRECTORY include/qtaround
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h"
)
