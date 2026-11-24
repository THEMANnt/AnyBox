#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckFactory();
        AnyBox::Tests::CheckFiles();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
