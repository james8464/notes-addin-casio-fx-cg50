cmake_minimum_required(VERSION 3.15)

# fxSDK entrypoint.
# This project is intentionally minimal; fxSDK injects the toolchain config when
# you run `fxsdk build-cg`.

project(CasioCAS LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CasioCAS
  src/main.cpp
  src/device/device_solver.cpp
  src/ui/text_input_device.cpp
  src/ui/menu.cpp
)

# gint is the default runtime for fxSDK add-ins; keep it explicit so the intent
# is obvious when reading this file outside fxSDK docs.
find_package(Gint 2.1 REQUIRED)
target_link_libraries(CasioCAS Gint::Gint)

target_include_directories(CasioCAS PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(CasioCAS PRIVATE
  CASIO_DEVICE=1
)

target_compile_options(CasioCAS PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  -fno-exceptions
  -fno-rtti
)

# Generate the .g3a container.
# Note: icons/metadata will be wired once we add `assets/` and decide names.
include(GenerateG3A)
generate_g3a(TARGET CasioCAS
  # Empty NAME avoids over-icon text; set to non-empty to show a label.
  NAME ""
  VERSION "00.01.0000"
)
