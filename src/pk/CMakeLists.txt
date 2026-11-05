include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${DBUS_TQT_INCLUDE_DIRS}
)
link_directories( ${TQT_LIBRARY_DIRS} ${TDE_LIB_DIR} )

tde_add_library( tdepkcore STATIC_PIC AUTOMOC
  SOURCES pkenums.cpp packageid.cpp pkdaemon.cpp pktransaction.cpp changequeue.cpp tdepksettings.cpp
  LINK ${DBUS_TQT_LIBRARIES} tdecore-shared
)
