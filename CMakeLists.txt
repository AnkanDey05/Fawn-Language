cmake_minimum_required(VERSION 3.20)

project(FawnLang VERSION 1.4.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FAWN_BUILD_WARNINGS "Enable warnings" ON)

file(GLOB_RECURSE FAWN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# --- WINDOWS ICON LOGIC ---
if (WIN32)
    # Point CMake to the 'res' folder
    list(APPEND FAWN_SOURCES "${CMAKE_SOURCE_DIR}/res/resource.rc")
endif()
# --------------------------

add_executable(fawn ${FAWN_SOURCES})

target_include_directories(fawn PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

if (MSVC)
    target_compile_options(fawn PRIVATE /W4 /permissive- /O2 /Oi /Ot /GL)
    target_link_options(fawn PRIVATE /LTCG)
else()
    target_compile_options(fawn PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
endif()

if (FAWN_BUILD_WARNINGS)
    message(STATUS "Warnings enabled")
else()
    target_compile_options(fawn PRIVATE -Wall -Wextra -Wpedantic -O3 -flto)
    target_link_options(fawn PRIVATE -flto)

endif()