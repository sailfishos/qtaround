pkg_check_modules(TUT REQUIRED tut>=0.0.3)
include_directories(${TUT_INCLUDES})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC TRUE)
find_package(Qt5Core REQUIRED)

testrunner_project(qtaround)
set(UNIT_TESTS os dbus misc mt debug)

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} qtaround ${COR_LIBRARIES})
  qt5_use_modules(${_exe_name} Core)
  testrunner_install(TARGETS ${_exe_name})
ENDMACRO(UNIT_TEST)

foreach(t ${UNIT_TESTS})
  UNIT_TEST(${t})
endforeach(t)

# more for dbus tests

find_package(Qt5DBus REQUIRED)
qt5_use_modules(test_dbus Core DBus)
#set_source_files_properties(DBus.Properties.xml PROPERTIES NO_NAMESPACE TRUE)
#qt5_add_dbus_interface(SRC DBus.Properties.xml prop_interface)
#add_library(testprop STATIC ${SRC})
target_link_libraries(test_dbus
  qtaround-dbus
  #testprop
  )

configure_file(tests.xml.in tests.xml @ONLY)
FILE(GLOB SH_FILES *.sh)
testrunner_install(PROGRAMS ${SH_FILES})

# linking tests
add_executable(test-linking qtaround-link-main.cpp qtaround-link-2.cpp)
target_link_libraries(test-linking qtaround)
qt5_use_modules(test-linking Core)
add_executable(test-linking-dbus qtaround-dbus-link-main.cpp qtaround-dbus-link-2.cpp)
target_link_libraries(test-linking-dbus qtaround qtaround-dbus)
qt5_use_modules(test-linking-dbus Core DBus)
