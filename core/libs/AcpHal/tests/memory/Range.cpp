#include "../../src/memory/Pool.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const TException&) { return; }
    throw std::runtime_error("Invalid APU memory range was accepted");
}
}

int main() {
    try {
        AnyBox::AcpHal::Pool pool(4096, 0x10000);
        auto* first = static_cast<std::byte*>(pool.Allocate(256, 256));
        auto* second = pool.Allocate(256, 256);
        Require(pool.GetMemory(0x10000, 256).data() == first, "APU memory range maps to a different allocation");
        auto tail = pool.GetMemory(0x100FF, 1);
        tail[0] = std::byte{0x73};
        Require(first[255] == std::byte{0x73}, "APU memory range is not a view of its backing allocation");
        Reject<std::length_error>([&] { pool.GetMemory(0x10000, 257); });
        Reject<std::length_error>([&] { pool.GetMemory(0x100FF, 2); });
        Reject<std::length_error>([&] { pool.GetMemory(0x10000, static_cast<std::size_t>(-1)); });
        Reject<std::invalid_argument>([&] { pool.GetMemory(0x10000 - 1, 1); });
        Reject<std::invalid_argument>([&] { pool.GetMemory(0x11000, 1); });
        Require(pool.GetMemory(0x10000, 0).empty(), "Empty APU memory range changed size");
        pool.Free(first);
        Reject<std::invalid_argument>([&] { pool.GetMemory(0x10000, 1); });
        pool.Free(second);
        Reject<std::invalid_argument>([&] { pool.GetMemory(0x10100, 1); });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
