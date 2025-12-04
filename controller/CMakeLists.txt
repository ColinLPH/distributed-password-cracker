cmake_minimum_required(VERSION 3.16)
project(controller VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(controller ${SRC_FILES})

target_include_directories(controller
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Compiler warning flags
if(MSVC)
  target_compile_options(controller PRIVATE /W4 /permissive-)
else()
  target_compile_options(controller PRIVATE -Wall -Wextra -Wpedantic)
    # Link with libcrypt on Unix/Linux
  #target_link_libraries(controller PRIVATE crypt)
endif()
