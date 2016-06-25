cmake_minimum_required (VERSION 2.6)
project (narg)
set(CMAKE_C_FLAGS "-Os -std=gnu99 -Wall -Werror")

add_subdirectory(test)

add_library(narg STATIC argparse.c printopt.c utf8len.c)

INSTALL(FILES narg.h DESTINATION include)
INSTALL(TARGETS narg ARCHIVE DESTINATION lib)

INCLUDE(CPack)
