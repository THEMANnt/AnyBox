#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckClientLimits();
        AnyBox::Tests::CheckFftStreams();
        AnyBox::Tests::CheckConvolutionStream();
        AnyBox::Tests::CheckConcurrentStreams();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
