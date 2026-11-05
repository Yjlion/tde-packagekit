include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/pk ${CMAKE_BINARY_DIR}/src/pk ${TDE_INCLUDE_DIR} ${TQT_INCLUDE_DIRS} ${DBUS_TQT_INCLUDE_DIRS} )
link_directories( ${TQT_LIBRARY_DIRS} ${TDE_LIB_DIR} )
tde_add_executable( pkcore-cli AUTOMOC
  SOURCES pkcore_cli.cpp
  LINK tdepkcore-static ${TQT_LIBRARIES} ${DBUS_TQT_LIBRARIES} tdecore-shared
)

tde_add_executable( mime-offers
  SOURCES mime_offers.cpp
  LINK ${TQT_LIBRARIES} tdeio-shared tdecore-shared
)
