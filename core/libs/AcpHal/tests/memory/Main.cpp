#include "Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>

namespace {

std::uint32_t backingAllocations = 0;
std::uint32_t backingFrees = 0;

void* Allocate(std::size_t, std::uint64_t) {
    ++backingAllocations;
    throw std::runtime_error("APU allocation incorrectly used title XMem hooks");
}

void Free(void*, std::uint64_t) {
    ++backingFrees;
    throw std::runtime_error("APU release incorrectly used title XMem hooks");
}

}

struct TitleHooks {
    decltype(&Allocate) Allocation;
    decltype(&Free) Release;
};

extern "C" {
TitleHooks __XMemAllocationHooks__{Allocate, Free};
}

int main() {
    try {
        using namespace AnyBox::Tests;
        Require(!ApuIsVirtualAddressValid(nullptr, 64), "Null APU address is valid");
        Require(ApuMapVirtualAddress(nullptr) == 0 && ApuMapApuAddress(0) == nullptr, "Null APU translations changed");
        ExpectException<std::invalid_argument>([] { ApuCreateHeap(0, 0); });
        ExpectException<std::length_error>([] { ApuCreateHeap(0xFFFFFFFF, 0); });
        ExpectException<std::length_error>([] { ApuCreateHeap(0x100000, 0xFFFFFFFF); });
        Require(backingAllocations == 0, "Rejected APU heap allocated storage");
        Require(ApuCreateHeap(0x100000, 0x226E00) == 0, "APU heap creation failed");
        ExpectException<std::logic_error>([] { ApuCreateHeap(0x100000, 0x226E00); });
        CheckAllocation();
        CheckMapping();
        CheckErrors();
        CheckConcurrency();
        Require(backingAllocations == 0 && backingFrees == 0, "APU pools invoked title XMem hooks");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
