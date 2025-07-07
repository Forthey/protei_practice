perf stat -e cache-references,cache-misses ./bin/matrix_multiply_test
perf report