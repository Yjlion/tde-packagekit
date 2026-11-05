cmake_minimum_required( VERSION 3.5 )

project( tdepackagekit VERSION 0.1.0 LANGUAGES C CXX )

set( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

include( FindPkgConfig )
include( CheckCXXSourceCompiles )
include( TDEMacros )
include( TDESetupPaths )
tde_setup_paths( )

option( BUILD_TESTING "Build the test programs" ON )

include( ConfigureChecks.cmake )

add_definitions( -DHAVE_CONFIG_H -DTDEPK_VERSION="${PROJECT_VERSION}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TQT_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )

add_subdirectory( src )
add_subdirectory( data )

if( BUILD_TESTING )
  enable_testing( )
  add_subdirectory( tests )
endif( )
