cmake_minimum_required(VERSION 3.10)

project(kb-networking C CXX)
include(CheckIncludeFileCXX)

set(CMAKE_CXX_STANDARD 20)

find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(msgpack-cxx REQUIRED)
find_package(fmt REQUIRED)
#find_package(libcoro CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCORO REQUIRED libcoro)
link_directories(${LIBCORO_LIBRARY_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(KB_NETWORKING_STANDALONE ON)

    # configure project version
    # TODO
else()
    set(KB_NETWORKING_STANDALONE OFF)
endif()

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

option(KB_NETWORKING_BUILD_TESTS "kb-networking: build tests" ${KB_NETWORKING_STANDALONE})

add_library(kb-networking
        src/kb/core/logger.cpp
        include/kb/kb_networking.h
        include/kb/kb_networking_cpp.hpp
        include/kb/kb_core.h
        src/kb/kb_core.cpp
        include/kb/core/logger.hpp
        src/kb/kb_core.cpp
        src/kb/net/server.hpp
        src/kb/types.h
        include/kb/net/kb_server.h
        src/kb/net/kb_server.cpp
        include/kb/net/kb_client_info.h
        src/kb/net/server.cpp
        src/kb/kb_networking.cpp
        include/kb/async/awaitable.hpp
        src/kb/net/client.hpp
        src/kb/net/packet.hpp
        src/kb/net/client.cpp
        src/kb/net/network_utils.h
        src/kb/net/network_utils.cpp
)

target_include_directories(kb-networking PUBLIC include)
target_include_directories(kb-networking PRIVATE src)
target_compile_features   (kb-networking PRIVATE cxx_std_20)

target_link_libraries(kb-networking PRIVATE GameNetworkingSockets::shared)
target_link_libraries(kb-networking PRIVATE spdlog::spdlog_header_only)
target_link_libraries(kb-networking PRIVATE msgpack-cxx)
target_link_libraries(kb-networking PRIVATE fmt::fmt)
#target_link_libraries(kb-networking PUBLIC libcoro)
target_include_directories(kb-networking PUBLIC ${LIBCORO_INCLUDE_DIRS})
target_link_libraries(kb-networking PUBLIC ${LIBCORO_LIBRARIES})

if (BUILD_SHARED_LIBS)
    set_target_properties(kb-networking PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(kb-networking PRIVATE KB_NETWORKING_BUILD)
    target_compile_definitions(kb-networking PUBLIC  KB_NETWORKING_SHARED)
endif()

set_target_properties(kb-networking
        PROPERTIES
        PUBLIC_HEADER "${KB_NETWORKING_PUBLIC_HEADERS}")

if (KB_NETWORKING_BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()


# TODO: copy vcpkg.json from root into vcpkg-overlays/kbnetworking/