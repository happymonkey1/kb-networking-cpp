cmake_minimum_required(VERSION 3.10)

project(kb-networking-cpp)

#find_package(fmt CONFIG REQUIRED)
#find_package(gamenetworkingsockets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(kb-networking-cpp src/entrypoint.cpp
        src/kb/core/core.h
        src/kb/core/logger.h
        src/kb/core/logger.cpp)

#target_link_libraries(kb-networking-cpp PRIVATE fmt::fmt)
#target_link_libraries(kb-networking-cpp PRIVATE gamenetworkingsockets::gamenetworkingsockets)
target_link_libraries(kb-networking-cpp PRIVATE spdlog::spdlog)