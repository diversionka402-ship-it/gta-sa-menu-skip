cmake_minimum_required(VERSION 3.15)
project(menuskip LANGUAGES C CXX)

# GTA SA — 32-битный процесс, поэтому DLL должна быть x86
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "Проект нацелен на x86 (32-бит). Убедись, что CMake сконфигурирован с -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- MinHook: тянем исходники напрямую, собираем сами (без опоры на чужой CMakeLists) ---
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.4
)
FetchContent_GetProperties(minhook)
if(NOT minhook_POPULATED)
    FetchContent_Populate(minhook)
endif()

add_library(minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde32.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c
)
target_include_directories(minhook PUBLIC
    ${minhook_SOURCE_DIR}/include
    ${minhook_SOURCE_DIR}/src
)

add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE minhook)

# Итоговый файл: menuskip.dll -> переименуй в .asi для ASI Loader'а
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "menuskip"
    PREFIX ""
)
