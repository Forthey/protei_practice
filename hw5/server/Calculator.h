#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <sstream>
#include <string>
#include <vector>


class Calculator {
public:
    static long evaluate(const std::string &expr) {
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
};

#endif //CALCULATOR_H
