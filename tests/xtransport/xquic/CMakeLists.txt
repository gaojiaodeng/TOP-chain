cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xquic_test_src)

add_executable(xquic_test ${xquic_test_src})

add_dependencies(xquic_test xtransport)

target_link_libraries(xquic_test PRIVATE xcommon xtransport gtest pthread)

# target_include_directories(xquic_test PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/libevent/include/) # for libevent header file
# target_include_directories(xquic_test PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/include/) # for event-config.h
# target_link_libraries(xquic_test PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/lib/libevent.a) # static library output path
# target_link_libraries(xquic_test PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/lib/libevent_pthreads.a) # for event pthread
