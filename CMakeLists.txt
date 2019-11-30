cmake_minimum_required(VERSION 3.12.0)
project(asynqro LANGUAGES CXX VERSION 0.6.0)
include(GenerateExportHeader)

option(BUILD_SHARED_LIBS
    "Build asynqro as shared library"
    OFF
)

option(ASYNQRO_QT_SUPPORT
    "Build asynqro with Qt support"
    OFF
)

option(ASYNQRO_BUILD_TESTS
    "Build asynqro tests. Will download googletest from github during configure time and build it with tests"
    OFF
)

option(ASYNQRO_BUILD_WITH_GCOV
    "Build asynqro with gcov support. Enables ASYNQRO_BUILD_TESTS"
    OFF
)

option(ASYNQRO_BUILD_WITH_DUMMY
    "Build asynqro with dummy TU for better static analysis coverage"
    OFF
)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
endif()

if (BUILD_SHARED_LIBS)
    message("-- asynqro: Building as shared library")
else()
    message("-- asynqro: Building as static library")
endif()

add_library(asynqro
    src/future.cpp
    src/failure_handling.cpp
    src/tasksdispatcher.cpp

    include/asynqro/asynqro
    include/asynqro/future.h
    include/asynqro/simplefuture.h
    include/asynqro/tasks.h
    include/asynqro/repeat.h
    include/asynqro/impl/promise.h
    include/asynqro/impl/cancelablefuture.h
    include/asynqro/impl/failure_handling.h
    include/asynqro/impl/spinlock.h
    include/asynqro/impl/typetraits.h
    include/asynqro/impl/zipfutures.h
    include/asynqro/impl/containers_helpers.h
    include/asynqro/impl/containers_traverse.h
    include/asynqro/impl/tasksdispatcher.h
    include/asynqro/impl/taskslist_p.h
)

if (ASYNQRO_BUILD_WITH_GCOV)
    message("-- asynqro: Building with gcov support")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline -fkeep-static-functions -fno-elide-constructors -fprofile-update=atomic")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
    set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
    set(ASYNQRO_BUILD_TESTS ON)
    target_compile_definitions(asynqro PUBLIC ASYNQRO_GCOV_ENABLED)
endif()

if(ASYNQRO_BUILD_WITH_DUMMY)
    message("-- asynqro: Building with dummy TU")
    target_sources(asynqro PRIVATE src/extra/dummy.cpp)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(asynqro PUBLIC Threads::Threads)

add_library(asynqro::asynqro ALIAS asynqro)
generate_export_header(asynqro EXPORT_FILE_NAME generated_include/asynqro/impl/asynqro_export.h)

set_target_properties(asynqro PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)

if (ASYNQRO_QT_SUPPORT)
    message("-- asynqro: Building with Qt support")
    find_package(Qt5Core CONFIG REQUIRED)
    find_package(Qt5Concurrent CONFIG REQUIRED)
    target_link_libraries(asynqro INTERFACE Qt5::Core Qt5::Concurrent)
    target_compile_definitions(asynqro INTERFACE ASYNQRO_QT_SUPPORT)
endif()

if(MSVC)
    target_compile_options(asynqro PUBLIC /wd4251)
endif()

target_include_directories(asynqro PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated_include>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/asynqro
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*_p.h" EXCLUDE
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/asynqro/asynqro
    DESTINATION include/asynqro
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include/asynqro
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*_p.h" EXCLUDE
)

install(TARGETS asynqro
   EXPORT asynqro-targets
   RUNTIME DESTINATION lib
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

install(EXPORT asynqro-targets DESTINATION lib/cmake/asynqro
   NAMESPACE asynqro::
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/asynqro-config.cmake
   DESTINATION lib/cmake/asynqro
)

export(EXPORT asynqro-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake NAMESPACE asynqro::)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/asynqro-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/asynqro-config.cmake
    INSTALL_DESTINATION lib/cmake/asynqro
)

if(ASYNQRO_BUILD_TESTS)
    message("-- asynqro: Building with tests")
    enable_testing()
    add_subdirectory(tests)
endif()
