cmake_minimum_required(VERSION 3.12.0)
project(asynqro_future_tests LANGUAGES CXX)

add_executable(asynqro_future_tests
    main.cpp
    future_basics_test.cpp
    common_future_callbacks_test.cpp
    future_callbacks_test.cpp
    cancelablefuture_callbacks_test.cpp
    common_future_morphisms_test.cpp
    future_morphisms_test.cpp
    cancelablefuture_morphisms_test.cpp
    common_future_inner_morphisms_test.cpp
    future_inner_morphisms_test.cpp
    cancelablefuture_inner_morphisms_test.cpp
    common_future_zip_test.cpp
    future_zip_test.cpp
    cancelablefuture_zip_test.cpp
    future_sequence_test.cpp
    future_sequence_with_failures_test.cpp
    future_failure_test.cpp
    future_exceptions_test.cpp
    futurebasetest.h
    copycountcontainers.h
)

set_source_files_properties(
    common_future_callbacks_test.cpp
    common_future_morphisms_test.cpp
    common_future_inner_morphisms_test.cpp
    common_future_zip_test.cpp
    PROPERTIES HEADER_FILE_ONLY ON
)

set_target_properties(asynqro_future_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if (ASYNQRO_QT_SUPPORT)
    set_target_properties(asynqro_future_tests PROPERTIES AUTOMOC ON)
endif()

target_link_libraries(asynqro_future_tests asynqro::asynqro gtest)

if (NOT DEFINED ENV{APPVEYOR})
    gtest_discover_tests(asynqro_future_tests DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
endif()
