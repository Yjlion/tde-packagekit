include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/pk
  ${CMAKE_BINARY_DIR}/src/pk
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${DBUS_TQT_INCLUDE_DIRS}
)
link_directories( ${TQT_LIBRARY_DIRS} ${TDE_LIB_DIR} )

tde_add_executable( tdepackagekit AUTOMOC
  SOURCES
    main.cpp mainwindow.cpp applyflow.cpp queryhelper.cpp
    browsepage.cpp updatespage.cpp historypage.cpp repospage.cpp
    transactiondialog.cpp confirmchangesdialog.cpp signaturedialog.cpp
    euladialog.cpp settingsdialog.cpp packageicon.cpp errordialog.cpp
  LINK tdepkcore-static tdeui-shared tdecore-shared tdeio-shared DCOP-shared ${DBUS_TQT_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
