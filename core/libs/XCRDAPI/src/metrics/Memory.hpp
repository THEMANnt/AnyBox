#ifndef ANYBOX_STORAGE_TRACKED_MEMORY_HPP
#define ANYBOX_STORAGE_TRACKED_MEMORY_HPP

#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace AnyBox::Storage {

void AddMemory(std::size_t size) noexcept;
void RemoveMemory(std::size_t size) noexcept;
void* AllocateCodecMemory(std::size_t size) noexcept;
void FreeCodecMemory(void* memory) noexcept;

class TrackedObject {
public:
    static void* operator new(std::size_t size) {
        auto* memory = ::operator new(size);
        AddMemory(size);
        return memory;
    }
    static void operator delete(void* memory, std::size_t size) noexcept {
        RemoveMemory(size);
        ::operator delete(memory);
    }
};

template<class TValue>
class TrackedAllocator {
public:
    using value_type = TValue;
    TrackedAllocator() = default;
    template<class TOther>
    TrackedAllocator(const TrackedAllocator<TOther>&) noexcept {}
    TValue* allocate(std::size_t count) {
        auto* memory = std::allocator<TValue>{}.allocate(count);
        AddMemory(count * sizeof(TValue));
        return memory;
    }
    void deallocate(TValue* memory, std::size_t count) noexcept {
        RemoveMemory(count * sizeof(TValue));
        std::allocator<TValue>{}.deallocate(memory, count);
    }
    template<class TOther>
    bool operator==(const TrackedAllocator<TOther>&) const noexcept { return true; }
};

template<class TValue>
struct FreeTrackedArray {
    std::size_t count = 0;
    void operator()(TValue* memory) const noexcept {
        RemoveMemory(count * sizeof(TValue));
        delete[] memory;
    }
};

template<class TValue>
using TrackedArray = std::unique_ptr<TValue[], FreeTrackedArray<TValue>>;

template<class TValue>
TrackedArray<TValue> MakeTrackedArray(std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(TValue)) throw std::bad_array_new_length();
    auto* memory = new TValue[count];
    AddMemory(count * sizeof(TValue));
    return TrackedArray<TValue>(memory, {count});
}

template<class TValue>
using TrackedVector = std::vector<TValue, TrackedAllocator<TValue>>;

template<class TValue>
using TrackedString = std::basic_string<TValue, std::char_traits<TValue>, TrackedAllocator<TValue>>;

}

#endif
