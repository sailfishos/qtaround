set(CMAKE_AUTOMOC TRUE)

add_library(qtaround SHARED
  debug.cpp os.cpp json.cpp sys.cpp subprocess.cpp util.cpp
  )
qt5_use_modules(qtaround Core)
target_link_libraries(qtaround ${COR_LIBRARIES})
set_target_properties(qtaround PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS qtaround DESTINATION ${DST_LIB})
