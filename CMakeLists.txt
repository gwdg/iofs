cmake_minimum_required (VERSION 3.0)

set(PROJECT_NAME IOFS)
project (${PROJECT_NAME})

find_package (PkgConfig REQUIRED)
pkg_search_module(FUSE3 REQUIRED fuse3)

set(CMAKE_C_FLAGS "-O0 -g3")

set( CMAKE_REQUIRED_INCLUDES ${SQLITE3_INCLUDE_DIRS} )
set( CMAKE_REQUIRED_LIBRARIES ${SQLITE3_LIBRARIES} )

add_subdirectory(src)
