tde_install_icons( tdepackagekit DESTINATION ${ICON_INSTALL_DIR} )

install( FILES tdepackagekit.desktop tdepackagekit-install-file.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES tdepackagekit-notifier.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR} )
