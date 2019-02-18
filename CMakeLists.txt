cmake_minimum_required(VERSION 3.13)
project(TemportalEngine)

set(CMAKE_CXX_STANDARD 17)

# https://foonathan.net/blog/2016/07/07/cmake-dependency-handling.html
include_directories("libs/glfw/include")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("libs/glfw")

file(GLOB_RECURSE TemportalEngine_Source "source/*")

add_executable(TemportalEngine ${TemportalEngine_Source})
target_link_libraries(TemportalEngine glfw)
target_include_directories(TemportalEngine PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_sources(TemportalEngine PRIVATE ${TemportalEngine_Source})

