
function(kbnetworking_build, source)
    if (DEFINED KB_NETWORKING_TEST_NAME)
        set(TEST_TARGET ${KB_NETWORKING_TEST_NAME})
    else()
        get_filename_component(TEST_TARGET ${source} NAME_WE)
    endif()

    add_executable(${TEST_TARGET} ${source})
    target_link_libraries(${TEST_TARGET} PRIVATE kb-networking)
    install(${TEST_TARGET} RUNTIME)
endfunction()

function(kbnetworking_test target)
    include(CMakeParseArguments)
    set(options)
    set(onValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(KB_NETWORKING_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED KB_NETWORKING_TEST_LABEL)
        set(KB_NETWORKING_TEST_LABEL "main")
    endif()

    if (NOT DEFINED KB_NETWORKING_WORKING_DIRECTORY)
        set(KB_NETWORKING_WORKING_DIRECTORY .)
    endif()

    if (DEFINED KB_NETWORKING_TEST_NAME)
        set(TEST_NAME ${KB_NETWORKING_TEST_NAME})
    else()
        set(TEST_NAME ${target})
    endif()

    set(TEST_TARGET ${target})

    add_test(
            NAME ${TEST_NAME}
            WORKING_DIRECTORY ${KB_NETWORKING_TEST_NAME}
            COMMAND $<TARGET_FILE:${TEST_TARGET}>
            ${KB_NETWORKING_TEST_ARGS}
    )

    set_property(TEST ${TEST_NAME} PROPERTY LABELS ${KB_NETWORKING_TEST_LABELS})
endfunction()

function(kbnetworking_test_cmd target)
    include(CMakeParseArguments)
    set(options)
    set(oneValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(KB_NETWORKING_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED KB_NETWORKING_TEST_LABEL)
        set(KB_NETWORKING_TEST_LABEL "main")
    endif()

    if (NOT DEFINED KB_NETWORKING_WORKING_DIRECTORY)
        set(KB_NETWORKING_WORKING_DIRECTORY .)
    endif()

    if (DEFINED KB_NETWORKING_TEST_NAME)
        set(TEST_NAME ${KB_NETWORKING_TEST_NAME})
    else()
        set(TEST_NAME ${target})
    endif()

    add_test(
            NAME ${TEST_NAME}
            WORKING_DIRECTORY ${KB_NETWORKING_TEST_NAME}
            COMMAND $<TARGET_FILE:${TEST_TARGET}>
            ${KB_NETWORKING_TEST_ARGS}
    )

    set_property(TEST ${TEST_NAME} PROPERTY LABELS ${KB_NETWORKING_TEST_LABELS})
endfunction()

function(kbnetworking_test_and_build source)
    include(CMakeParseArguments)
    set(options)
    set(oneValueArgs NAME LABEL WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(KB_NETWORKING_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED KB_NETWORKING_TEST_LABEL)
        set(KB_NETWORKING_TEST_LABEL "main")
    endif()

    if (NOT DEFINED KB_NETWORKING_WORKING_DIRECTORY)
        set(KB_NETWORKING_WORKING_DIRECTORY .)
    endif()

    if (DEFINED KB_NETWORKING_TEST_NAME)
        set(TEST_TARGET  ${KB_NETWORKING_TEST_NAME})
    else()
        get_filename_component(TEST_TARGET ${source} NAME_WE)
    endif()

    add_executable(${TEST_TARGET} ${source})
    install(TARGETS ${TEST_TARGET} RUNTIME)
    target_include_directories(${TEST_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src) # TODO: we shouldn't be including this
    #target_link_libraries(${TEST_TARGET} PRIVATE GameNetworkingSockets::shared)
    #target_link_libraries(${TEST_TARGET} PRIVATE spdlog::spdlog_header_only)
    #target_link_libraries(${TEST_TARGET} PRIVATE msgpack-cxx)
    target_link_libraries(${TEST_TARGET} PRIVATE kb-networking)
    #target_link_libraries(${TEST_TARGET} PUBLIC libcoro)
    target_include_directories(${TEST_TARGET} PUBLIC ${LIBCORO_INCLUDE_DIRS})
    #target_link_libraries(${TEST_TARGET} PUBLIC ${LIBCORO_LIBRARIES})

    add_test(
            NAME ${TEST_TARGET}
            WORKING_DIRECTORY ${KB_NETWORKING_TEST_NAME}
            COMMAND $<TARGET_FILE:${TEST_TARGET}>
            ${KB_NETWORKING_TEST_ARGS}
    )

    set_property(TEST ${TEST_TARGET} PROPERTY LABELS ${KB_NETWORKING_TEST_LABELS})
endfunction()

kbnetworking_test_and_build(net/test_serde.cpp)
kbnetworking_test_and_build(core/test_owning_buffer.cpp)
kbnetworking_test_and_build(udp/test_udp_server.cpp)
kbnetworking_test_and_build(udp/test_udp_c.c)
kbnetworking_test_and_build(udp/test_udp_packet_client.cpp)
kbnetworking_test_and_build(udp/test_async.cpp)

