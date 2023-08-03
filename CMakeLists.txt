cmake_minimum_required(VERSION 3.25)
project(check_links_rewrite)

set(CMAKE_CXX_STANDARD 20)

find_package(cpr REQUIRED)

find_package(Boost REQUIRED COMPONENTS locale)

find_library(URIPARSER_LIB
        NAMES uriparser
        HINTS /usr/locale/lib64
)

include(FetchContent)
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

add_executable(check_links_rewrite main.cpp Application.cpp Application.h Page.h)

target_link_libraries(check_links_rewrite
        cpr::cpr
        Boost::locale
        gumbo_query
        ${URIPARSER_LIB}
)
