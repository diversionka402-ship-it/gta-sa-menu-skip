cmake_minimum_required(VERSION 3.15)
project(menuskip LANGUAGES CXX)

# GTA SA — 32-битный процесс, поэтому DLL должна быть x86
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "Проект нацелен на x86 (32-бит). Убедись, что CMake сконфигурирован с -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED
    dllmain.cpp
)

# Итоговый файл: menuskip.dll -> переименуй в .asi для ASI Loader'а
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "menuskip"
    PREFIX ""
)
