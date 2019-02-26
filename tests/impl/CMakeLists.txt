cmake_minimum_required(VERSION 3.12.0)
project(asynqro_impl_tests LANGUAGES CXX)


add_executable(asynqro_impl_tests
    main.cpp
    containers_traverse_test.cpp
    containers_traverse_flatten_test.cpp
    containers_traverse_reduce_test.cpp
    containers_traverse_map_one_socket_test.cpp
    containers_traverse_map_two_sockets_test.cpp
    taskslist_test.cpp
)
set_target_properties(asynqro_impl_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(asynqro_impl_tests asynqro::asynqro gtest)
if (ASYNQRO_QT_SUPPORT)
    target_sources(asynqro_impl_tests PRIVATE containers_traverse_map_no_sockets_test.cpp)
    find_package(Qt5Core CONFIG REQUIRED)
    target_link_libraries(asynqro_impl_tests Qt5::Core)
    target_compile_definitions(asynqro_impl_tests PRIVATE ASYNQRO_QT_SUPPORT)
endif()

gtest_discover_tests(asynqro_impl_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
