include(CMakeFindDependencyMacro)

if(NOT TARGET asynqro::asynqro)
    include("${CMAKE_CURRENT_LIST_DIR}/asynqro-targets.cmake")
endif()
