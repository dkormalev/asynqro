cmake_minimum_required(VERSION 3.12.0)
project(asynqro_tasks_tests LANGUAGES CXX)

set(TASKS_TESTS_SOURCES
    tasks_clustered_test.cpp
    tasks_exceptions_test.cpp
    tasks_sequence_test.cpp
    tasks_test.cpp
    tasks_threadbound_test.cpp
    tasksbasetest.h
)

add_executable(asynqro_tasks_tests main.cpp ${TASKS_TESTS_SOURCES})
set_target_properties(asynqro_tasks_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)
target_link_libraries(asynqro_tasks_tests asynqro::asynqro gtest)
gtest_discover_tests(asynqro_tasks_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)

add_executable(asynqro_tasks_preheated_tests main_preheated.cpp ${TASKS_TESTS_SOURCES})
set_target_properties(asynqro_tasks_preheated_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(asynqro_tasks_preheated_tests asynqro::asynqro gtest)
gtest_discover_tests(asynqro_tasks_preheated_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)

add_executable(asynqro_tasks_preheated_intensive_tests main_preheated_intensive.cpp ${TASKS_TESTS_SOURCES})
set_target_properties(asynqro_tasks_preheated_intensive_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)
target_link_libraries(asynqro_tasks_preheated_intensive_tests asynqro::asynqro gtest)
gtest_discover_tests(asynqro_tasks_preheated_intensive_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)

if (ASYNQRO_QT_SUPPORT)
    find_package(Qt5Core CONFIG REQUIRED)
    target_link_libraries(asynqro_tasks_tests Qt5::Core)
    target_compile_definitions(asynqro_tasks_tests PRIVATE ASYNQRO_QT_SUPPORT)
    target_link_libraries(asynqro_tasks_preheated_tests Qt5::Core)
    target_compile_definitions(asynqro_tasks_preheated_tests PRIVATE ASYNQRO_QT_SUPPORT)
    target_link_libraries(asynqro_tasks_preheated_intensive_tests Qt5::Core)
    target_compile_definitions(asynqro_tasks_preheated_intensive_tests PRIVATE ASYNQRO_QT_SUPPORT)
endif()

