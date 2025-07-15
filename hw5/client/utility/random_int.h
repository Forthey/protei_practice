#ifndef RANDOM_INT_H
#define RANDOM_INT_H
#include <random>

int random_int(int min, int max) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

#endif //RANDOM_INT_H
