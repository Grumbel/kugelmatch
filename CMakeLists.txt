# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
cmake_minimum_required(VERSION 3.16)
project(kugelmatch VERSION 1.2.12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Portable release flags by default; enable -march=native for local builds.
option(KUGELMATCH_NATIVE "Enable -march=native in Release builds" OFF)
set(KUGELMATCH_RELEASE_FLAGS "-O3 -DNDEBUG -ffast-math")
if(KUGELMATCH_NATIVE)
  set(KUGELMATCH_RELEASE_FLAGS "${KUGELMATCH_RELEASE_FLAGS} -march=native")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${KUGELMATCH_RELEASE_FLAGS}" CACHE STRING "" FORCE)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(kugelmatch
  src/main.cpp
  src/raytracer.cpp
  src/gpu_raytracer.cpp
  src/game.cpp
  src/audio.cpp
  src/config.cpp
  src/glyphs.cpp
)

target_include_directories(kugelmatch PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(kugelmatch PRIVATE
  ${SDL2_LIBRARIES}
  OpenGL::GL
  Threads::Threads
)

target_compile_options(kugelmatch PRIVATE ${SDL2_CFLAGS_OTHER})

# --- Install (Linux packaging) ---
include(GNUInstallDirs)

install(TARGETS kugelmatch
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY shaders/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/kugelmatch/shaders
  FILES_MATCHING PATTERN "*.vert" PATTERN "*.frag"
)

install(FILES man/kugelmatch.6
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man6
)

install(FILES data/kugelmatch.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES data/icons/hicolor/scalable/apps/kugelmatch.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

install(FILES data/metainfo/org.kugelmatch.KugelMatch.metainfo.xml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)
