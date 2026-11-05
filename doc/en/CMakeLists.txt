# The screenshots are shared with the Markdown docs, so they are installed from screenshots/.
set( _shots browse-details confirm-changes updates-details history repositories
     notifier-popup settings error-dialog )
set( _files index.docbook )
foreach( _shot ${_shots} )
  list( APPEND _files ${CMAKE_SOURCE_DIR}/screenshots/${_shot}.png )
endforeach( )

tde_create_handbook( DESTINATION tdepackagekit FILES ${_files} )
