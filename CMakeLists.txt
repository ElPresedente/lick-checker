cmake_minimum_required(VERSION 3.25)
project(check_links_rewrite)

set(CMAKE_CXX_STANDARD 17)

find_package(cpr REQUIRED)

find_package(Boost REQUIRED COMPONENTS locale)

find_library(URIPARSER_LIB
        NAMES uriparser
        HINTS /usr/locale/lib64
)

add_executable(check_links_rewrite main.cpp link_checker.cpp link_checker.h)

target_link_libraries(check_links_rewrite
        cpr::cpr
        Boost::locale
        gumbo_query
        ${URIPARSER_LIB}
)
