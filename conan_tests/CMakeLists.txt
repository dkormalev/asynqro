cmake_minimum_required(VERSION 3.12.0)
project(AsynqroPackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(asynqro_tester asynqro_tester.cpp)
target_link_libraries(asynqro_tester CONAN_PKG::asynqro)

set_target_properties(asynqro_tester PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
