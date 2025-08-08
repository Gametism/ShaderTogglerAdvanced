
cmake_minimum_required(VERSION 3.20)
project(ShaderToggler LANGUAGES CXX RC)

add_library(ShaderToggler SHARED
  ShaderToggler-1.2.1/src/Main.cpp
  ShaderToggler-1.2.1/src/ShaderManager.cpp
  ShaderToggler-1.2.1/src/ToggleGroup.cpp
  ShaderToggler-1.2.1/src/KeyData.cpp
  ShaderToggler-1.2.1/src/CDataFile.cpp
  ShaderToggler-1.2.1/src/ShaderToggler.rc
)

target_include_directories(ShaderToggler PRIVATE ShaderToggler-1.2.1/src ShaderToggler-1.2.1/src/Include)
target_compile_features(ShaderToggler PRIVATE cxx_std_17)
target_compile_definitions(ShaderToggler PRIVATE UNICODE _UNICODE)

# Windows-specific settings
if (MSVC)
  target_compile_options(ShaderToggler PRIVATE /permissive- /EHsc)
  # Disable security warnings for CRT, optional
  target_compile_definitions(ShaderToggler PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Output name as .dll, then copy/rename to .addon after build
add_custom_command(TARGET ShaderToggler POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ShaderToggler>" "$<TARGET_FILE_DIR:ShaderToggler>/ShaderToggler.addon"
  COMMENT "Copying to .addon extension"
)
