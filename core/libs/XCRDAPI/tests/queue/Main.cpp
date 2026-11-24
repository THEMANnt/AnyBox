#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckMemoryQueue();
        AnyBox::Tests::CheckCancellation();
        AnyBox::Tests::CheckQueueLimits();
        AnyBox::Tests::CheckQueueInformation();
        AnyBox::Tests::CheckQueueErrors();
        AnyBox::Tests::CheckFileQueue();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
