cmake_minimum_required(VERSION 3.13)
project(TemportalEngine)

set(CMAKE_CXX_STANDARD 17)

# https://foonathan.net/blog/2016/07/07/cmake-dependency-handling.html
include_directories("libs/glfw/include")
add_subdirectory("libs/glfw")

file(GLOB TemportalEngine_Source "source/*.h" "source/*.cpp")

add_executable(TemportalEngine ${TemportalEngine_Source})
target_link_libraries(TemportalEngine glfw)
target_include_directories(TemportalEngine PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_sources(TemportalEngine PRIVATE ${CMAKE_SOURCE_DIR}/source)

