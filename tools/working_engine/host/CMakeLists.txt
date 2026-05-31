cmake_minimum_required(VERSION 3.15)
project(CasioCAS_Host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(casio_host
  main.cpp
  ../../../khicas/upstream/giac90_1addin/cascas_working.cc
)

target_compile_definitions(casio_host PRIVATE CASCAS_HOST_STD_STRING=1 CASCAS_DISABLE_GOLDEN_QUEUE=1)
target_compile_options(casio_host PRIVATE -Wall -Wextra -Wno-unused-parameter)
