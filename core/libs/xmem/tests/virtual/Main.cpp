#include "Tests.hpp"

#include <iostream>

int main() {
    try {
        AnyBox::Tests::CheckQuery();
        AnyBox::Tests::CheckVirtualAllocation();
        AnyBox::Tests::CheckAddressRanges();
        AnyBox::Tests::CheckVirtualValidation();
        AnyBox::Tests::CheckStatistics();
        AnyBox::Tests::CheckStackStatistics();
        AnyBox::Tests::CheckTransfer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
