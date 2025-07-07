#include <assert.h>
#include <iostream>
#include <iomanip>
#include <vector>

#include <sys/mman.h>

#include "utility.h"
#include "matrix.h"

#include "basic_multiply.h"
#include "upgraded_multiply.h"


std::vector<std::pair<char const *, matrix (*)(matrix const &, matrix const &)> > const multiply_functions = {
    {"basic_multiply", basic_multiply},
    {"upgraded_multiply", upgraded_multiply}
};

int main() {
    mlockall(MCL_CURRENT | MCL_FUTURE);
    srand(time(nullptr));

    size_t const big_r = 2000;
    size_t const big_c = 2000;
    matrix big_test_matrix(big_r, big_c);
    for (size_t i = 0; i < big_r; ++i) {
        for (size_t j = 0; j < big_c; ++j) {
            big_test_matrix[i][j] = i == j ? 1 + rand() % 100 : rand() % 100;
            // big_test_matrix[i][j] = i == j ?  1 : 0;
        }
    }
    std::cout << "Generated " << big_r * big_c << " numbers\n" << std::endl;

    auto realResult = basic_multiply(big_test_matrix, big_test_matrix);

    timespec result_times[multiply_functions.size()]{};
    size_t i = -1;

    for (auto const &[owner_name, multiply_func]: multiply_functions) {
        ++i;
        if (not basic_test(multiply_func)) {
            std::cout << "Function by " << owner_name << " failed in basic test and rejected!" << std::endl;
            continue;
        } else {
            std::cout << "Function by " << owner_name << " passed basic test! Continue..." << std::endl;
        }

        timespec start_time{}, stop_time{};

        clock_gettime(CLOCK_MONOTONIC, &start_time);
        matrix result = multiply_func(big_test_matrix, big_test_matrix);
        clock_gettime(CLOCK_MONOTONIC, &stop_time);
        timespec_diff(&start_time, &stop_time, &result_times[i]);

        if (result == realResult) {
            std::cout << "Function by " << owner_name << " succeeded!" << std::endl;
        }
    }

    std::ostream result_printer(std::cout.rdbuf());
    std::cout << '\n';
    result_printer << '|' << std::setw(24) << ' ' << "RESULTS" << std::setw(24) << ' ' << '|' << std::endl;
    for (i = 0; i < multiply_functions.size(); ++i) {
        auto const &owner_name = multiply_functions[i].first;
        timespec const &result_time = result_times[i];
        result_printer << '|' << std::setw(30) << owner_name << " | ";
        if (result_time.tv_nsec == 0 and result_time.tv_nsec == result_time.tv_sec) {
            result_printer << std::setw(23) << "failed |" << std::endl;
        } else {
            result_printer << std::setw(3) << result_time.tv_sec <<
                    "sec " << std::setw(10) << result_time.tv_nsec <<
                    "nsec |" << std::endl;
        }
    }

    return 0;
}
