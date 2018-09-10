cmake_minimum_required(VERSION 3.3)
project(myhttpd C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror -O2 -pthread")
add_executable(myhttpd main.c config.c config.h handle.c handle.h mmanage.c mmanage.h handle_core.h my_string.c my_string.h handle_read.c handle_read.h http_response.c http_response.h handle_write.c handle_write.h)
