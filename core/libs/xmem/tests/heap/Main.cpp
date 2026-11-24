#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckDefaultHeap();
        AnyBox::Tests::CheckGraphicsHeap();
        AnyBox::Tests::CheckHeapErrors();
        AnyBox::Tests::CheckHeapConcurrency();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
