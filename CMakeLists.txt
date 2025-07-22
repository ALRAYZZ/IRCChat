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

# Find Qt5
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# Enable Qt's MOC (Meta-Object Compiler) - moved before executable definitions
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
    src/client/client_window.cpp
    include/client/client.hpp
    include/client/client_window.hpp
)
target_link_libraries(irc_client PRIVATE Boost::system spdlog::spdlog Qt5::Core Qt5::Widgets)

# Set Qt5 properties for the client target
set_target_properties(irc_client PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)