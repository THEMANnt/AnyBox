#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckStatisticsLifetime();
        AnyBox::Tests::CheckStatisticsMemory();
        AnyBox::Tests::CheckStatisticsFile();
        AnyBox::Tests::CheckStatisticsFence();
        AnyBox::Tests::CheckStatisticsConcurrency();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
