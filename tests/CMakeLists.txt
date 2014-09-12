pkg_check_modules(TUT REQUIRED tut>=0.0.3)
include_directories(${TUT_INCLUDES})

find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

set(UNIT_TESTS os)

MACRO(UNIT_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} qtaround ${COR_LIBRARIES})
  qt5_use_modules(${_exe_name} Core)
ENDMACRO(UNIT_TEST)

foreach(t ${UNIT_TESTS})
  UNIT_TEST(${t})
endforeach(t)
