#include "Tests.hpp"

#include <system_error>

void AnyBox::Tests::CheckVirtualValidation() {
    for (auto forbidden : {MEM_RESET, MEM_RESET_UNDO, 0x800000}) {
        ExpectException<std::invalid_argument>([&] { XMemVirtualAlloc(nullptr, 0x10000, forbidden, 1, PAGE_READWRITE); });
    }
    for (auto attributes : {0ULL, 2ULL, 4ULL, 0x80ULL}) {
        ExpectException<std::invalid_argument>([&] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE | MEM_COMMIT, attributes, PAGE_READWRITE); });
    }
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE | MEM_COMMIT, 3, PAGE_READWRITE | PAGE_NOCACHE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE, 0x81, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE, 0x21, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, 0x20403000, 0x21, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, 0x21, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE, 0x11, PAGE_READWRITE); });
    for (auto mode : {0x20400000U, 0x20000000U}) {
        try {
            XMemVirtualAlloc(nullptr, 0x10000, mode | MEM_RESERVE | MEM_COMMIT, 0x11, PAGE_READWRITE);
            throw std::runtime_error("Unavailable XMem tools allocation succeeded");
        } catch (const std::system_error& error) {
            Require(error.code().value() == ERROR_NOT_FOUND, "Unavailable XMem tools allocation returned the wrong error");
        }
    }
}
