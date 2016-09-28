cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(hs_leader VERSION 1.0 LANGUAGES CXX C)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(hs_leader
  main.cc
  HSAlgo.cc
  HSAlgo.h
  utils/log_util.h
  utils/message.h
  utils/message_channel.h)

target_include_directories(hs_leader PRIVATE utils)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(hs_leader PRIVATE -std=c++0x)
  target_link_libraries(hs_leader PRIVATE Threads::Threads)
endif()
