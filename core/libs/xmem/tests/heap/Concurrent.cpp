#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <exception>
#include <thread>

void AnyBox::Tests::CheckHeapConcurrency() {
    std::array<std::exception_ptr, 8> errors{};
    std::array<std::jthread, 8> threads;
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::jthread([&errors, i] {
            try {
                for (std::uint64_t j = 0; j < 128; ++j) {
                    auto attributes = (j % 16) << 24;
                    auto* allocation = XMemAlloc(1024, attributes);
                    auto* bytes = static_cast<std::byte*>(allocation);
                    std::memset(allocation, static_cast<int>(i + 1), 1024);
                    for (std::size_t k = 0; k < 1024; ++k) Require(bytes[k] == std::byte(i + 1), "Concurrent XMem allocations overlap");
                    XMemFree(allocation, attributes);
                }
            } catch (...) {
                errors[i] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}
