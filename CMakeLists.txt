cmake_minimum_required(VERSION 3.10)
project(kuserspace VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    lib/Buffer.cpp
    lib/List.cpp
    lib/Memory.cpp
    lib/Parser.cpp
    lib/Processor.cpp
)

# Create shared library
add_library(kuserspace SHARED ${SOURCES})

# Link against required libraries
target_link_libraries(kuserspace PRIVATE Threads::Threads)

# Install rules
install(TARGETS kuserspace
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/kuserspace
    FILES_MATCHING PATTERN "*.h"
)
