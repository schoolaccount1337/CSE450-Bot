cmake_minimum_required(VERSION 3.10)

project(cse450bot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(cse450bot 
    src/main.cpp
    src/command_register.cpp
    src/bot_command_handler.cpp
    src/config/config.cpp
    src/handlers/rss_feed_handler.cpp
    src/handlers/canvas_handler.cpp
)

target_include_directories(cse450bot PRIVATE src/include src/config /usr/include/jsoncpp src/handlers)

find_package(jsoncpp REQUIRED)

find_package(CURL REQUIRED)

find_package(LibXml2 REQUIRED)

target_link_libraries(cse450bot dpp jsoncpp CURL::libcurl ${LIBXML2_LIBRARIES})

include_directories(${LIBXML2_INCLUDE_DIR})