cmake_minimum_required(VERSION 3.12.0)
project(asynqro-benchmarks LANGUAGES CXX)

find_package(asynqro REQUIRED)
find_package(Qt5Concurrent REQUIRED)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_DEBUG_LIBS         OFF)
set(Boost_USE_RELEASE_LIBS       ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ThreadPoolCpp_INCLUDE_DIRS})

set(EMPTY_JOBS_COUNT 100000 1000000)
set(TIMED_JOBS_COUNT 10000 100000)
set(TIMED_JOB_LENGTH 100000)
set(REPOST_CONCURRENCY 1 2 4 6 8 10 12 14 16 32)
set(ASYNQRO_TASK_MODE 1 2)
set(ASYNQRO_USE_FEATURES 0 1)
set(ASYNQRO_IDLE_LOOPS_AMOUNT 1 100 1000 100000)

set(NON_ASYNQRO_SYSTEMS boostasio threadpoolcpp qtconcurrent)

add_definitions(-DCORES=8)

set(ALL_BENCHMARKS)

## empty repost
foreach(job_count ${EMPTY_JOBS_COUNT})
    foreach(concurrency ${REPOST_CONCURRENCY})
        foreach(system ${NON_ASYNQRO_SYSTEMS})
            add_executable(${system}_c${concurrency}_j${job_count}_empty_repost empty-repost/${system}.cpp)
            target_compile_definitions(${system}_c${concurrency}_j${job_count}_empty_repost PRIVATE
                "CONCURRENCY=${concurrency}"
                "JOBS_COUNT=${job_count}"
                )
            target_link_libraries(${system}_c${concurrency}_j${job_count}_empty_repost ${Boost_LIBRARIES} Qt5::Concurrent)
            set(ALL_BENCHMARKS ${ALL_BENCHMARKS} ${system}_c${concurrency}_j${job_count}_empty_repost)
        endforeach()

        foreach(task_mode ${ASYNQRO_TASK_MODE})
            foreach(use_features ${ASYNQRO_USE_FEATURES})
                foreach(idle_loops ${ASYNQRO_IDLE_LOOPS_AMOUNT})
                    add_executable(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_empty_repost empty-repost/asynqro.cpp)
                    target_compile_definitions(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_empty_repost PRIVATE
                        "CONCURRENCY=${concurrency}"
                        "JOBS_COUNT=${job_count}"
                        "TASK_MODE=${task_mode}"
                        "WITH_FUTURES=${use_features}"
                        "WITH_PREHEAT=1"
                        "IDLE_AMOUNT=${idle_loops}"
                        )
                    target_link_libraries(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_empty_repost asynqro::asynqro)
                    set(ALL_BENCHMARKS ${ALL_BENCHMARKS} asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_empty_repost)
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()

## timed repost
foreach(job_count ${TIMED_JOBS_COUNT})
    foreach(job_length ${TIMED_JOB_LENGTH})
        foreach(concurrency ${REPOST_CONCURRENCY})
            foreach(system ${NON_ASYNQRO_SYSTEMS})
                add_executable(${system}_c${concurrency}_j${job_count}_l${job_length}_timed_repost timed-repost/${system}.cpp)
                target_compile_definitions(${system}_c${concurrency}_j${job_count}_l${job_length}_timed_repost PRIVATE
                    "CONCURRENCY=${concurrency}"
                    "JOBS_COUNT=${job_count}"
                    "JOB_LENGTH=${job_length}"
                    )
                target_link_libraries(${system}_c${concurrency}_j${job_count}_l${job_length}_timed_repost ${Boost_LIBRARIES} Qt5::Concurrent)
                set(ALL_BENCHMARKS ${ALL_BENCHMARKS} ${system}_c${concurrency}_j${job_count}_l${job_length}_timed_repost)
            endforeach()

            foreach(task_mode ${ASYNQRO_TASK_MODE})
                foreach(use_features ${ASYNQRO_USE_FEATURES})
                    foreach(idle_loops ${ASYNQRO_IDLE_LOOPS_AMOUNT})
                        add_executable(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_l${job_length}_timed_repost timed-repost/asynqro.cpp)
                        target_compile_definitions(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_l${job_length}_timed_repost PRIVATE
                            "CONCURRENCY=${concurrency}"
                            "JOBS_COUNT=${job_count}"
                            "JOB_LENGTH=${job_length}"
                            "TASK_MODE=${task_mode}"
                            "WITH_FUTURES=${use_features}"
                            "WITH_PREHEAT=1"
                            "IDLE_AMOUNT=${idle_loops}"
                            )
                        target_link_libraries(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_l${job_length}_timed_repost asynqro::asynqro)
                        set(ALL_BENCHMARKS ${ALL_BENCHMARKS} asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_c${concurrency}_j${job_count}_l${job_length}_timed_repost)
                    endforeach()
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()




# empty avalanche
foreach(job_count ${EMPTY_JOBS_COUNT})
    foreach(system ${NON_ASYNQRO_SYSTEMS})
        add_executable(${system}_j${job_count}_empty_avalanche empty-avalanche/${system}.cpp)
        target_compile_definitions(${system}_j${job_count}_empty_avalanche PRIVATE
            "JOBS_COUNT=${job_count}"
            )
        target_link_libraries(${system}_j${job_count}_empty_avalanche ${Boost_LIBRARIES} Qt5::Concurrent)
        set(ALL_BENCHMARKS ${ALL_BENCHMARKS} ${system}_j${job_count}_empty_avalanche)
    endforeach()

    foreach(task_mode ${ASYNQRO_TASK_MODE})
        foreach(use_features ${ASYNQRO_USE_FEATURES})
            foreach(idle_loops ${ASYNQRO_IDLE_LOOPS_AMOUNT})
                add_executable(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_empty_avalanche empty-avalanche/asynqro.cpp)
                target_compile_definitions(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_empty_avalanche PRIVATE
                    "JOBS_COUNT=${job_count}"
                    "TASK_MODE=${task_mode}"
                    "WITH_FUTURES=${use_features}"
                    "WITH_PREHEAT=1"
                    "IDLE_AMOUNT=${idle_loops}"
                    )
                target_link_libraries(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_empty_avalanche asynqro::asynqro)
                set(ALL_BENCHMARKS ${ALL_BENCHMARKS} asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_empty_avalanche)
            endforeach()
        endforeach()
    endforeach()
endforeach()

# timed avalanche
foreach(job_count ${TIMED_JOBS_COUNT})
    foreach(job_length ${TIMED_JOB_LENGTH})
        foreach(system ${NON_ASYNQRO_SYSTEMS})
            add_executable(${system}_j${job_count}_l${job_length}_timed_avalanche timed-avalanche/${system}.cpp)
            target_compile_definitions(${system}_j${job_count}_l${job_length}_timed_avalanche PRIVATE
                "JOBS_COUNT=${job_count}"
                "JOB_LENGTH=${job_length}"
                )
            target_link_libraries(${system}_j${job_count}_l${job_length}_timed_avalanche ${Boost_LIBRARIES}  Qt5::Concurrent)
            set(ALL_BENCHMARKS ${ALL_BENCHMARKS} ${system}_j${job_count}_l${job_length}_timed_avalanche)
        endforeach()

        foreach(task_mode ${ASYNQRO_TASK_MODE})
            foreach(use_features ${ASYNQRO_USE_FEATURES})
                foreach(idle_loops ${ASYNQRO_IDLE_LOOPS_AMOUNT})
                    add_executable(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_l${job_length}_timed_avalanche timed-avalanche/asynqro.cpp)
                    target_compile_definitions(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_l${job_length}_timed_avalanche PRIVATE
                        "JOBS_COUNT=${job_count}"
                        "JOB_LENGTH=${job_length}"
                        "TASK_MODE=${task_mode}"
                        "WITH_FUTURES=${use_features}"
                        "WITH_PREHEAT=1"
                        "IDLE_AMOUNT=${idle_loops}"
                        )
                    target_link_libraries(asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_l${job_length}_timed_avalanche asynqro::asynqro)
                    set(ALL_BENCHMARKS ${ALL_BENCHMARKS} asynqro_f${use_features}_t${task_mode}_ph${preheat}_il${idle_loops}_j${job_count}_l${job_length}_timed_avalanche)
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()

foreach(target ${ALL_BENCHMARKS})
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        AUTOMOC ON
        )
endforeach()
