# ~~~
# Copyright (C) 2025-2026 Crypto Lab Inc.
# - This file is part of HEaven homomorphic encryption library.
# - HEaven cannot be copied and/or distributed without the express
#   permission of Crypto Lab Inc.
# ~~~

cmake_minimum_required(VERSION 3.23)
project(
  Zn-multiplication
  VERSION 0.1.0
  DESCRIPTION "Zn-multiplication benchmark based on HEaaN2"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/install")
endif()

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target/release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

find_package(HEaaN2 REQUIRED)
find_package(OpenMP REQUIRED)

set(SRCS
    # cmake-format: sortable
    src/WordArithConstantCache.cpp
    src/WordEncoder.cpp
    CACHE FILEPATH "Zn-multiplication Sources" FORCE)

add_library(Zn-multiplication SHARED ${SRCS})

target_link_libraries(Zn-multiplication PUBLIC HEaaN2::HEaaN2)
target_include_directories(Zn-multiplication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(Zn-multiplication PUBLIC OpenMP::OpenMP_CXX)

macro(add_target executable_name_ dir_ name_)
  add_executable(${executable_name_} ${dir_}/${name_}.cpp)
  target_link_libraries(${executable_name_} PRIVATE Zn-multiplication)
endmacro()

add_target(client_preprocess_input . client_preprocess_input)
add_target(client_key_generation . client_key_generation)
add_target(client_encode_encrypt_input . client_encode_encrypt_input)
add_target(server_encrypted_compute . server_encrypted_compute)
add_target(client_decrypt_decode . client_decrypt_decode)
add_target(client_postprocess . client_postprocess)
