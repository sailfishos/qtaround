set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC TRUE)

qt5_wrap_cpp(QTAROUND_MOC_SRC ${QTAROUND_MOC_HEADERS})

add_library(qtaround SHARED
  ${QTAROUND_MOC_SRC}
  debug.cpp os.cpp json.cpp sys.cpp subprocess.cpp util.cpp
  mt.cpp
  )
qt5_use_modules(qtaround Core)
target_link_libraries(qtaround ${COR_LIBRARIES})
set_target_properties(qtaround PROPERTIES
  SOVERSION 2
  VERSION ${VERSION}
  )
install(TARGETS qtaround DESTINATION ${DST_LIB})

qt5_wrap_cpp(QTAROUND_DBUS_SRC ${QTAROUND_DBUS_MOC_HEADERS})

add_library(qtaround-dbus SHARED
  dbus.cpp
  ${QTAROUND_DBUS_SRC}
  )
qt5_use_modules(qtaround-dbus Core DBus)
target_link_libraries(qtaround ${COR_LIBRARIES})
set_target_properties(qtaround-dbus PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )
install(TARGETS qtaround-dbus DESTINATION ${DST_LIB})
