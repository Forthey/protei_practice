#ifndef EVALUATE_CHECK_H
#define EVALUATE_CHECK_H

#include <vector>
#include <string>
#include <sstream>
#include <optional>
#include <random>


class ExprGenerator {
    std::optional<int> seed;
    std::mt19937 gen;

public:
    explicit ExprGenerator(std::optional<int> seed = std::nullopt) : seed(seed) {
        if (seed.has_value()) {
            gen = std::mt19937(seed.value());
        } else {
            gen = std::mt19937(std::random_device()());
        }
    }

    static long evaluate_check(const std::string &expr) {
        std::vector<long> nums;
        std::vector<char> ops;
        std::istringstream ss(expr);
        long num;
        char op;

        ss >> num;
        nums.push_back(num);
        while (ss >> op >> num) {
            ops.push_back(op);
            nums.push_back(num);
        }
        for (std::size_t i = 0; i < ops.size();) {
            if (ops[i] == '*' || ops[i] == '/') {
                long const a = nums[i], b = nums[i + 1];
                long const res = (ops[i] == '*') ? a * b : a / b;

                nums[i] = res;
                nums.erase(std::next(nums.begin(), i + 1ull));
                ops.erase(std::next(ops.begin(), i));
            } else ++i;
        }
        long result = nums[0];
        for (size_t i = 0; i < ops.size(); ++i) {
            if (ops[i] == '+') result += nums[i + 1];
            else result -= nums[i + 1];
        }
        return result;
    }

    std::string gen_expr(int n) {
        std::uniform_int_distribution<> num_dist(1, 100);
        std::uniform_int_distribution<> op_dist(0, 3);

        constexpr char ops[] = {'+', '-', '*', '/'};
        std::ostringstream oss;

        for (int i = 0; i < n; ++i) {
            oss << num_dist(gen);
            if (i < n - 1) {
                oss << ops[op_dist(gen)];
            }
        }
        return oss.str();
    }
};

#endif //EVALUATE_CHECK_H
