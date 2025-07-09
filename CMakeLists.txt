cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (WIN32)
  set(CMAKE_CXX_FLAGS "/utf-8")
  set(CMAKE_CXX_FLAGS "/EHsc")
  set(CMAKE_C_FLAGS "/utf-8")
  set(CMAKE_C_FLAGS "/EHsc")
else() 
  set(CMAKE_CXX_FLAGS "-fPIC")
endif()
add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
add_compile_definitions(_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

project(GAME_BASE VERSION 0.0.1 DESCRIPTION "raylib game base" LANGUAGES CXX C)

add_subdirectory(game)

add_custom_command(OUTPUT res_icon.c COMMAND embedfile res_icon ${GAME_BASE_SOURCE_DIR}/res/icon.png DEPENDS ${GAME_BASE_SOURCE_DIR}/res/icon.png)

# TARGET
if (GAME_BASE_SHARED_BUILD)
  set(GAME_BASE_SOURCE_FILES    
    "src/base.cpp"
    res_icon.c
  )
  add_executable(GAME_BASE ${GAME_BASE_SOURCE_FILES})
else()
  set(GAME_PURE_SOURCE_FILES    
    "src/pure.cpp"
    res_icon.c
  )
  add_executable(GAME_PURE ${GAME_PURE_SOURCE_FILES})
endif()

# DEPENDENCIES
set(RAYLIB_VERSION 5.5.4)
if (GAME_BASE_SHARED_BUILD)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Build raylib as shared library" FORCE)
endif()
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  set(CMAKE_TLS_VERIFY FALSE)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/KraftJrIvo/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    URL_HASH SHA512=0f24c24108a75b5f4e82de38e814f17220823bd1e578369eee4122f323e491dc4a607d192c21935170035a22b23d0f4a5f197486758be434c6f80759f1356b80
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()
target_compile_options(raylib PUBLIC -DRAYMATH_DISABLE_CPP_OPERATORS)
target_compile_options(raylib PUBLIC -DMANUAL_INPUT_EVENTS_POLLING)
target_compile_options(raylib PUBLIC -DGRAPHICS_API_OPENGL_33)

if (GAME_BASE_SHARED_BUILD)
  set(DYLIB_VERSION v2.2.1)
  find_package(dylib QUIET)
  if (NOT dylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        dylib
        GIT_REPOSITORY "https://github.com/martin-olivier/dylib"
        GIT_TAG        "v2.2.1"
    )
    FetchContent_MakeAvailable(dylib)
  endif()
  target_link_libraries(GAME_BASE PUBLIC GAME_NEW raylib dylib)
  add_custom_command(TARGET GAME_BASE
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/_deps/raylib-build/raylib/raylib.dll
        ${CMAKE_CURRENT_BINARY_DIR}/raylib.dll
  )
else()
  target_link_libraries(GAME_PURE PUBLIC GAME raylib)
endif()