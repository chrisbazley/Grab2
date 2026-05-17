cmake_minimum_required(VERSION 3.11)
project(Grab2 C)

include(FetchContent)

FetchContent_Declare(CBOS GIT_REPOSITORY https://github.com/chrisbazley/CBOSLib.git GIT_TAG main)
FetchContent_Declare(CB GIT_REPOSITORY https://github.com/chrisbazley/CBLibrary.git GIT_TAG main)
FetchContent_Declare(OptionalAcornC GIT_REPOSITORY https://github.com/chrisbazley/OptionalAcornC.git GIT_TAG main)

FetchContent_MakeAvailable(CBOS CB OptionalAcornC)

set(CMAKE_C_STANDARD 99)

if(MSVC)
    # MSVC equivalent of -Wall -Wextra is /W4
    add_compile_options(/W4 /wd4200 /wd4996)
else()
    # For GCC and Clang
    add_compile_options(-Wall -Wextra -Wsign-compare -pedantic)
endif()

string(TOUPPER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME_UPPER)

if(SYSTEM_NAME_UPPER STREQUAL "RISCOS")
  # RISC OS: The only platform where it links and runs
  add_executable(Grab2App Grab2.c)
else()
  # Other platforms: Compile only (no linking) to catch errors
  add_library(Grab2App Grab2.c)
endif()

target_link_libraries(Grab2App PRIVATE
    CB
    CBOS
    OptionalAcornC
)

target_compile_definitions(Grab2App PUBLIC
    $<$<CONFIG:Debug>:DEBUG_OUTPUT>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(Grab2App PUBLIC
    $<$<CONFIG:Release>:-O3>
)
