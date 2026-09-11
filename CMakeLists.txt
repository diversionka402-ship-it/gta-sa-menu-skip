cmake_minimum_required(VERSION 3.15)
project(gta_sa_menu_dll LANGUAGES CXX)

# GTA SA — 32-битный процесс, поэтому DLL должна быть x86
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "Проект нацелен на x86 (32-бит), а конфигурация сейчас не 32-битная. Убедись, что генератор CMake настроен на Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Имя итоговой DLL
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "menuskip"
    PREFIX ""
)
