#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckFourierTransform();
        AnyBox::Tests::CheckConvolution();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
