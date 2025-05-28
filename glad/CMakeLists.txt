cmake_minimum_required(VERSION 3.26)
project(Glad)

add_library(glad include/glad/gles2.h src/gles2.c)
target_include_directories(glad PUBLIC include/)
