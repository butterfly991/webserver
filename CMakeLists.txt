cmake_minimum_required(VERSION 3.10)

project(ServerProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
file(GLOB SOURCES src/*.cpp)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)

# Target
add_executable(ServerProject ${SOURCES})

# Link Boost libraries
target_link_libraries(ServerProject Boost::system Boost::filesystem Boost::program_options)

