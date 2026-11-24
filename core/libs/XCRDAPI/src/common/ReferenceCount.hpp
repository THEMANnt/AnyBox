#ifndef ANYBOX_STORAGE_REFERENCE_COUNT_HPP
#define ANYBOX_STORAGE_REFERENCE_COUNT_HPP

#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace AnyBox::Storage {

class ReferenceCount {
public:
    explicit ReferenceCount(std::uint32_t initial) : count(initial) {}
    std::uint32_t Add() {
        auto value = count.load(std::memory_order_relaxed);
        do {
            if (value == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("DirectStorage reference count overflows");
        } while (!count.compare_exchange_weak(value, value + 1, std::memory_order_relaxed));
        return value + 1;
    }
    std::uint32_t Release() {
        auto value = count.load(std::memory_order_relaxed);
        do {
            if (value == 0) throw std::logic_error("DirectStorage object has no references");
        } while (!count.compare_exchange_weak(value, value - 1, std::memory_order_acq_rel));
        return value - 1;
    }

private:
    std::atomic<std::uint32_t> count;
};

}

#endif
