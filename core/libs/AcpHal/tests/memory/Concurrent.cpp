#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <exception>
#include <thread>

void AnyBox::Tests::CheckConcurrency() {
    std::array<std::exception_ptr, 8> errors{};
    std::array<std::jthread, 8> threads;
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::jthread([&errors, i] {
            try {
                for (std::uint32_t j = 0; j < 64; ++j) {
                    void* allocation = nullptr;
                    ApuAlloc(&allocation, nullptr, 2048, 2048, j % 2 + 1);
                    auto value = static_cast<int>(i + 1);
                    std::memset(allocation, value, 2048);
                    auto* bytes = static_cast<const std::byte*>(allocation);
                    for (std::size_t k = 0; k < 2048; ++k) Require(bytes[k] == std::byte(value), "Concurrent APU allocations overlap");
                    Require(ApuMapApuAddress(ApuMapVirtualAddress(allocation)) == allocation, "Concurrent APU translation failed");
                    ApuFree(allocation);
                }
            } catch (...) {
                errors[i] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}
