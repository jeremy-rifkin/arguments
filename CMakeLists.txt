cmake_minimum_required(VERSION 3.14)

project(
  agruments
  LANGUAGES C CXX
)

add_executable(
  scratch
  scratch.cpp
)

set(
  warning_options
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror=return-type -Wundef>
  $<$<CXX_COMPILER_ID:GNU>:-Wuseless-cast -Wmaybe-uninitialized>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

target_compile_options(
  scratch
  PRIVATE
  ${warning_options}
)

target_compile_features(
  scratch
  PRIVATE cxx_std_23
)

target_include_directories(
  scratch
  PRIVATE ./
)
