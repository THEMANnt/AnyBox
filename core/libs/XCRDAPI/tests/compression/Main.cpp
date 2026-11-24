#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckTrackedMemory();
        AnyBox::Tests::CheckStaging();
        AnyBox::Tests::CheckZlibDecoder();
        AnyBox::Tests::CheckZlibStream();
        AnyBox::Tests::CheckCompressedMemory();
        AnyBox::Tests::CheckCompressedFile();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
