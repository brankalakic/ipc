cmake_minimum_required(VERSION 2.8)



project(Server)

add_definitions(-DUNICODE -D_UNICODE)

set(SOURCE "server.cpp" "Car.h" "Car.cpp" "BaseClass.h" "BaseClass.cpp" "../common/utils.h" "../common/utils.cpp")

add_executable(server ${SOURCE})

