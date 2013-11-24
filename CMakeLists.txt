set( PROJECT_NAME ippon )
cmake_minimum_required (VERSION 2.6.4)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cotire/CMake" )	
include(cotire OPTIONAL)
project ( ${PROJECT_NAME} )

find_package( Threads )
set( CMAKE_LEGACY_CYGWIN_WIN32 0 )

if(MSVC)
  # Force to always compile with W4; all warnings are errors
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Warnings are errors; generate lots of warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Werror")
endif()

add_executable( ${PROJECT_NAME} Chess.cpp )

target_link_libraries( ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

if (COMMAND cotire)
	cotire( ${PROJECT_NAME} )
endif()
