cmake_minimum_required(VERSION 3.10)


project(drlt)
file(GLOB_RECURSE SOURCE_FILES
        "*.h"
        "*.c"
        "*.cpp")

FIND_PACKAGE(Boost COMPONENTS program_options system log filesystem REQUIRED)

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_MULTITHREADED ON)

add_definitions(-DBOOST_LOG_DYN_LINK)

find_package(Boost 1.74.0 COMPONENTS  system
                                    thread
                                    log
                                    log_setup
                                    program_options
                                    filesystem
                                    iostreams
            REQUIRED)


find_package(ZLIB REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})


add_executable(drlt ${SOURCE_FILES})
target_link_libraries(drlt ${Boost_LIBRARIES})
target_link_libraries(drlt ZLIB::ZLIB)

