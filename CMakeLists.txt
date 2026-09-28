cmake_minimum_required(VERSION 3.16)
project(kugelmatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Aggressive release optimisations for the raytracer
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

add_executable(kugelmatch
  src/main.cpp
  src/raytracer.cpp
  src/game.cpp
)

target_include_directories(kugelmatch PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(kugelmatch PRIVATE
  ${SDL2_LIBRARIES}
  Threads::Threads
)

# On some systems SDL2 needs explicit libraries
if(TARGET SDL2::SDL2)
  target_link_libraries(kugelmatch PRIVATE SDL2::SDL2)
endif()
