#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>

void AnyBox::Tests::CheckHeapErrors() {
    ExpectException<std::invalid_argument>([] { XMemAlloc(16, 1ULL << 34); });
    ExpectException<std::invalid_argument>([] { XMemAlloc(16, 3ULL << 14); });
    ExpectException<std::invalid_argument>([] { XMemAlloc(16, 10ULL << 29); });
    ExpectException<std::bad_alloc>([] { XMemAlloc(std::numeric_limits<std::size_t>::max(), 0); });
    std::array<std::byte, 32> foreign{};
    ExpectException<std::invalid_argument>([&] { XMemFree(foreign.data(), 0); });
    auto* allocation = XMemAlloc(64, 0);
    ExpectException<std::invalid_argument>([&] { XMemFree(static_cast<std::byte*>(allocation) + 1, 0); });
    ExpectException<std::invalid_argument>([&] { XMemFree(allocation, 1); });
    std::memset(allocation, 0x5A, 64);
    XMemFree(allocation, 0);
    ExpectException<std::invalid_argument>([&] { XMemFree(allocation, 0); });
}
