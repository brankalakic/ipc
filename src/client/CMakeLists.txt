cmake_minimum_required(VERSION 2.8)



project(Client)

add_definitions(-DUNICODE -D_UNICODE)

set(SOURCE "client.cpp" "../common/utils.h" "../common/utils.cpp")

add_executable(client ${SOURCE})
