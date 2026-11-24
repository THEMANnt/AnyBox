#include "Configuration.hpp"

#include <stdexcept>
#include <system_error>

void AnyBox::Storage::WorkerConfiguration::SetAffinity(std::uint64_t value) {
    if (value == 0) throw std::invalid_argument("DirectStorage CPU affinity mask is empty");
    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask)) Windows::ThrowLastError("GetProcessAffinityMask DirectStorage");
    if ((value & ~processMask) != 0) throw std::invalid_argument("DirectStorage CPU affinity includes unavailable processors");

    std::lock_guard lock(mutex);
    TrackedVector<DWORD_PTR> previous(threads.size());
    for (std::size_t index = 0; index < threads.size(); ++index) {
        previous[index] = SetThreadAffinityMask(threads[index], static_cast<DWORD_PTR>(value));
        if (previous[index] != 0) continue;
        auto error = GetLastError();
        for (std::size_t changed = 0; changed < index; ++changed) {
            if (!SetThreadAffinityMask(threads[changed], previous[changed])) std::terminate();
        }
        throw std::system_error(static_cast<int>(error), std::system_category(), "SetThreadAffinityMask DirectStorage");
    }
    affinity = value;
}
