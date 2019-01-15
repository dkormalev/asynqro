cmake_minimum_required(VERSION 3.12.0)
project(asynqro_tests LANGUAGES CXX)

configure_file(gtest_CMakeLists.txt.in googletest/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest/download
)
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest/download
)
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# defines gtest and gtest_main targets
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest/build
    EXCLUDE_FROM_ALL
)

include(GoogleTest)
add_subdirectory(impl)
add_subdirectory(future)
add_subdirectory(tasks)
