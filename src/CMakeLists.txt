add_executable(iofs iofs.c)

link_directories(iofs ${FUSE3_LIBRARIES_DIRS})
target_link_libraries(iofs ${FUSE3_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR}/src ${FUSE3_INCLUDE_DIRS})
install(TARGETS iofs DESTINATION bin)
