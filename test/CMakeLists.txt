include_directories(${DIRDIG_SOURCE_DIR})
link_directories(${DIRDIG_SOURCE_DIR})

add_executable(test_utf8len test_utf8len.c)
add_executable(test_getopt test_getopt.c)
add_executable(test_wordcount test_wordcount.c)
target_link_libraries(test_utf8len narg)
target_link_libraries(test_getopt narg)
target_link_libraries(test_wordcount narg)

add_executable(mvring mvring.c)
target_link_libraries(mvring narg)

add_executable(colorcat colorcat.c)
target_link_libraries(colorcat narg)
