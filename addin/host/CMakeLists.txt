cmake_minimum_required(VERSION 3.15)
project(CasioCAS_Host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(casio_host
  main.cpp
  ../src/modules/boolean/boolean.cpp
  ../src/modules/suvat/suvat.cpp
  ../src/modules/integrate/integrate.cpp
  ../src/modules/algebra/algebra.cpp
  ../src/modules/trig/trig.cpp
  ../src/core/rational.cpp
  ../src/core/arena.cpp
  ../src/core/normalize.cpp
  ../src/core/simplify.cpp
  ../src/core/tokenize.cpp
  ../src/core/parse.cpp
  ../src/core/format_expr.cpp
  ../src/core/format_exam.cpp
)

target_include_directories(casio_host PRIVATE
  ../src
)

target_compile_options(casio_host PRIVATE -Wall -Wextra -Wno-unused-parameter)

