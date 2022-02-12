add_executable(iofs iofs.c elasticsearch.c)

link_directories(iofs ${FUSE3_LIBRARIES_DIRS})
target_link_libraries(iofs ${FUSE3_LIBRARIES})
link_directories(iofs ${CURL_LIBRARIES_DIRS})
target_link_libraries(iofs ${CURL_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR}/src ${FUSE3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS iofs DESTINATION bin)
