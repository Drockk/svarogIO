add_executable(svarog_benchmarks
    execution/work_queue_bench.cpp
)
target_link_libraries(svarog_benchmarks PRIVATE
    svarog
    Catch2::Catch2WithMain
)
target_compile_options(svarog_benchmarks PRIVATE -O3 -march=native)

# Enable benchmarking
target_compile_definitions(svarog_benchmarks PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING)

add_test(NAME svarog_benchmarks COMMAND svarog_benchmarks)
