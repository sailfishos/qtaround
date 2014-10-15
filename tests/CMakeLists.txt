pkg_check_modules(TUT REQUIRED tut>=0.0.3)
include_directories(${TUT_INCLUDES})

find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

set(UNIT_TESTS os dbus misc)
set(TESTS_DIR /opt/tests/qtaround)

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} qtaround ${COR_LIBRARIES})
  qt5_use_modules(${_exe_name} Core)
  install(TARGETS ${_exe_name} DESTINATION ${TESTS_DIR})
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
install(PROGRAMS ${SH_FILES} DESTINATION ${TESTS_DIR})
install(FILES tests.xml DESTINATION ${TESTS_DIR})
