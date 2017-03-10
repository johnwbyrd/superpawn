# CMakeLists.txt

set( PROJECT_NAME superpawn )
set( BUILD_FOR_RELEASE ON BOOL 
	"Builds executable and documentation for release to web site.")
set( BUILD_DOCS ON BOOL
	"Builds documentation in common output formats for Web and release.")
cmake_minimum_required (VERSION 2.6.4)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cotire/CMake" )	
set (CMAKE_OSX_ARCHITECTURES “i386;x86_64”)
include(cotire OPTIONAL)
project ( ${PROJECT_NAME} )
find_package( Threads )
set( CMAKE_LEGACY_CYGWIN_WIN32 0 )
if (MSVC)
	set(BUILD_PLATFORM "win32")
endif()

set (BUILD_SHARED_LIBS FALSE) # should force the libraries to be static

if( BUILD_DOCS )
	# Need to change if we ever build docs on some other platform
	if( MSVC )
		set( MMD_BINARY "${CMAKE_SOURCE_DIR}/tools/${BUILD_PLATFORM}/multimarkdown/multimarkdown.exe")
	endif()
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # using Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
  # using Intel C++
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  # using Visual Studio C++
  # Force to always compile with W4; all warnings are errors
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
  endif()
  if (BUILD_SHARED_LIBS)
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MDd")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MDd")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -MD")
  else()
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MTd")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -MTd")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -MT")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -MT")
  endif()
  if( BUILD_DOCS )
	# Need to change if we ever build docs on some other platform
    set( MMD_BINARY "${CMAKE_SOURCE_DIR}/tools/${BUILD_PLATFORM}/multimarkdown/multimarkdown.exe")
  endif()
endif()

add_executable( ${PROJECT_NAME} Chess.cpp )
target_link_libraries( ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
configure_file( ${CMAKE_SOURCE_DIR}/BuildInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.h )

# All targets get the logo file
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
					   COMMAND ${CMAKE_COMMAND} -E copy
						${CMAKE_SOURCE_DIR}/logo/logo-small.bmp $<TARGET_FILE_DIR:${PROJECT_NAME}> )

# All targets can have the markdown format documentation
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
					   COMMAND ${CMAKE_COMMAND} -E copy
						${CMAKE_SOURCE_DIR}/README.md $<TARGET_FILE_DIR:${PROJECT_NAME}> )

if (COMMAND cotire)
	cotire( ${PROJECT_NAME} )
endif()
