cmake_minimum_required(VERSION 3.10)

project(kb-networking C CXX)
include(CheckIncludeFileCXX)

set(CMAKE_CXX_STANDARD 20)

find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(msgpack-cxx REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(KB_NETWORKING_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/kb/kb_networking.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/kb/kb_networking_cpp.hpp
)

if (MINGW)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)
else()
    set(BUILD_SHARED_LIBS_DEFAULT OFF)
endif()

option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})

add_executable(kb-networking-cli src/entrypoint.cpp
        src/kb/core/logger.cpp
        include/kb/kb_networking.h
        include/kb/kb_networking_cpp.hpp
        include/kb/kb_core.h
        src/kb/kb_core.cpp
        src/kb/core/logger.hpp
        src/kb/kb_core.cpp
        src/kb/net/server.hpp
        src/kb/types.h
        include/kb/net/kb_server.h
        src/kb/net/kb_server.cpp
        include/kb/net/kb_client_info.h
        src/kb/net/server.cpp
)

target_include_directories(kb-networking-cli PUBLIC include)
target_include_directories(kb-networking-cli PRIVATE src)
target_compile_features   (kb-networking-cli PRIVATE cxx_std_20)

target_link_libraries(kb-networking-cli PRIVATE GameNetworkingSockets::shared)
target_link_libraries(kb-networking-cli PRIVATE spdlog::spdlog_header_only)
target_link_libraries(kb-networking-cli PRIVATE msgpack-cxx)

add_library(kb-networking
        src/entrypoint.cpp
        src/kb/core/logger.cpp
        include/kb/kb_networking.h
        include/kb/kb_networking_cpp.hpp
        include/kb/kb_core.h
        src/kb/kb_core.cpp
        src/kb/core/logger.hpp
        src/kb/kb_core.cpp
        src/kb/net/server.hpp
        src/kb/types.h
        include/kb/net/kb_server.h
        src/kb/net/kb_server.cpp
        include/kb/net/kb_client_info.h
        src/kb/net/server.cpp
)

target_include_directories(kb-networking PUBLIC include)
target_include_directories(kb-networking PRIVATE src)
target_compile_features   (kb-networking PRIVATE cxx_std_20)

target_link_libraries(kb-networking PRIVATE GameNetworkingSockets::shared)
target_link_libraries(kb-networking PRIVATE spdlog::spdlog_header_only)
target_link_libraries(kb-networking PRIVATE msgpack-cxx)

if (BUILD_SHARED_LIBS)
    set_target_properties(kb-networking PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(kb-networking PRIVATE KB_NETWORKING_BUILD)
    target_compile_definitions(kb-networking PUBLIC  KB_NETWORKING_SHARED)
endif()

set_target_properties(kb-networking-cli kb-networking
        PROPERTIES
        PUBLIC_HEADER "${LLAMA_PUBLIC_HEADERS}")