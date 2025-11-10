cmake_minimum_required(VERSION 3.16)
project(worker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(worker ${SRC_FILES})

target_include_directories(worker
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Compiler warning flags
if(MSVC)
  target_compile_options(worker PRIVATE /W4 /permissive-)
else()
  target_compile_options(worker PRIVATE -Wall -Wextra -Wpedantic)
    # Link with libcrypt on Unix/Linux
  target_link_libraries(worker PRIVATE crypt)
endif()
