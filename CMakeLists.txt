cmake_minimum_required(VERSION 3.16)
project(kugelmatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(kugelmatch
  src/main.cpp
  src/raytracer.cpp
  src/gpu_raytracer.cpp
  src/game.cpp
)

target_include_directories(kugelmatch PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(kugelmatch PRIVATE
  Threads::Threads
  OpenGL::GL
  ${SDL2_LIBRARIES}
)

target_compile_options(kugelmatch PRIVATE ${SDL2_CFLAGS_OTHER})

install(TARGETS kugelmatch RUNTIME DESTINATION bin)
install(DIRECTORY shaders DESTINATION share/kugelmatch)
