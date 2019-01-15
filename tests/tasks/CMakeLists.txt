cmake_minimum_required(VERSION 3.12.0)
project(asynqro_tasks_tests LANGUAGES CXX)

find_package(Qt5Core CONFIG REQUIRED)

add_executable(asynqro_tasks_tests
    main.cpp
    tasks_clustered_test.cpp
    tasks_exceptions_test.cpp
    tasks_sequence_test.cpp
    tasks_test.cpp
    tasks_threadbound_test.cpp
    tasksbasetest.h
)
set_target_properties(asynqro_tasks_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)
target_link_libraries(asynqro_tasks_tests asynqro::asynqro gtest)
gtest_discover_tests(asynqro_tasks_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
