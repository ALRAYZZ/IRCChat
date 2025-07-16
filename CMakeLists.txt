cmake_minimum_required(VERSION 3.15)
project(IRCChat LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost.Asio
find_package(Boost 1.78 REQUIRED COMPONENTS system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# Find spdlog
find_package(spdlog CONFIG REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Server executable
add_executable(irc_server
    src/server/main.cpp
    src/server/server.cpp
)
target_link_libraries(irc_server PRIVATE Boost::system spdlog::spdlog)

# Client executable
add_executable(irc_client
    src/client/main.cpp
    src/client/client.cpp
)
target_link_libraries(irc_client PRIVATE Boost::system spdlog::spdlog)