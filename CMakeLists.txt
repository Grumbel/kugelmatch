# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
cmake_minimum_required(VERSION 3.16)

# Source of truth: top-level VERSION (e.g. "1.2.14-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix may append .<revCount>+g<rev>).
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
  # Local -dev builds: append git rev count + short hash when available.
  if(PROJECT_VERSION_FULL MATCHES "-dev")
    find_package(Git QUIET)
    if(Git_FOUND)
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _km_rev_count
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _km_short_rev
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" diff --quiet
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _km_dirty
        ERROR_QUIET
      )
      if(_km_rev_count STREQUAL "")
        set(_km_rev_count "0")
      endif()
      if(_km_short_rev STREQUAL "")
        set(_km_short_rev "dirty")
      endif()
      set(_km_dirty_suf "")
      if(NOT _km_dirty EQUAL 0)
        set(_km_dirty_suf "-dirty")
      endif()
      set(PROJECT_VERSION_FULL
          "${PROJECT_VERSION_FULL}.${_km_rev_count}+g${_km_short_rev}${_km_dirty_suf}")
    endif()
  endif()
endif()

# project() needs a numeric version; strip -dev / +git / similar suffixes.
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")
if(_project_version_cmake STREQUAL "")
  set(_project_version_cmake "0.0.0")
endif()

project(kugelmatch VERSION ${_project_version_cmake} LANGUAGES CXX)

message(STATUS "KugelMatch version: ${PROJECT_VERSION_FULL} (CMake ${PROJECT_VERSION})")

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

# --- SDL2 (CONFIG package, pkg-config, Emscripten, or explicit paths) ---
if(EMSCRIPTEN)
  if(SDL2_ROOT)
    if(NOT EXISTS "${SDL2_ROOT}/lib/libSDL2.a" OR NOT EXISTS "${SDL2_ROOT}/include/SDL2/SDL.h")
      message(FATAL_ERROR "SDL2_ROOT=${SDL2_ROOT} missing lib/libSDL2.a or include/SDL2/SDL.h")
    endif()
    set(SDL2_INCLUDE_DIRS "${SDL2_ROOT}/include/SDL2" "${SDL2_ROOT}/include")
    set(SDL2_LIBRARIES "${SDL2_ROOT}/lib/libSDL2.a")
  else()
    add_compile_options(-sUSE_SDL=2)
    add_link_options(-sUSE_SDL=2)
    set(SDL2_INCLUDE_DIRS "")
    set(SDL2_LIBRARIES "")
  endif()
elseif(NOT SDL2_LIBRARIES)
  # MinGW cross builds pass -DSDL2_DIR=... (official SDL2 CMake package).
  find_package(SDL2 QUIET CONFIG)
  if(SDL2_FOUND)
    # SDL2::SDL2 / SDL2::SDL2main from the CONFIG package.
    set(SDL2_LIBRARIES "")
    if(TARGET SDL2::SDL2main)
      list(APPEND SDL2_LIBRARIES SDL2::SDL2main)
    endif()
    list(APPEND SDL2_LIBRARIES SDL2::SDL2)
    set(SDL2_INCLUDE_DIRS "")
    message(STATUS "SDL2 via CMake CONFIG (${SDL2_DIR})")
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
  endif()
endif()

# GPU path is OpenGL ES 2.0 (SDL_GL_GetProcAddress). Skip desktop GL on GLES targets.
if(EMSCRIPTEN OR ANDROID)
  add_compile_definitions(KUGELMATCH_USE_OPENGLES2=1)
endif()

option(KUGELMATCH_OPENGLES2 "GPU backend uses OpenGL ES 2.0 (Web/Android/R36S; also .#kugelmatch-gles2)" OFF)
set(KUGELMATCH_GL_LIBS "")
if(NOT EMSCRIPTEN AND NOT ANDROID)
  if(KUGELMATCH_OPENGLES2)
    add_compile_definitions(KUGELMATCH_USE_OPENGLES2=1)
    find_library(GLESV2_LIBRARY NAMES GLESv2 libGLESv2)
    if(GLESV2_LIBRARY)
      list(APPEND KUGELMATCH_GL_LIBS ${GLESV2_LIBRARY})
      message(STATUS "Linking GLESv2: ${GLESV2_LIBRARY}")
    endif()
    find_library(EGL_LIBRARY NAMES EGL libEGL)
    if(EGL_LIBRARY)
      list(APPEND KUGELMATCH_GL_LIBS ${EGL_LIBRARY})
    endif()
  else()
    find_package(OpenGL REQUIRED)
    list(APPEND KUGELMATCH_GL_LIBS OpenGL::GL)
  endif()
endif()

# Generated version header (only source of truth for the string at compile time).
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp"
  @ONLY
)

# Embedded fallback copy of the fragment shader, generated from shaders/raytrace.frag
# so the two can never drift apart.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/raytrace.frag" KUGELMATCH_FRAG_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/raytrace_es.frag" KUGELMATCH_FRAG_ES_SOURCE)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/embedded_frag.inc"
  "// Generated by CMake — do not edit.\n"
  "const char* kEmbeddedFrag = R\"GLSL(${KUGELMATCH_FRAG_SOURCE})GLSL\";\n"
  "const char* kEmbeddedFragES = R\"GLSL(${KUGELMATCH_FRAG_ES_SOURCE})GLSL\";\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/raytrace.frag")

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
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(kugelmatch PRIVATE
  ${SDL2_LIBRARIES}
  ${KUGELMATCH_GL_LIBS}
  Threads::Threads
)

target_compile_options(kugelmatch PRIVATE ${SDL2_CFLAGS_OTHER})

if(WIN32)
  enable_language(RC)
  target_compile_definitions(kugelmatch PRIVATE _WIN32_WINNT=0x0601)
  set_target_properties(kugelmatch PROPERTIES WIN32_EXECUTABLE TRUE)
  if(MINGW)
    target_link_options(kugelmatch PRIVATE -static-libgcc -static-libstdc++)
  endif()
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/kugelmatch.rc.in")
    set(KUGELMATCH_ICO "${CMAKE_CURRENT_SOURCE_DIR}/data/icons/kugelmatch.ico")
    configure_file(data/kugelmatch.rc.in ${CMAKE_CURRENT_BINARY_DIR}/kugelmatch.rc @ONLY)
    target_sources(kugelmatch PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/kugelmatch.rc)
  endif()
endif()

if(EMSCRIPTEN)
  if(NOT DEFINED KUGELMATCH_GIT_REV)
    execute_process(COMMAND git rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE KUGELMATCH_GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE _git_status ERROR_QUIET)
    if(NOT _git_status EQUAL 0 OR KUGELMATCH_GIT_REV STREQUAL "")
      set(KUGELMATCH_GIT_REV "unknown")
    endif()
  endif()
  set(KUGELMATCH_SOURCE_URL "https://github.com/Grumbel/kugelmatch")
  configure_file(mk/wasm/shell.html ${CMAKE_CURRENT_BINARY_DIR}/shell.html @ONLY)
  set_target_properties(kugelmatch PROPERTIES SUFFIX ".html")
  target_link_options(kugelmatch PRIVATE
    --shell-file ${CMAKE_CURRENT_BINARY_DIR}/shell.html
    -sALLOW_MEMORY_GROWTH=1
    # Emscripten 6: growable ArrayBuffer breaks TextDecoder → blank page
    -sGROWABLE_ARRAYBUFFERS=0
    -sMAX_WEBGL_VERSION=2
    -sMIN_WEBGL_VERSION=1
    -sFULL_ES2=1
    -sFORCE_FILESYSTEM=1
    -lidbfs.js
    -sEXPORTED_RUNTIME_METHODS=FS,ccall,cwrap
    -sENVIRONMENT=web
  )
  install(FILES
    $<TARGET_FILE_DIR:kugelmatch>/kugelmatch.html
    $<TARGET_FILE_DIR:kugelmatch>/kugelmatch.js
    $<TARGET_FILE_DIR:kugelmatch>/kugelmatch.wasm
    DESTINATION .
  )
  # Consumers can copy kugelmatch.html to index.html for static hosting.
endif()

# --- Install (Linux packaging; skipped for Emscripten which installs the site above) ---
if(NOT EMSCRIPTEN AND NOT ANDROID)
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
endif()
