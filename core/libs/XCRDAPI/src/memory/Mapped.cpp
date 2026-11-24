#include "View.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <limits>
#include <stdexcept>

bool AnyBox::Storage::IsMappedMemory(std::span<std::byte> memory) {
    auto address = reinterpret_cast<std::uintptr_t>(memory.data());
    auto end = address + memory.size();
    while (address < end) {
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(reinterpret_cast<const void*>(address), &information, sizeof(information))) Windows::ThrowLastError("VirtualQuery DirectStorage memory alias");
        if (information.Type == MEM_MAPPED) return true;
        auto base = reinterpret_cast<std::uintptr_t>(information.BaseAddress);
        if (information.RegionSize > std::numeric_limits<std::uintptr_t>::max() - base) throw std::overflow_error("DirectStorage alias range overflows");
        auto next = base + information.RegionSize;
        if (next <= address) throw std::runtime_error("DirectStorage alias query did not advance");
        address = next;
    }
    return false;
}
