cmake_minimum_required(VERSION 3.0.0)
project(jLib VERSION 1.0.0)

set(
  PROG_SRCS

  src/jLib/jLib.c

  src/main.c
)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} ${PROG_SRCS})
target_link_libraries(${PROJECT_NAME})
